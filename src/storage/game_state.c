/*
 * Storage/Content API — dominio `game_state` (D-10).
 *
 * Este e o PRIMEIRO dominio de ESCRITA da Storage API. letters.c/photos.c/
 * tracks.c/games.c/settings.c so leem; aqui o dispositivo grava.
 *
 * Escopo/portabilidade: esta implementacao e do ambiente `native_sim` (PC) —
 * usa stream I/O da libc (fopen/fread/fwrite/rename) contra arquivos em
 * assets/save/. A Fase 5 (hardware) troca ESTAS chamadas pelo caminho real de
 * SD/NVS mantendo as assinaturas de content_api.h intactas: nenhum jogo
 * precisa mudar uma linha quando isso acontecer.
 *
 * Disciplina V12 (herdada de letters.c/photos.c): os caminhos sao uma tabela
 * const compilada. Nada de caminho montado a partir de conteudo, nada de
 * varredura de diretorio.
 *
 * Disciplina V5: o arquivo lido do disco e ENTRADA NAO CONFIAVEL — pode ter
 * sido truncado por um processo morto no meio da escrita, corrompido por um
 * cartao cheio (hardware futuro) ou editado na mao. Ver validate_record().
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "content_api.h"

#define RATIMOS_SAVE_MAGIC          0x524d4753u  /* "RMGS" */
#define RATIMOS_SAVE_FORMAT_VERSION 1u

#define SAVE_DIR       "assets/save"
#define SAVE_PATH_MAX  64

/* Tabela fixa de caminhos, indexada por ratimos_game_kind_t. A ordem TEM de
 * espelhar o enum — o _Static_assert abaixo trava isso em tempo de compilacao. */
static const char * const s_state_paths[RATIMOS_GAME_COUNT] = {
    SAVE_DIR "/sudoku.bin",
    SAVE_DIR "/paciencia.bin",
    SAVE_DIR "/termo.bin",
    SAVE_DIR "/cruzadinha.bin",
    SAVE_DIR "/conexo.bin",
};

static const char * const s_progression_path = SAVE_DIR "/progression.bin";

_Static_assert(sizeof(s_state_paths) / sizeof(s_state_paths[0]) == RATIMOS_GAME_COUNT,
               "tabela de caminhos de save dessincronizada de ratimos_game_kind_t");

/* Registro em disco de um tabuleiro: cabecalho fixo + blob completo. O blob e
 * sempre gravado inteiro (512B) para que todo save tenha exatamente o mesmo
 * tamanho — um arquivo de tamanho diferente ja e, por si so, invalido. */
typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t game_kind;
    uint32_t used;
    uint32_t checksum;
    uint8_t bytes[RATIMOS_GAME_STATE_BLOB_SIZE];
} save_record_t;

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t reserved;
    uint32_t checksum;
    uint16_t shared_completions;
    uint8_t game_exclusive_unlocked[RATIMOS_GAME_COUNT];
    uint8_t reserved2;
} progression_record_t;

/* O registro e gravado como bytes crus da struct, entao o tamanho precisa ser
 * previsivel: se o compilador inserir padding inesperado, o save antigo deixa
 * de casar com o novo binario silenciosamente. Falhar aqui e melhor. */
_Static_assert(sizeof(save_record_t) == 16 + RATIMOS_GAME_STATE_BLOB_SIZE,
               "save_record_t ganhou padding inesperado");
_Static_assert(sizeof(progression_record_t) == 20,
               "progression_record_t ganhou padding inesperado");

/* Soma aditiva de 32 bits. Nao e criptografia (o save nao e secreto, V6 nao
 * se aplica): serve so para pegar corrupcao acidental de bytes. */
static uint32_t additive_checksum(const uint8_t * data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

static bool kind_in_range(ratimos_game_kind_t game)
{
    return (int) game >= 0 && (int) game < (int) RATIMOS_GAME_COUNT;
}

/*
 * Escrita atomica: grava em "<path>.tmp", fecha, e so entao renomeia por cima
 * do caminho final. rename() e atomico dentro do mesmo diretorio, entao um
 * processo morto no meio da escrita deixa no maximo um .tmp orfao — o save
 * anterior, completo, continua intacto (ameaca T-02.1-04).
 */
static bool write_atomic(const char * path, const void * data, size_t size)
{
    char tmp[SAVE_PATH_MAX + 8];

    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int) sizeof(tmp)) {
        return false;
    }

    FILE * f = fopen(tmp, "wb");
    if (!f) {
        return false;
    }

    bool ok = (fwrite(data, 1, size, f) == size) && (fflush(f) == 0);

    if (fclose(f) != 0) {
        ok = false;
    }

    if (!ok || rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }

    return true;
}

