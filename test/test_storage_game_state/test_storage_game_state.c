/*
 * Suite do dominio `game_state` (D-10) — o primeiro dominio de ESCRITA da
 * Storage/Content API. Roda via PlatformIO native Unity, sem LVGL/SDL,
 * exercitando o caminho real de fopen/fwrite/rename de src/storage/game_state.c
 * contra o diretorio `assets/save/` (gitignored, D-10).
 *
 * Cada teste chama ratimos_storage_index_game_state() e
 * ratimos_storage_clear_game_state() em setUp() para nao vazar estado de um
 * teste pro outro (o board fica em assets/save/conexo.bin, arquivo real em
 * disco entre execucoes do binario de teste). A progressao (assets/save/
 * progression.bin) e resetada explicitamente para zero em setUp() via
 * ratimos_storage_save_progression() -- e a unica forma de garantir uma
 * baseline deterministica sem violar a regra de "progressao nunca regride"
 * do jogo em si (aqui e o teste controlando sua propria fixture, nao o
 * jogador perdendo progresso real).
 *
 * As duas unicas mutacoes diretas de arquivo (fopen local, sem passar pela
 * Storage API) sao os testes de corrupcao/truncamento, que precisam simular
 * um save quebrado -- sempre reutilizando o MESMO caminho fixo em vez de
 * inventar um novo, espelhando a tabela const de src/storage/game_state.c.
 */
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "storage/content_api.h"

/* Mesmo caminho fixo de s_state_paths[RATIMOS_GAME_CONEXO] em game_state.c —
 * reutilizado por todos os testes de corrupcao em vez de inventado por teste. */
static const char * const CONEXO_SAVE_PATH = "assets/save/conexo.bin";
static const char * const CONEXO_SAVE_TMP_PATH = "assets/save/conexo.bin.tmp";

/* Espelha o layout PRIVADO de save_record_t em game_state.c (cabecalho fixo
 * + blob de RATIMOS_GAME_STATE_BLOB_SIZE bytes, sem padding -- travado la
 * pelo _Static_assert(sizeof(save_record_t) == 16 + BLOB_SIZE)). So existe
 * aqui para os testes de corrupcao de campo unico do Task 3 (magic errado,
 * used estourado, game_kind fora do range, checksum quebrado) poderem
 * escrever um registro do TAMANHO CORRETO com um unico campo adulterado --
 * mutacao direta de arquivo explicitamente permitida pelo plano so para
 * este proposito, sempre contra CONEXO_SAVE_PATH. */
typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t game_kind;
    uint32_t used;
    uint32_t checksum;
    uint8_t bytes[RATIMOS_GAME_STATE_BLOB_SIZE];
} test_save_record_t;

_Static_assert(sizeof(test_save_record_t) == 16 + RATIMOS_GAME_STATE_BLOB_SIZE,
               "test_save_record_t saiu de sincronia com o layout privado de game_state.c");

static void write_valid_conexo_save(void)
{
    ratimos_game_state_t state;
    memset(&state, 0, sizeof(state));
    state.used = 10;
    for (size_t i = 0; i < state.used; i++) {
        state.bytes[i] = (uint8_t) (i + 1);
    }
    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &state));
}

static void read_conexo_record(test_save_record_t * out)
{
    FILE * f = fopen(CONEXO_SAVE_PATH, "rb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(sizeof(*out), fread(out, 1, sizeof(*out), f));
    fclose(f);
}

static void write_conexo_record(const test_save_record_t * rec)
{
    FILE * f = fopen(CONEXO_SAVE_PATH, "wb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(sizeof(*rec), fwrite(rec, 1, sizeof(*rec), f));
    fclose(f);
}

static void assert_get_returns_invalid_and_zeroed_for_kind(ratimos_game_kind_t kind)
{
    ratimos_game_state_t out;
    memset(&out, 0xFF, sizeof(out));
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(kind, &out);

    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_INVALID, status);
    TEST_ASSERT_EQUAL_UINT(0, out.used);
    for (size_t i = 0; i < RATIMOS_GAME_STATE_BLOB_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, out.bytes[i]);
    }
}

static void assert_get_returns_invalid_and_zeroed(void)
{
    assert_get_returns_invalid_and_zeroed_for_kind(RATIMOS_GAME_CONEXO);
}

void setUp(void)
{
    ratimos_storage_index_game_state();
    ratimos_storage_clear_game_state(RATIMOS_GAME_CONEXO);

    ratimos_progression_state_t zero;
    memset(&zero, 0, sizeof(zero));
    ratimos_storage_save_progression(&zero);
}

void tearDown(void) {}

void test_get_absent_returns_absent_and_zeroed(void)
{
    ratimos_game_state_t out;
    memset(&out, 0xFF, sizeof(out)); /* venenoso: prova que a funcao zera de verdade */

    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &out);

    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_ABSENT, status);
    TEST_ASSERT_EQUAL_UINT(0, out.used);
    for (size_t i = 0; i < RATIMOS_GAME_STATE_BLOB_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, out.bytes[i]);
    }
}

