/*
 * Suite do motor do sudoku (JOGOS-01) -- src/ratimos/apps/jogos/sudoku_engine.c.
 *
 * Exercita apenas o motor, que e deliberadamente livre de LVGL para nao
 * precisar de SDL/display para rodar via PlatformIO native Unity. A
 * checagem de 20 geracoes consecutivas por dificuldade e a load-bearing
 * (RESEARCH "Don't Hand-Roll": unicidade nunca pode regredir para heuristica).
 */
#include <string.h>
#include <unity.h>

#include "ratimos/apps/jogos/sudoku_engine.h"
#include "storage/content_api.h"

void setUp(void) {}
void tearDown(void) {}

/* Exemplo classico (Wikipedia "Sudoku") -- 30 pistas, provadamente
 * unicamente soluvel. Usado como fixture "unique" e como base para os
 * testes de is_solved/set_cell/cell_conflicts. */
static const uint8_t UNIQUE_PUZZLE[9][9] = {
    { 5, 3, 0, 0, 7, 0, 0, 0, 0 },
    { 6, 0, 0, 1, 9, 5, 0, 0, 0 },
    { 0, 9, 8, 0, 0, 0, 0, 6, 0 },

    { 8, 0, 0, 0, 6, 0, 0, 0, 3 },
    { 4, 0, 0, 8, 0, 3, 0, 0, 1 },
    { 7, 0, 0, 0, 2, 0, 0, 0, 6 },

    { 0, 6, 0, 0, 0, 0, 2, 8, 0 },
    { 0, 0, 0, 4, 1, 9, 0, 0, 5 },
    { 0, 0, 0, 0, 8, 0, 0, 7, 9 },
};

/* A unica solucao completa de UNIQUE_PUZZLE. */
static const uint8_t FULL_SOLUTION[9][9] = {
    { 5, 3, 4, 6, 7, 8, 9, 1, 2 },
    { 6, 7, 2, 1, 9, 5, 3, 4, 8 },
    { 1, 9, 8, 3, 4, 2, 5, 6, 7 },

    { 8, 5, 9, 7, 6, 1, 4, 2, 3 },
    { 4, 2, 6, 8, 5, 3, 7, 9, 1 },
    { 7, 1, 3, 9, 2, 4, 8, 5, 6 },

    { 9, 6, 1, 5, 3, 7, 2, 8, 4 },
    { 2, 8, 7, 4, 1, 9, 6, 3, 5 },
    { 3, 4, 5, 2, 8, 6, 1, 7, 9 },
};

/* Sub-restrito de proposito (uma unica pista): sem duvida tem mais de 2
 * solucoes completas, entao count_solutions(.., 2) deve saturar em 2. */