/* Le exatamente `size` bytes. Um arquivo maior OU menor que isso e recusado:
 * truncamento e lixo extra sao os dois lados da mesma falha. */
static bool read_exact(const char * path, void * out, size_t size, bool * out_missing)
{
    *out_missing = false;

    FILE * f = fopen(path, "rb");
    if (!f) {
        *out_missing = true;
        return false;
    }

    size_t got = fread(out, 1, size, f);
    /* fgetc extra: se ainda houver byte depois de `size`, o arquivo e maior
     * que o registro esperado e portanto invalido. */
    bool has_trailing = (fgetc(f) != EOF);
    fclose(f);

    return got == size && !has_trailing;
}

/* ------------------------------------------------------------------------
 * FRONTEIRA DE ENTRADA NAO CONFIAVEL (ASVS V5, ameacas T-02.1-01/T-02.1-02).
 *
 * Tudo abaixo desta linha veio do disco. Nenhum campo pode ser usado — nem
 * para dimensionar um loop, nem para indexar um array de tamanho fixo — antes
 * de passar por validate_record(). Mesmo espirito do fallback defensivo de
 * tracks.c (ratimos_storage_tracks_read_title_or_default), so que aqui a
 * consequencia de confiar seria leitura fora dos limites, nao um titulo feio.
 * ------------------------------------------------------------------------ */
static bool validate_record(const save_record_t * rec, ratimos_game_kind_t requested)
{
    if (rec->magic != RATIMOS_SAVE_MAGIC) {
        return false;
    }
    if (rec->format_version != RATIMOS_SAVE_FORMAT_VERSION) {
        return false;
    }
    if (rec->game_kind >= (uint16_t) RATIMOS_GAME_COUNT) {
        return false;
    }
    if (rec->game_kind != (uint16_t) requested) {
        return false;
    }
    if (rec->used > (uint32_t) RATIMOS_GAME_STATE_BLOB_SIZE) {
        return false;
    }
    if (rec->checksum != additive_checksum(rec->bytes, RATIMOS_GAME_STATE_BLOB_SIZE)) {
        return false;
    }
    return true;
}

void ratimos_storage_index_game_state(void)
{
    /* Cria assets/ e assets/save/ se ainda nao existirem. Um EEXIST e o caso
     * normal (segundo boot em diante), nao um erro. */
    if (mkdir("assets", 0777) != 0 && errno != EEXIST) {
        /* sem diretorio, os saves simplesmente falham de forma silenciosa e
         * o jogador cai no caminho ABSENT — nunca um crash. */
    }
    if (mkdir(SAVE_DIR, 0777) != 0 && errno != EEXIST) {
        /* idem */
    }
}

ratimos_game_state_status_t ratimos_storage_get_game_state(ratimos_game_kind_t game,
                                                           ratimos_game_state_t * out)
{
    if (!out) {
        return RATIMOS_GAME_STATE_INVALID;
    }

    memset(out, 0, sizeof(*out));

    /* Checagem de limites ANTES de tocar em s_state_paths[game]. */
    if (!kind_in_range(game)) {
        return RATIMOS_GAME_STATE_INVALID;
    }

    save_record_t rec;
    bool missing = false;

    if (!read_exact(s_state_paths[game], &rec, sizeof(rec), &missing)) {
        return missing ? RATIMOS_GAME_STATE_ABSENT : RATIMOS_GAME_STATE_INVALID;
    }

    if (!validate_record(&rec, game)) {
        return RATIMOS_GAME_STATE_INVALID;
    }

    memcpy(out->bytes, rec.bytes, RATIMOS_GAME_STATE_BLOB_SIZE);
    out->used = (size_t) rec.used;
    return RATIMOS_GAME_STATE_OK;
}