void test_save_then_get_round_trip_returns_same_bytes_in_order(void)
{
    ratimos_game_state_t in;
    memset(&in, 0, sizeof(in));
    in.used = 40;
    for (size_t i = 0; i < 40; i++) {
        in.bytes[i] = (uint8_t) (i + 1);
    }

    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &in));

    ratimos_game_state_t out;
    memset(&out, 0, sizeof(out));
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &out);

    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_OK, status);
    TEST_ASSERT_EQUAL_UINT(40, out.used);
    for (size_t i = 0; i < 40; i++) {
        TEST_ASSERT_EQUAL_UINT8(in.bytes[i], out.bytes[i]);
    }
}

void test_last_write_wins_keeps_only_second_save(void)
{
    ratimos_game_state_t first;
    memset(&first, 0, sizeof(first));
    first.used = 1;
    first.bytes[0] = 1;
    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &first));

    ratimos_game_state_t second;
    memset(&second, 0, sizeof(second));
    second.used = 2;
    second.bytes[0] = 2;
    second.bytes[1] = 3;
    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &second));

    ratimos_game_state_t out;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &out);

    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_OK, status);
    TEST_ASSERT_EQUAL_UINT(2, out.used);
    TEST_ASSERT_EQUAL_UINT8(2, out.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(3, out.bytes[1]);
}

void test_save_leaves_no_stray_tmp_file(void)
{
    ratimos_game_state_t state;
    memset(&state, 0, sizeof(state));
    state.used = 1;

    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &state));

    FILE * f = fopen(CONEXO_SAVE_TMP_PATH, "rb");
    TEST_ASSERT_NULL(f);
}

void test_corrupted_file_returns_invalid_and_zeroed(void)
{
    ratimos_game_state_t state;
    memset(&state, 0, sizeof(state));
    state.used = 1;
    state.bytes[0] = 0xAB;
    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &state));

    /* Sobrescreve o save valido com poucos bytes de lixo — mais curto que o
     * registro esperado, entao read_exact() ja rejeita (T-02.1-01). Este e o
     * unico lugar da suite que toca o arquivo direto, via fopen local. */
    FILE * f = fopen(CONEXO_SAVE_PATH, "wb");
    TEST_ASSERT_NOT_NULL(f);
    uint8_t junk[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11, 0x22, 0x33 };
    fwrite(junk, 1, sizeof(junk), f);
    fclose(f);

    ratimos_game_state_t out;
    memset(&out, 0xFF, sizeof(out));
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &out);

    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_INVALID, status);
    TEST_ASSERT_EQUAL_UINT(0, out.used);
    for (size_t i = 0; i < RATIMOS_GAME_STATE_BLOB_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, out.bytes[i]);
    }
}

void test_clear_game_state_removes_board_but_not_progression(void)
{
    ratimos_game_state_t state;
    memset(&state, 0, sizeof(state));
    state.used = 4;
    TEST_ASSERT_TRUE(ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &state));
    TEST_ASSERT_TRUE(ratimos_storage_record_daily_win(RATIMOS_GAME_CONEXO));

    ratimos_progression_state_t before;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&before));

    TEST_ASSERT_TRUE(ratimos_storage_clear_game_state(RATIMOS_GAME_CONEXO));

    ratimos_game_state_t out;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &out);
    TEST_ASSERT_EQUAL_INT(RATIMOS_GAME_STATE_ABSENT, status);

    ratimos_progression_state_t after;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&after));
    TEST_ASSERT_EQUAL_UINT16(before.shared_completions, after.shared_completions);
    TEST_ASSERT_EQUAL_UINT8(before.game_exclusive_unlocked[RATIMOS_GAME_CONEXO],
                            after.game_exclusive_unlocked[RATIMOS_GAME_CONEXO]);
}

void test_progression_clamp_normalizes_extreme_values(void)
{
    ratimos_progression_state_t p;
    memset(&p, 0, sizeof(p));
    p.shared_completions = 65535;
    p.game_exclusive_unlocked[RATIMOS_GAME_CONEXO] = 200;

    TEST_ASSERT_TRUE(ratimos_storage_save_progression(&p));

    ratimos_progression_state_t out;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&out));
    TEST_ASSERT_EQUAL_UINT16(RATIMOS_PROGRESSION_MAX_COMPLETIONS, out.shared_completions);
    TEST_ASSERT_EQUAL_UINT8(1, out.game_exclusive_unlocked[RATIMOS_GAME_CONEXO]);
}

void test_record_daily_win_increments_and_sets_unlock_flag(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_record_daily_win(RATIMOS_GAME_CONEXO));

    ratimos_progression_state_t p;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&p));
    TEST_ASSERT_EQUAL_UINT16(1, p.shared_completions);
    TEST_ASSERT_EQUAL_UINT8(1, p.game_exclusive_unlocked[RATIMOS_GAME_CONEXO]);
    TEST_ASSERT_EQUAL_UINT8(0, p.game_exclusive_unlocked[RATIMOS_GAME_SUDOKU]);
}

