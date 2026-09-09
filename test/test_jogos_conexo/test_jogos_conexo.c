/*
 * Suite do motor do conexo (JOGOS-05) — src/ratimos/apps/jogos/conexo.c.
 *
 * Exercita apenas o motor (start/submit/shuffle), que e deliberadamente
 * separado de qualquer chamada LVGL para nao precisar de SDL/display para
 * rodar via PlatformIO native Unity. `conexo.h` inclui lvgl.h por causa de
 * ratimos_conexo_show(), mas nenhuma funcao de motor toca em lv_obj_t.
 */
#include <string.h>
#include <unity.h>

#include "ratimos/apps/jogos/conexo.h"
#include "ratimos/apps/jogos/conexo_puzzles.h"
#include "storage/content_api.h"

/* Task 2 acrescenta ratimos_conexo_record_win_if_needed(), que chama a
 * Storage API de verdade -- por isso setUp() agora reseta o dominio
 * game_state exatamente como test_storage_game_state.c faz, para nao vazar
 * progressao/board de um teste pro outro. */
void setUp(void)
{
    ratimos_storage_index_game_state();
    ratimos_storage_clear_game_state(RATIMOS_GAME_CONEXO);

    ratimos_progression_state_t zero;
    memset(&zero, 0, sizeof(zero));
    ratimos_storage_save_progression(&zero);
}

void tearDown(void) {}

static void load_puzzle(ratimos_conexo_puzzle_t * out)
{
    TEST_ASSERT_TRUE(ratimos_conexo_get_puzzle(0, out));
}

/* Mesma formula documentada no header (tile_word[slot] = grupo*4 + palavra):
 * usada aqui so pra localizar, apos o embaralhamento inicial, quais slots do
 * grid pertencem a um grupo alvo — nao duplica logica de negocio nenhuma. */
static void slots_of_group(const ratimos_conexo_state_t * state, uint8_t group, uint8_t out_slots[4])
{
    uint8_t n = 0;
    for (uint8_t slot = 0; slot < 16 && n < 4; slot++) {
        if ((state->tile_word[slot] / 4) == group) {
            out_slots[n++] = slot;
        }
    }
    TEST_ASSERT_EQUAL_UINT8(4, n);
}

void test_submit_with_three_selected_is_rejected_incomplete(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 12345);

    uint8_t g0[4];
    slots_of_group(&state, 0, g0);

    state.selected_count = 3;
    state.selected[0] = g0[0];
    state.selected[1] = g0[1];
    state.selected[2] = g0[2];

    ratimos_conexo_submit_t result = ratimos_conexo_submit(&state, &puzzle);

    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_REJECTED_INCOMPLETE, result);
    TEST_ASSERT_EQUAL_UINT8(0, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(0, state.solved_count);
    TEST_ASSERT_EQUAL_UINT8(3, state.selected_count); /* selecao intocada */
}

void test_submit_all_four_correct_solves_group(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 555);

    uint8_t g0[4];
    slots_of_group(&state, 0, g0);

    state.selected_count = 4;
    memcpy(state.selected, g0, 4);

    ratimos_conexo_submit_t result = ratimos_conexo_submit(&state, &puzzle);

    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_CORRECT, result);
    TEST_ASSERT_EQUAL_UINT8(1, state.solved_count);
    TEST_ASSERT_EQUAL_UINT8(0, state.solved_group_order[0]);
    TEST_ASSERT_EQUAL_UINT8(0, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(0, state.selected_count); /* selecao limpa apos acertar */
}

void test_submit_exactly_three_shared_is_one_away(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 777);

    uint8_t g0[4];
    uint8_t g1[4];
    slots_of_group(&state, 0, g0);
    slots_of_group(&state, 1, g1);

    state.selected_count = 4;
    state.selected[0] = g0[0];
    state.selected[1] = g0[1];
    state.selected[2] = g0[2];
    state.selected[3] = g1[0];

    ratimos_conexo_submit_t result = ratimos_conexo_submit(&state, &puzzle);

    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_ONE_AWAY, result);
    TEST_ASSERT_EQUAL_UINT8(1, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(0, state.solved_count); /* nunca resolve parcialmente */
}

void test_submit_fully_wrong_counts_mistake(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 999);

    uint8_t g0[4];
    uint8_t g1[4];
    slots_of_group(&state, 0, g0);
    slots_of_group(&state, 1, g1);

    /* 2 de cada grupo: melhor contagem e 2, nem CORRECT (4) nem ONE_AWAY (3). */
    state.selected_count = 4;
    state.selected[0] = g0[0];
    state.selected[1] = g0[1];
    state.selected[2] = g1[0];
    state.selected[3] = g1[1];

    ratimos_conexo_submit_t result = ratimos_conexo_submit(&state, &puzzle);

    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_WRONG, result);
    TEST_ASSERT_EQUAL_UINT8(1, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(0, state.solved_count);
}

