#ifndef RATIMOS_PROGRESSION_H
#define RATIMOS_PROGRESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../storage/content_api.h"

/*
 * Manifesto de progressao do castelo/jardim (PROGRESSAO-01, D-03/D-04/D-05/D-06a).
 *
 * Contrato de extensibilidade (RESEARCH.md, Follow-up Round 2, secao 3):
 *
 *   - `stage_count` e o UNICO ponto de crescimento. Uma atualizacao futura de
 *     firmware pode subir `stage_count` e acrescentar linhas
 *     {threshold, asset_id} sem mexer em nenhum codigo de renderizacao e sem
 *     mudar o layout de nenhuma struct.
 *   - `manifest_version` so sobe numa mudanca de LAYOUT que quebre
 *     compatibilidade -- nunca por causa do crescimento de `stage_count`.
 *   - `asset_id` e uma chave de STRING resolvida em tempo de execucao contra
 *     uma tabela de imagens (ratimos_progress_image_by_id(), adicionada pelo
 *     plano 02.1-03 Tarefa 2), nao um ponteiro compilado nem um valor de
 *     enum -- assim uma imagem nova entra junto com sua propria linha de
 *     manifest, sem exigir nenhum switch/case novo em lugar nenhum.
 *   - Um save gravado sob um `stage_count` menor continua valido depois de
 *     uma atualizacao: `shared_completions` e sempre comparado contra
 *     `threshold`, nunca "switchado" num numero de estagio fixo -- o
 *     progresso ja conquistado nunca precisa de migracao.
 */
#define RATIMOS_PROGRESSION_MAX_STAGES 32 /* folga generosa -- esta fase preenche
                                              6, atualizacoes futuras podem
                                              acrescentar ate 26 a mais sem
                                              nenhuma mudanca de struct/layout */

typedef struct {
    uint16_t threshold;   /* valor de shared_completions em que este estagio desbloqueia */
    char asset_id[40];    /* chave de string resolvida em tempo de execucao,
                              NUNCA um ponteiro compilado nem um case de switch --
                              40 bytes comporta o id mais longo do manifesto v1
                              ("castle_stage_04_bandeira_jardim_cheio", 37 chars)
                              com folga para ids futuros um pouco maiores */
} ratimos_progression_stage_t;

typedef struct {
    uint16_t manifest_version;                            /* sobe so numa mudanca de layout que quebre compatibilidade */
    uint16_t stage_count;                                 /* ESTE cresce em atualizacoes futuras -- o ponto de extensibilidade */
    ratimos_progression_stage_t stages[RATIMOS_PROGRESSION_MAX_STAGES];
} ratimos_progression_manifest_t;

/* Retorna o manifesto v1 compilado (somente leitura). */
const ratimos_progression_manifest_t * ratimos_progression_manifest(void);

/* Indice do estagio mais alto cujo threshold e <= shared_completions. Nunca
 * le alem de stage_count nem de RATIMOS_PROGRESSION_MAX_STAGES. */
size_t ratimos_progression_stage_index(uint16_t shared_completions);

/* Atalho para stages[stage_index(shared_completions)].asset_id. */
const char * ratimos_progression_stage_asset_id(uint16_t shared_completions);

/* Maior threshold populado -- o alvo "fim de jogo" da progressao (D-04). */
uint16_t ratimos_progression_target_completions(void);

/* Verdadeiro quando shared_completions >= ratimos_progression_target_completions(). */
bool ratimos_progression_is_complete(uint16_t shared_completions);

/* Id do desbloqueio decorativo exclusivo de `game` (D-05). Faz bounds-check
 * contra RATIMOS_GAME_COUNT antes de indexar; retorna NULL fora do intervalo. */
const char * ratimos_progression_unlock_asset_id(ratimos_game_kind_t game);

#endif