void test_record_daily_win_twice_increments_counter_but_flag_stays_one(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_record_daily_win(RATIMOS_GAME_CONEXO));
    TEST_ASSERT_TRUE(ratimos_storage_record_daily_win(RATIMOS_GAME_CONEXO));

    ratimos_progression_state_t p;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&p));
    TEST_ASSERT_EQUAL_UINT16(2, p.shared_completions);
    TEST_ASSERT_EQUAL_UINT8(1, p.game_exclusive_unlocked[RATIMOS_GAME_CONEXO]);
}

void test_record_daily_win_rejects_out_of_range_game(void)
{
    TEST_ASSERT_FALSE(ratimos_storage_record_daily_win((ratimos_game_kind_t) RATIMOS_GAME_COUNT));

    ratimos_progression_state_t p;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&p));
    TEST_ASSERT_EQUAL_UINT16(0, p.shared_completions);
}

/* ------------------------------------------------------------------------
 * Task 3 — endurecimento do blob de save: validacao campo a campo (T-02.1-01
 * DoS / T-02.1-02 Tampering). Cada teste grava um save valido pela API
 * publica, adultera UM campo especifico via fopen local (tamanho do registro
 * sempre correto, exceto no teste de truncamento), e confirma que a leitura
 * rejeita com *out zerado -- nunca um crash, nunca leitura fora dos limites.
 * ------------------------------------------------------------------------ */

void test_wrong_magic_returns_invalid_and_zeroed(void)
{
    write_valid_conexo_save();

    test_save_record_t rec;
    read_conexo_record(&rec);
    rec.magic = 0xDEADBEEFu; /* so o magic muda -- resto do registro intacto */
    write_conexo_record(&rec);

    assert_get_returns_invalid_and_zeroed();
}

void test_truncated_to_half_length_returns_invalid_and_zeroed(void)
{
    write_valid_conexo_save();

    test_save_record_t rec;
    read_conexo_record(&rec);

    FILE * f = fopen(CONEXO_SAVE_PATH, "wb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(sizeof(rec) / 2, fwrite(&rec, 1, sizeof(rec) / 2, f));
    fclose(f);

    assert_get_returns_invalid_and_zeroed();
}

void test_used_field_exceeding_blob_size_returns_invalid_and_zeroed(void)
{
    write_valid_conexo_save();

    test_save_record_t rec;
    read_conexo_record(&rec);
    rec.used = (uint32_t) RATIMOS_GAME_STATE_BLOB_SIZE + 1u; /* checksum continua batendo com bytes[] */
    write_conexo_record(&rec);

    assert_get_returns_invalid_and_zeroed();
}

void test_stored_game_kind_out_of_range_returns_invalid_and_zeroed(void)
{
    write_valid_conexo_save();

    test_save_record_t rec;
    read_conexo_record(&rec);
    rec.game_kind = (uint16_t) RATIMOS_GAME_COUNT; /* fora do range, nunca deve indexar a tabela */
    write_conexo_record(&rec);

    assert_get_returns_invalid_and_zeroed();
}

void test_checksum_mismatch_after_byte_tamper_returns_invalid_and_zeroed(void)
{
    write_valid_conexo_save();

    test_save_record_t rec;
    read_conexo_record(&rec);
    rec.bytes[0] = (uint8_t) (rec.bytes[0] + 1); /* blob muda, checksum gravado nao acompanha */
    write_conexo_record(&rec);

    assert_get_returns_invalid_and_zeroed();
}

void test_get_game_state_rejects_out_of_range_kind_without_touching_disk(void)
{
    /* Nenhum save gravado nesta chamada de proposito -- se o bounds-check
     * nao acontecer ANTES de tocar disco, isso indexaria s_state_paths fora
     * dos limites em vez de simplesmente devolver INVALID. */
    assert_get_returns_invalid_and_zeroed_for_kind((ratimos_game_kind_t) RATIMOS_GAME_COUNT);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_get_absent_returns_absent_and_zeroed);
    RUN_TEST(test_save_then_get_round_trip_returns_same_bytes_in_order);
    RUN_TEST(test_last_write_wins_keeps_only_second_save);
    RUN_TEST(test_save_leaves_no_stray_tmp_file);
    RUN_TEST(test_corrupted_file_returns_invalid_and_zeroed);
    RUN_TEST(test_clear_game_state_removes_board_but_not_progression);
    RUN_TEST(test_progression_clamp_normalizes_extreme_values);
    RUN_TEST(test_record_daily_win_increments_and_sets_unlock_flag);
    RUN_TEST(test_record_daily_win_twice_increments_counter_but_flag_stays_one);
    RUN_TEST(test_record_daily_win_rejects_out_of_range_game);
    RUN_TEST(test_wrong_magic_returns_invalid_and_zeroed);
    RUN_TEST(test_truncated_to_half_length_returns_invalid_and_zeroed);
    RUN_TEST(test_used_field_exceeding_blob_size_returns_invalid_and_zeroed);
    RUN_TEST(test_stored_game_kind_out_of_range_returns_invalid_and_zeroed);
    RUN_TEST(test_checksum_mismatch_after_byte_tamper_returns_invalid_and_zeroed);
    RUN_TEST(test_get_game_state_rejects_out_of_range_kind_without_touching_disk);
    return UNITY_END();
}