static const uint8_t MULTI_SOLUTION_PUZZLE[9][9] = {
    { 5, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

/* (0,8) precisa de 9 pela linha (1-8 ja usados), mas coluna 8 ja tem um 9
 * em (1,8) e a mesma caixa 3x3 tambem -- nenhum digito serve em (0,8),
 * entao a primeira celula vazia encontrada (varredura linha-a-linha) ja
 * garante zero solucoes sem depender do resto do tabuleiro. */
static const uint8_t CONTRADICTORY_PUZZLE[9][9] = {
    { 1, 2, 3, 4, 5, 6, 7, 8, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 9 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

/* ------------------------------------------------------------------------
 * ratimos_sudoku_is_valid_placement
 * ------------------------------------------------------------------------ */

void test_is_valid_placement_rejects_row_duplicate(void)
{
    uint8_t board[9][9];
    memset(board, 0, sizeof(board));
    board[0][0] = 5;
    TEST_ASSERT_FALSE(ratimos_sudoku_is_valid_placement(board, 0, 3, 5));
}

void test_is_valid_placement_rejects_col_duplicate(void)
{
    uint8_t board[9][9];
    memset(board, 0, sizeof(board));
    board[2][4] = 7;
    TEST_ASSERT_FALSE(ratimos_sudoku_is_valid_placement(board, 6, 4, 7));
}

void test_is_valid_placement_rejects_box_duplicate(void)
{
    uint8_t board[9][9];
    memset(board, 0, sizeof(board));
    board[0][0] = 9;
    TEST_ASSERT_FALSE(ratimos_sudoku_is_valid_placement(board, 2, 2, 9));
}

void test_is_valid_placement_accepts_non_conflicting_digit(void)
{
    uint8_t board[9][9];
    memset(board, 0, sizeof(board));
    board[0][0] = 5;
    TEST_ASSERT_TRUE(ratimos_sudoku_is_valid_placement(board, 4, 4, 5));
}

/* ------------------------------------------------------------------------
 * ratimos_sudoku_count_solutions
 * ------------------------------------------------------------------------ */

void test_count_solutions_returns_1_for_unique_fixture(void)
{
    uint8_t board[9][9];
    memcpy(board, UNIQUE_PUZZLE, sizeof(board));
    TEST_ASSERT_EQUAL_INT(1, ratimos_sudoku_count_solutions(board, 2));
}

void test_count_solutions_returns_2_for_multi_solution_fixture(void)
{
    uint8_t board[9][9];
    memcpy(board, MULTI_SOLUTION_PUZZLE, sizeof(board));
    TEST_ASSERT_EQUAL_INT(2, ratimos_sudoku_count_solutions(board, 2));
}

void test_count_solutions_returns_0_for_contradictory_fixture(void)
{
    uint8_t board[9][9];
    memcpy(board, CONTRADICTORY_PUZZLE, sizeof(board));
    TEST_ASSERT_EQUAL_INT(0, ratimos_sudoku_count_solutions(board, 2));
}

/* ------------------------------------------------------------------------
 * ratimos_sudoku_generate / generate_daily -- load-bearing uniqueness check
 * ------------------------------------------------------------------------ */

static int count_given(const ratimos_sudoku_state_t * state)
{
    int n = 0;
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state->given[r][c] != 0) {
                n++;
            }
        }
    }
    return n;
}

static void assert_clue_band(ratimos_sudoku_mode_t mode, int clues)
{
    switch (mode) {
        case RATIMOS_SUDOKU_FACIL:
            TEST_ASSERT_TRUE(clues >= 40 && clues <= 45);
            break;
        case RATIMOS_SUDOKU_DIFICIL:
            TEST_ASSERT_TRUE(clues >= 22 && clues <= 27);
            break;
        default:
            TEST_ASSERT_TRUE(clues >= 30 && clues <= 35);
            break;
    }
}

/* Load-bearing: 20 geracoes consecutivas por dificuldade, cada uma verificada
 * unicamente soluvel pelo proprio solver contador e dentro da banda de
 * pistas declarada. Uma regressao em qualquer banda falha aqui, alto. */
void test_generate_produces_unique_puzzle_within_clue_band_for_each_difficulty(void)
{
    ratimos_sudoku_mode_t modes[3] = { RATIMOS_SUDOKU_FACIL, RATIMOS_SUDOKU_MEDIO, RATIMOS_SUDOKU_DIFICIL };

    for (int m = 0; m < 3; m++) {
        for (int i = 0; i < 20; i++) {
            ratimos_sudoku_state_t state;
            uint32_t seed = (uint32_t) (1000 * (m + 1) + i + 1);
            TEST_ASSERT_TRUE(ratimos_sudoku_generate(&state, modes[m], seed, 64));

            uint8_t verify[9][9];
            memcpy(verify, state.given, sizeof(verify));
            TEST_ASSERT_EQUAL_INT(1, ratimos_sudoku_count_solutions(verify, 2));

            assert_clue_band(modes[m], count_given(&state));
        }
    }
}

void test_generate_daily_is_deterministic_for_same_day(void)
{
    ratimos_sudoku_state_t a, b;
    TEST_ASSERT_TRUE(ratimos_sudoku_generate_daily(&a, 12345));
    TEST_ASSERT_TRUE(ratimos_sudoku_generate_daily(&b, 12345));
    TEST_ASSERT_EQUAL_MEMORY(a.given, b.given, sizeof(a.given));
    TEST_ASSERT_EQUAL_MEMORY(a.solution, b.solution, sizeof(a.solution));
}

void test_generate_daily_differs_for_different_days(void)
{
    ratimos_sudoku_state_t a, b;
    TEST_ASSERT_TRUE(ratimos_sudoku_generate_daily(&a, 12345));
    TEST_ASSERT_TRUE(ratimos_sudoku_generate_daily(&b, 54321));
    TEST_ASSERT_TRUE(memcmp(a.given, b.given, sizeof(a.given)) != 0);
}

void test_generate_with_zero_attempts_returns_false_and_leaves_output_untouched(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0xAB, sizeof(state));
    ratimos_sudoku_state_t before;
    memcpy(&before, &state, sizeof(state));

    TEST_ASSERT_FALSE(ratimos_sudoku_generate(&state, RATIMOS_SUDOKU_FACIL, 42, 0));
    TEST_ASSERT_EQUAL_MEMORY(&before, &state, sizeof(state));
}