void test_mistake_ceiling_reaches_finished_revealed_and_stops_recording_wins(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 2468);

    uint8_t g0[4];
    uint8_t g1[4];
    slots_of_group(&state, 0, g0);
    slots_of_group(&state, 1, g1);

    for (int i = 0; i < RATIMOS_CONEXO_MAX_MISTAKES; i++) {
        state.selected_count = 4;
        state.selected[0] = g0[0];
        state.selected[1] = g0[1];
        state.selected[2] = g1[0];
        state.selected[3] = g1[1];

        ratimos_conexo_submit_t result = ratimos_conexo_submit(&state, &puzzle);
        TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_WRONG, result);
    }

    TEST_ASSERT_EQUAL_UINT8(RATIMOS_CONEXO_MAX_MISTAKES, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(2, state.finished);
    TEST_ASSERT_EQUAL_UINT8(4, state.solved_count); /* todos os grupos revelados */
}

void test_solved_order_preserved_across_serialize_deserialize_round_trip(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 31337);

    uint8_t g2[4];
    uint8_t g1[4];
    slots_of_group(&state, 2, g2);
    slots_of_group(&state, 1, g1);

    /* Resolve grupo 2 primeiro, grupo 1 segundo — ordem propositalmente
     * diferente da ordem numerica dos grupos. */
    state.selected_count = 4;
    memcpy(state.selected, g2, 4);
    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_CORRECT, ratimos_conexo_submit(&state, &puzzle));

    state.selected_count = 4;
    memcpy(state.selected, g1, 4);
    TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_CORRECT, ratimos_conexo_submit(&state, &puzzle));

    TEST_ASSERT_EQUAL_UINT8(2, state.solved_group_order[0]);
    TEST_ASSERT_EQUAL_UINT8(1, state.solved_group_order[1]);
    TEST_ASSERT_EQUAL_UINT8(2, state.solved_count);

    /* Round trip via memcpy cru — exatamente o que
     * ratimos_storage_save/get_game_state() faz com ratimos_game_state_t.bytes. */
    uint8_t buf[sizeof(state)];
    memcpy(buf, &state, sizeof(state));

    ratimos_conexo_state_t restored;
    memset(&restored, 0xAA, sizeof(restored));
    memcpy(&restored, buf, sizeof(restored));

    TEST_ASSERT_EQUAL_UINT8(2, restored.solved_group_order[0]);
    TEST_ASSERT_EQUAL_UINT8(1, restored.solved_group_order[1]);
    TEST_ASSERT_EQUAL_UINT8(2, restored.solved_count);
}

static void sort_bytes(uint8_t * arr, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        uint8_t key = arr[i];
        size_t j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

void test_shuffle_preserves_group_membership(void)
{
    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 42);

    uint8_t before[16];
    memcpy(before, state.tile_word, sizeof(before));

    ratimos_conexo_shuffle(&state, 909090);

    uint8_t after_shuffle[16];
    memcpy(after_shuffle, state.tile_word, sizeof(after_shuffle));

    /* Embaralhar so pode permutar slots — o CONJUNTO de word_id (e portanto
     * o grupo de cada palavra, group = word_id/4) tem que ser identico. */
    sort_bytes(before, 16);
    sort_bytes(after_shuffle, 16);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, after_shuffle, 16);
}

/* ------------------------------------------------------------------------
 * Task 2 — banco de 5 quebra-cabecas, selecao sem repeticao, vitoria/derrota
 * e progressao diaria.
 * ------------------------------------------------------------------------ */

void test_puzzle_count_is_5(void)
{
    TEST_ASSERT_EQUAL_UINT(5, ratimos_conexo_puzzle_count());
}

void test_every_puzzle_has_four_distinct_nonempty_words_per_group(void)
{
    size_t count = ratimos_conexo_puzzle_count();

    for (size_t p = 0; p < count; p++) {
        ratimos_conexo_puzzle_t puzzle;
        TEST_ASSERT_TRUE(ratimos_conexo_get_puzzle(p, &puzzle));

        for (int g = 0; g < 4; g++) {
            for (int w = 0; w < 4; w++) {
                TEST_ASSERT_TRUE(puzzle.groups[g].words[w][0] != '\0');
                for (int w2 = w + 1; w2 < 4; w2++) {
                    TEST_ASSERT_TRUE(strcmp(puzzle.groups[g].words[w], puzzle.groups[g].words[w2]) != 0);
                }
            }
        }
    }
}

