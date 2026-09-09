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

void setUp(void) {}
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
    return UNITY_END();
}