/* ------------------------------------------------------------------------
 * ratimos_sudoku_set_cell / cell_conflicts / is_solved
 * ------------------------------------------------------------------------ */

void test_set_cell_refuses_to_overwrite_given_clue(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    TEST_ASSERT_FALSE(ratimos_sudoku_set_cell(&state, 0, 0, 7)); /* given[0][0] == 5 */
    TEST_ASSERT_EQUAL_UINT8(0, state.filled[0][0]);
}

void test_set_cell_writes_player_entry_on_empty_given_cell(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    TEST_ASSERT_TRUE(ratimos_sudoku_set_cell(&state, 0, 2, 4));
    TEST_ASSERT_EQUAL_UINT8(4, state.filled[0][2]);
}

void test_cell_conflicts_detects_duplicate_in_row(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    ratimos_sudoku_set_cell(&state, 0, 2, 3); /* duplica given[0][1] == 3 */
    TEST_ASSERT_TRUE(ratimos_sudoku_cell_conflicts(&state, 0, 2));
}

void test_is_solved_true_when_fully_filled_and_valid(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0) {
                state.filled[r][c] = FULL_SOLUTION[r][c];
            }
        }
    }

    TEST_ASSERT_TRUE(ratimos_sudoku_is_solved(&state));
}

void test_is_solved_false_when_incomplete(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0) {
                state.filled[r][c] = FULL_SOLUTION[r][c];
            }
        }
    }
    state.filled[0][2] = 0; /* given[0][2] == 0, agora fica vazio de novo */

    TEST_ASSERT_FALSE(ratimos_sudoku_is_solved(&state));
}

void test_is_solved_false_when_conflict_present(void)
{
    ratimos_sudoku_state_t state;
    memset(&state, 0, sizeof(state));
    memcpy(state.given, UNIQUE_PUZZLE, sizeof(state.given));

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0) {
                state.filled[r][c] = FULL_SOLUTION[r][c];
            }
        }
    }
    state.filled[0][2] = 3; /* duplica given[0][1] == 3 na mesma linha */

    TEST_ASSERT_FALSE(ratimos_sudoku_is_solved(&state));
}

/* ------------------------------------------------------------------------
 * Task 3: caminho de vitoria (is_solved a partir da propria solucao gerada)
 * e round-trip do estado atraves do blob de save.
 * ------------------------------------------------------------------------ */

void test_is_solved_true_when_filled_from_own_generated_solution(void)
{
    ratimos_sudoku_state_t state;
    TEST_ASSERT_TRUE(ratimos_sudoku_generate(&state, RATIMOS_SUDOKU_FACIL, 777, 64));

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0) {
                TEST_ASSERT_TRUE(ratimos_sudoku_set_cell(&state, r, c, state.solution[r][c]));
            }
        }
    }

    TEST_ASSERT_TRUE(ratimos_sudoku_is_solved(&state));
}