void test_puzzle_pick_avoids_history_loaded_index(void)
{
    ratimos_puzzle_history_t history;
    memset(&history, 0, sizeof(history));

    size_t count = ratimos_conexo_puzzle_count();
    TEST_ASSERT_TRUE(count >= 4);

    /* Marca metade do banco como recente (indices 0 e 1) -- deixa candidatos
     * livres de sobra (>=2 de 5) para que 16 tentativas praticamente nunca
     * esgotem sem achar um, evitando um teste estatisticamente franzino.
     * `ratimos_puzzle_pick` so cai no fallback de repetir apos esgotar as
     * tentativas (limite de escala documentado em puzzle_history.h) -- essa
     * franja nao e o que este teste quer exercitar. */
    ratimos_puzzle_history_push(&history, 0);
    ratimos_puzzle_history_push(&history, 1);

    for (uint32_t seed = 1; seed <= 20; seed++) {
        size_t picked = ratimos_puzzle_pick(&history, count, seed);
        TEST_ASSERT_TRUE(picked != 0 && picked != 1);
    }
}

static void solve_all_groups(ratimos_conexo_state_t * state, const ratimos_conexo_puzzle_t * puzzle)
{
    for (uint8_t g = 0; g < 4; g++) {
        uint8_t slots[4];
        slots_of_group(state, g, slots);
        state->selected_count = 4;
        memcpy(state->selected, slots, 4);
        ratimos_conexo_submit_t r = ratimos_conexo_submit(state, puzzle);
        TEST_ASSERT_EQUAL_INT(RATIMOS_CONEXO_SUBMIT_CORRECT, r);
    }
}

void test_full_game_win_sets_finished_one_and_records_win(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 314);
    solve_all_groups(&state, &puzzle);

    TEST_ASSERT_EQUAL_UINT8(1, state.finished);
    TEST_ASSERT_EQUAL_UINT8(0, state.mistakes);
    TEST_ASSERT_EQUAL_UINT8(0, state.daily_win_recorded); /* ainda nao contado */

    ratimos_progression_state_t before;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&before));

    TEST_ASSERT_TRUE(ratimos_conexo_record_win_if_needed(&state));
    TEST_ASSERT_EQUAL_UINT8(1, state.daily_win_recorded);

    ratimos_progression_state_t after;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&after));
    TEST_ASSERT_EQUAL_UINT16((uint16_t) (before.shared_completions + 1), after.shared_completions);
    TEST_ASSERT_EQUAL_UINT8(1, after.game_exclusive_unlocked[RATIMOS_GAME_CONEXO]);
}

void test_record_win_twice_increments_shared_completions_by_one_total(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 271);
    solve_all_groups(&state, &puzzle);
    TEST_ASSERT_EQUAL_UINT8(1, state.finished);

    TEST_ASSERT_TRUE(ratimos_conexo_record_win_if_needed(&state));
    /* Reentrar numa tela ja vencida (segunda chamada) nunca soma de novo. */
    TEST_ASSERT_FALSE(ratimos_conexo_record_win_if_needed(&state));

    ratimos_progression_state_t p;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&p));
    TEST_ASSERT_EQUAL_UINT16(1, p.shared_completions); /* setUp zera antes de cada teste */
}

void test_loss_does_not_record_daily_win(void)
{
    ratimos_conexo_puzzle_t puzzle;
    load_puzzle(&puzzle);

    ratimos_conexo_state_t state;
    ratimos_conexo_start(&state, 0, 161);

    uint8_t g0[4];
    uint8_t g1[4];
    slots_of_group(&state, 0, g0);
    slots_of_group(&state, 1, g1);

    for (int i = 0; i < RATIMOS_CONEXO_MAX_MISTAKES; i++) {
        state.selected_count = 4;
        state.selected[0] = g0[0];
        state.selected[1] = g0[1];
        state.selected[2] = g1[0];
        state.selected[3] = g1[1];
        ratimos_conexo_submit(&state, &puzzle);
    }

    TEST_ASSERT_EQUAL_UINT8(2, state.finished);
    TEST_ASSERT_FALSE(ratimos_conexo_record_win_if_needed(&state));

    ratimos_progression_state_t p;
    TEST_ASSERT_TRUE(ratimos_storage_get_progression(&p));
    TEST_ASSERT_EQUAL_UINT16(0, p.shared_completions);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_submit_with_three_selected_is_rejected_incomplete);
    RUN_TEST(test_submit_all_four_correct_solves_group);
    RUN_TEST(test_submit_exactly_three_shared_is_one_away);
    RUN_TEST(test_submit_fully_wrong_counts_mistake);
    RUN_TEST(test_mistake_ceiling_reaches_finished_revealed_and_stops_recording_wins);
    RUN_TEST(test_solved_order_preserved_across_serialize_deserialize_round_trip);
    RUN_TEST(test_shuffle_preserves_group_membership);
    RUN_TEST(test_puzzle_count_is_5);
    RUN_TEST(test_every_puzzle_has_four_distinct_nonempty_words_per_group);
    RUN_TEST(test_puzzle_pick_avoids_history_loaded_index);
    RUN_TEST(test_full_game_win_sets_finished_one_and_records_win);
    RUN_TEST(test_record_win_twice_increments_shared_completions_by_one_total);
    RUN_TEST(test_loss_does_not_record_daily_win);
    return UNITY_END();
}