bool ratimos_storage_save_game_state(ratimos_game_kind_t game, const ratimos_game_state_t * state)
{
    if (!kind_in_range(game) || !state) {
        return false;
    }
    if (state->used > (size_t) RATIMOS_GAME_STATE_BLOB_SIZE) {
        return false;
    }

    save_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = RATIMOS_SAVE_MAGIC;
    rec.format_version = RATIMOS_SAVE_FORMAT_VERSION;
    rec.game_kind = (uint16_t) game;
    rec.used = (uint32_t) state->used;
    memcpy(rec.bytes, state->bytes, RATIMOS_GAME_STATE_BLOB_SIZE);
    rec.checksum = additive_checksum(rec.bytes, RATIMOS_GAME_STATE_BLOB_SIZE);

    return write_atomic(s_state_paths[game], &rec, sizeof(rec));
}

bool ratimos_storage_clear_game_state(ratimos_game_kind_t game)
{
    if (!kind_in_range(game)) {
        return false;
    }

    /* Apaga SO o tabuleiro. A progressao ja conquistada nunca regride por
     * causa de um reset de jogo (proibicao explicita do plano). */
    int rc = remove(s_state_paths[game]);
    return rc == 0 || errno == ENOENT;
}

/* Progressao: mesmo cabecalho/rename atomico, mas com CLAMP em vez de rejeicao
 * nos dois campos numericos — uma progressao limitada e estritamente melhor
 * para a jogadora do que uma progressao descartada. */
static void clamp_progression(ratimos_progression_state_t * p)
{
    if (p->shared_completions > RATIMOS_PROGRESSION_MAX_COMPLETIONS) {
        p->shared_completions = RATIMOS_PROGRESSION_MAX_COMPLETIONS;
    }
    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        p->game_exclusive_unlocked[i] = p->game_exclusive_unlocked[i] ? 1 : 0;
    }
}

bool ratimos_storage_get_progression(ratimos_progression_state_t * out)
{
    if (!out) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    progression_record_t rec;
    bool missing = false;

    if (!read_exact(s_progression_path, &rec, sizeof(rec), &missing)) {
        return false;
    }
    if (rec.magic != RATIMOS_SAVE_MAGIC || rec.format_version != RATIMOS_SAVE_FORMAT_VERSION) {
        return false;
    }

    uint32_t payload_offset = (uint32_t) offsetof(progression_record_t, shared_completions);
    uint32_t payload_len = (uint32_t) (sizeof(rec) - payload_offset);
    if (rec.checksum != additive_checksum(((const uint8_t *) &rec) + payload_offset, payload_len)) {
        return false;
    }

    out->shared_completions = rec.shared_completions;
    memcpy(out->game_exclusive_unlocked, rec.game_exclusive_unlocked, RATIMOS_GAME_COUNT);
    clamp_progression(out);
    return true;
}

bool ratimos_storage_save_progression(const ratimos_progression_state_t * state)
{
    if (!state) {
        return false;
    }

    ratimos_progression_state_t safe = *state;
    clamp_progression(&safe);

    progression_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = RATIMOS_SAVE_MAGIC;
    rec.format_version = RATIMOS_SAVE_FORMAT_VERSION;
    rec.shared_completions = safe.shared_completions;
    memcpy(rec.game_exclusive_unlocked, safe.game_exclusive_unlocked, RATIMOS_GAME_COUNT);

    uint32_t payload_offset = (uint32_t) offsetof(progression_record_t, shared_completions);
    uint32_t payload_len = (uint32_t) (sizeof(rec) - payload_offset);
    rec.checksum = additive_checksum(((const uint8_t *) &rec) + payload_offset, payload_len);

    return write_atomic(s_progression_path, &rec, sizeof(rec));
}

bool ratimos_storage_record_daily_win(ratimos_game_kind_t game)
{
    if (!kind_in_range(game)) {
        return false;
    }

    ratimos_progression_state_t p;
    if (!ratimos_storage_get_progression(&p)) {
        /* Sem progressao valida ainda: comeca do zero em vez de falhar — uma
         * vitoria real nunca pode ser perdida por causa de um arquivo ausente. */
        memset(&p, 0, sizeof(p));
    }

    if (p.shared_completions < RATIMOS_PROGRESSION_MAX_COMPLETIONS) {
        p.shared_completions++;
    }
    p.game_exclusive_unlocked[game] = 1;

    return ratimos_storage_save_progression(&p);
}