void test_is_solved_false_when_one_digit_wrong_after_filling_from_solution(void)
{
    ratimos_sudoku_state_t state;
    TEST_ASSERT_TRUE(ratimos_sudoku_generate(&state, RATIMOS_SUDOKU_FACIL, 778, 64));

    int wrong_r = -1, wrong_c = -1;
    for (int r = 0; r < 9 && wrong_r < 0; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0) {
                uint8_t correct = state.solution[r][c];
                uint8_t wrong = (uint8_t) (correct % 9) + 1; /* sempre diferente de `correct` */
                TEST_ASSERT_TRUE(ratimos_sudoku_set_cell(&state, r, c, wrong));
                wrong_r = r;
                wrong_c = c;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(wrong_r >= 0); /* fixture sempre tem pelo menos uma celula vazia */

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (state.given[r][c] == 0 && !(r == wrong_r && c == wrong_c)) {
                TEST_ASSERT_TRUE(ratimos_sudoku_set_cell(&state, r, c, state.solution[r][c]));
            }
        }
    }

    TEST_ASSERT_FALSE(ratimos_sudoku_is_solved(&state));
}

/* Round-trip atraves do blob opaco da Storage API (mesmo shape que
 * sudoku.c's persist_state()/load_or_start_state() usam de verdade) --
 * confirma que solved/mode/daily_win_recorded sobrevivem ao serializar e
 * desserializar, sem precisar de LVGL nem tocar disco de verdade. */
void test_state_round_trips_through_game_state_blob(void)
{
    ratimos_sudoku_state_t original;
    memset(&original, 0, sizeof(original));
    memcpy(original.given, UNIQUE_PUZZLE, sizeof(original.given));
    original.mode = RATIMOS_SUDOKU_DIARIO;
    original.solved = 1;
    original.daily_win_recorded = 1;

    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &original, sizeof(original));
    blob.used = sizeof(original);

    ratimos_sudoku_state_t restored;
    memset(&restored, 0, sizeof(restored));
    TEST_ASSERT_EQUAL_size_t(sizeof(original), blob.used);
    memcpy(&restored, blob.bytes, sizeof(restored));

    TEST_ASSERT_EQUAL_UINT8(1, restored.solved);
    TEST_ASSERT_EQUAL_UINT8(1, restored.daily_win_recorded);
    TEST_ASSERT_EQUAL_INT(RATIMOS_SUDOKU_DIARIO, restored.mode);
    TEST_ASSERT_EQUAL_MEMORY(original.given, restored.given, sizeof(original.given));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_is_valid_placement_rejects_row_duplicate);
    RUN_TEST(test_is_valid_placement_rejects_col_duplicate);
    RUN_TEST(test_is_valid_placement_rejects_box_duplicate);
    RUN_TEST(test_is_valid_placement_accepts_non_conflicting_digit);

    RUN_TEST(test_count_solutions_returns_1_for_unique_fixture);
    RUN_TEST(test_count_solutions_returns_2_for_multi_solution_fixture);
    RUN_TEST(test_count_solutions_returns_0_for_contradictory_fixture);

    RUN_TEST(test_generate_produces_unique_puzzle_within_clue_band_for_each_difficulty);
    RUN_TEST(test_generate_daily_is_deterministic_for_same_day);
    RUN_TEST(test_generate_daily_differs_for_different_days);
    RUN_TEST(test_generate_with_zero_attempts_returns_false_and_leaves_output_untouched);

    RUN_TEST(test_set_cell_refuses_to_overwrite_given_clue);
    RUN_TEST(test_set_cell_writes_player_entry_on_empty_given_cell);
    RUN_TEST(test_cell_conflicts_detects_duplicate_in_row);
    RUN_TEST(test_is_solved_true_when_fully_filled_and_valid);
    RUN_TEST(test_is_solved_false_when_incomplete);
    RUN_TEST(test_is_solved_false_when_conflict_present);

    RUN_TEST(test_is_solved_true_when_filled_from_own_generated_solution);
    RUN_TEST(test_is_solved_false_when_one_digit_wrong_after_filling_from_solution);
    RUN_TEST(test_state_round_trips_through_game_state_blob);

    return UNITY_END();
}
