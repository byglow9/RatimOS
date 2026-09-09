/*
 * Progressao do castelo/jardim (PROGRESSAO-01) -- manifesto versionado e
 * aditivo-seguro (D-06a) + resolucao de estagio/desbloqueio.
 *
 * Modulo puramente de dados: nenhuma chamada a Storage API, nenhum LVGL.
 * Quem le/grava o contador persistido (shared_completions) e o chamador
 * (castelo_app.c) atraves de ratimos_storage_get_progression() -- este
 * arquivo so sabe transformar um contador em "qual estagio" e "qual id de
 * imagem", nunca onde o contador mora em disco.
 */
#include "progression.h"

/* Manifesto v1 (RESEARCH.md, Follow-up Round 2, secao 3): ~30 dias / 6
 * estagios discretos, confirmado por D-06a. Os indices 6..31 permanecem
 * zero-inicializados e inertes -- uma atualizacao futura sobe stage_count e
 * preenche mais linhas aqui, sem tocar em mais nada neste arquivo. */
static const ratimos_progression_manifest_t s_manifest_v1 = {
    .manifest_version = 1,
    .stage_count = 6,
    .stages = {
        { 0,  "castle_stage_00_terreno_vazio" },
        { 6,  "castle_stage_01_alicerce" },
        { 12, "castle_stage_02_muros_canteiro" },
        { 18, "castle_stage_03_torres_florindo" },
        { 24, "castle_stage_04_bandeira_jardim_cheio" },
        { 30, "castle_stage_05_completo" },
    },
};

/* Um id de desbloqueio exclusivo por jogo (D-05), na ORDEM de
 * ratimos_game_kind_t -- a escolha de qual decoracao pertence a qual jogo e
 * discricionaria (RESEARCH nao trava isso), fixada aqui uma unica vez. */
static const char * const s_unlock_asset_ids[RATIMOS_GAME_COUNT] = {
    [RATIMOS_GAME_SUDOKU]     = "unlock_sudoku_roseira",
    [RATIMOS_GAME_PACIENCIA]  = "unlock_paciencia_bandeira",
    [RATIMOS_GAME_TERMO]      = "unlock_termo_arvore",
    [RATIMOS_GAME_CRUZADINHA] = "unlock_cruzadinha_fonte",
    [RATIMOS_GAME_CONEXO]     = "unlock_conexo_portao",
};

const ratimos_progression_manifest_t * ratimos_progression_manifest(void)
{
    return &s_manifest_v1;
}

/* Nucleo de resolucao, parametrizado pelo manifesto -- linkage externa (nao
 * declarada em progression.h) para que a suite Unity possa apontar este
 * mesmo codigo para um manifesto local com uma setima linha e provar que
 * crescer stage_count muda so a resolucao, nao o codigo (D-06a). Mesmo
 * padrao de "helper interno com linkage externa, sem entrar no header
 * publico" ja usado por ratimos_storage_tracks_read_title_or_default(). */
size_t ratimos_progression_stage_index_in(const ratimos_progression_manifest_t * manifest,
                                          uint16_t shared_completions)
{
    if (!manifest) {
        return 0;
    }

    size_t best = 0;
    size_t limit = manifest->stage_count < RATIMOS_PROGRESSION_MAX_STAGES
                       ? manifest->stage_count
                       : RATIMOS_PROGRESSION_MAX_STAGES;

    /* Os thresholds sao estritamente crescentes (invariante estrutural
     * pinada pela suite de testes) -- assim que um threshold ultrapassa o
     * contador, nenhum estagio depois dele pode qualificar. */
    for (size_t i = 0; i < limit; i++) {
        if (manifest->stages[i].threshold <= shared_completions) {
            best = i;
        } else {
            break;
        }
    }

    return best;
}

size_t ratimos_progression_stage_index(uint16_t shared_completions)
{
    return ratimos_progression_stage_index_in(&s_manifest_v1, shared_completions);
}

const char * ratimos_progression_stage_asset_id(uint16_t shared_completions)
{
    size_t idx = ratimos_progression_stage_index(shared_completions);
    return s_manifest_v1.stages[idx].asset_id;
}

uint16_t ratimos_progression_target_completions(void)
{
    if (s_manifest_v1.stage_count == 0) {
        return 0;
    }
    return s_manifest_v1.stages[s_manifest_v1.stage_count - 1].threshold;
}

bool ratimos_progression_is_complete(uint16_t shared_completions)
{
    return shared_completions >= ratimos_progression_target_completions();
}

const char * ratimos_progression_unlock_asset_id(ratimos_game_kind_t game)
{
    if ((int) game < 0 || (int) game >= (int) RATIMOS_GAME_COUNT) {
        return NULL;
    }
    return s_unlock_asset_ids[game];
}
