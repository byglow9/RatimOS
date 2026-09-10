/*
 * Suite do motor do cruzadinha (JOGOS-04) --
 * src/ratimos/apps/jogos/cruzadinha_engine.c.
 *
 * Os dois testes de integridade do banco (test_bank_no_orphan_cells e
 * test_bank_generator_numbering_matches_engine_numbering) sao os
 * load-bearing: provam que os 5 quebra-cabecas compilados por
 * tools/generate_crossword.py sao internamente consistentes contra a
 * numeracao que o proprio motor deriva da geometria, independente do que o
 * gerador gravou em clue_number.
 */
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "ratimos/apps/jogos/cruzadinha_engine.h"
#include "ratimos/apps/jogos/cruzadinha_puzzles.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------------
 * Fixture de teste: um quebra-cabeca 5x5 pequeno, montado a mao (nao vem do
 * banco compilado), pensado especificamente para exercitar a regra de
 * numeracao: CAT (horizontal) e COW (vertical) comecam na MESMA celula
 * (0,0) e compartilham o numero 1; TIE (vertical) comeca em (0,2), que so e
 * inicio de entrada vertical, e recebe o numero 2.
 *
 *   C A T
 *   O . I
 *   W . E
 * ------------------------------------------------------------------------ */
static void make_fixture_puzzle(ratimos_cruzadinha_puzzle_t * pz)
{
    memset(pz, 0, sizeof(*pz));
    snprintf(pz->id, sizeof(pz->id), "fixture");
    pz->grid_w = 5;
    pz->grid_h = 5;
    pz->word_count = 3;

    /* words[0] = CAT, horizontal, (0,0..2) */
    pz->words[0].row = 0;
    pz->words[0].col = 0;
    pz->words[0].length = 3;
    pz->words[0].is_across = true;
    snprintf(pz->words[0].clue_text, sizeof(pz->words[0].clue_text), "felino domestico");
    snprintf(pz->words[0].answer, sizeof(pz->words[0].answer), "CAT");

    /* words[1] = COW, vertical, (0,0..2) */
    pz->words[1].row = 0;
    pz->words[1].col = 0;
    pz->words[1].length = 3;
    pz->words[1].is_across = false;
    snprintf(pz->words[1].clue_text, sizeof(pz->words[1].clue_text), "bovino que da leite");
    snprintf(pz->words[1].answer, sizeof(pz->words[1].answer), "COW");

    /* words[2] = TIE, vertical, (0,2..4) */
    pz->words[2].row = 0;
    pz->words[2].col = 2;
    pz->words[2].length = 3;
    pz->words[2].is_across = false;
    snprintf(pz->words[2].clue_text, sizeof(pz->words[2].clue_text), "gravata, em ingles");
    snprintf(pz->words[2].answer, sizeof(pz->words[2].answer), "TIE");
}

/* ------------------------------------------------------------------------
 * ratimos_cruzadinha_number_grid -- numeracao contra o fixture conhecido.
 * ------------------------------------------------------------------------ */

void test_number_grid_fixture_assigns_expected_numbers(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);

    ratimos_cruzadinha_grid_t grid;
    ratimos_cruzadinha_number_grid(&pz, &grid);

    /* (0,0) inicia CAT (horizontal) E COW (vertical) -- um unico numero
     * compartilhado pelas duas. */
    TEST_ASSERT_EQUAL_UINT8(1, grid.numbers[0][0]);

    /* (0,2) so inicia TIE (vertical) -- proximo numero sequencial. */
    TEST_ASSERT_EQUAL_UINT8(2, grid.numbers[0][2]);

    /* Nenhuma outra celula de letra recebe numero (continuacao de palavra,
     * nao inicio). */
    TEST_ASSERT_EQUAL_UINT8(0, grid.numbers[0][1]);
    TEST_ASSERT_EQUAL_UINT8(0, grid.numbers[1][0]);
    TEST_ASSERT_EQUAL_UINT8(0, grid.numbers[1][2]);
    TEST_ASSERT_EQUAL_UINT8(0, grid.numbers[2][0]);
    TEST_ASSERT_EQUAL_UINT8(0, grid.numbers[2][2]);

    /* Celulas fora do desenho (nunca pintadas por nenhuma palavra) nao sao
     * celula de letra. */
    TEST_ASSERT_EQUAL_UINT8(0, grid.is_cell[4][4]);
    TEST_ASSERT_EQUAL_UINT8(1, grid.is_cell[0][0]);
    TEST_ASSERT_EQUAL_UINT8(1, grid.is_cell[0][2]);
}

/* ------------------------------------------------------------------------
 * ratimos_cruzadinha_entry_at
 * ------------------------------------------------------------------------ */

void test_entry_at_returns_across_and_down_at_shared_start_cell(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);

    uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
    uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
    TEST_ASSERT_TRUE(ratimos_cruzadinha_entry_at(&pz, 0, 0, &across, &down));
    TEST_ASSERT_EQUAL_UINT8(0, across); /* CAT */
    TEST_ASSERT_EQUAL_UINT8(1, down);   /* COW */
}

void test_entry_at_returns_no_entry_for_missing_direction(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);

    /* (1,0) -- so pertence a COW (vertical); nao ha entrada horizontal
     * comecando ali. */
    uint8_t across = 0;
    uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
    TEST_ASSERT_TRUE(ratimos_cruzadinha_entry_at(&pz, 1, 0, &across, &down));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_CRUZADINHA_NO_ENTRY, across);
    TEST_ASSERT_EQUAL_UINT8(1, down); /* COW cobre (1,0) mesmo nao comecando ali */
}

void test_entry_at_returns_false_out_of_bounds(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);

    uint8_t across = 0;
    uint8_t down = 0;
    TEST_ASSERT_FALSE(ratimos_cruzadinha_entry_at(&pz, 99, 99, &across, &down));
}

/* ------------------------------------------------------------------------
 * ratimos_cruzadinha_get_puzzle -- indice fora do banco.
 * ------------------------------------------------------------------------ */

void test_get_puzzle_out_of_range_returns_false_and_leaves_out_untouched(void)
{
    ratimos_cruzadinha_puzzle_t out;
    memset(&out, 0xAB, sizeof(out));
    ratimos_cruzadinha_puzzle_t sentinel = out;

    bool ok = ratimos_cruzadinha_get_puzzle(ratimos_cruzadinha_puzzle_count() + 100, &out);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_MEMORY(&sentinel, &out, sizeof(out));
}

/* ------------------------------------------------------------------------
 * ratimos_cruzadinha_set_letter
 * ------------------------------------------------------------------------ */

void test_set_letter_rejects_coordinate_that_is_not_a_letter_cell(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);
    ratimos_cruzadinha_state_t st;
    memset(&st, 0, sizeof(st));

    TEST_ASSERT_FALSE(ratimos_cruzadinha_set_letter(&pz, &st, 4, 4, 'X'));
    TEST_ASSERT_EQUAL_INT8(0, st.entered[4][4]);
}

void test_set_letter_rejects_non_letter_character(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);
    ratimos_cruzadinha_state_t st;
    memset(&st, 0, sizeof(st));

    TEST_ASSERT_FALSE(ratimos_cruzadinha_set_letter(&pz, &st, 0, 0, '5'));
    TEST_ASSERT_FALSE(ratimos_cruzadinha_set_letter(&pz, &st, 0, 0, '#'));
    TEST_ASSERT_EQUAL_INT8(0, st.entered[0][0]);
}

void test_set_letter_accepts_letter_normalizes_case_and_clears_with_space(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);
    ratimos_cruzadinha_state_t st;
    memset(&st, 0, sizeof(st));

    TEST_ASSERT_TRUE(ratimos_cruzadinha_set_letter(&pz, &st, 0, 0, 'c'));
    TEST_ASSERT_EQUAL_INT8('C', st.entered[0][0]);

    TEST_ASSERT_TRUE(ratimos_cruzadinha_set_letter(&pz, &st, 0, 0, ' '));
    TEST_ASSERT_EQUAL_INT8(0, st.entered[0][0]);
}

/* ------------------------------------------------------------------------
 * ratimos_cruzadinha_is_complete / entry_is_solved / next_unsolved
 * ------------------------------------------------------------------------ */

static void fill_word(ratimos_cruzadinha_state_t * st, const ratimos_cruzadinha_word_t * w)
{
    for (uint8_t k = 0; k < w->length; k++) {
        uint8_t r = w->is_across ? w->row : (uint8_t) (w->row + k);
        uint8_t c = w->is_across ? (uint8_t) (w->col + k) : w->col;
        st->entered[r][c] = w->answer[k];
    }
}

void test_is_complete_true_only_when_every_entry_matches(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);
    ratimos_cruzadinha_state_t st;
    memset(&st, 0, sizeof(st));

    TEST_ASSERT_FALSE(ratimos_cruzadinha_is_complete(&pz, &st));

    fill_word(&st, &pz.words[0]);
    fill_word(&st, &pz.words[1]);
    fill_word(&st, &pz.words[2]);
    TEST_ASSERT_TRUE(ratimos_cruzadinha_is_complete(&pz, &st));

    /* Corrompe uma unica letra -- deixa de estar completo. */
    st.entered[0][0] = 'X';
    TEST_ASSERT_FALSE(ratimos_cruzadinha_is_complete(&pz, &st));
}

void test_next_unsolved_cycles_in_clue_number_order_and_wraps(void)
{
    ratimos_cruzadinha_puzzle_t pz;
    make_fixture_puzzle(&pz);
    ratimos_cruzadinha_state_t st;
    memset(&st, 0, sizeof(st));

    /* Nada resolvido: comecando de RATIMOS_CRUZADINHA_NO_ENTRY encontra a
     * primeira entrada (indice 0, CAT). */
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_cruzadinha_next_unsolved(&pz, &st, RATIMOS_CRUZADINHA_NO_ENTRY));

    /* Resolve CAT (0) e TIE (2), deixa COW (1) pendente. Comecando a busca
     * a partir de TIE (2) -- depois do fim do array -- precisa DAR A VOLTA
     * e achar COW (1) mesmo estando antes de 2 na ordem do array. */
    fill_word(&st, &pz.words[0]);
    fill_word(&st, &pz.words[2]);
    TEST_ASSERT_EQUAL_UINT8(1, ratimos_cruzadinha_next_unsolved(&pz, &st, 2));

    /* Tudo resolvido: nenhuma entrada pendente. */
    fill_word(&st, &pz.words[1]);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_CRUZADINHA_NO_ENTRY, ratimos_cruzadinha_next_unsolved(&pz, &st, 0));
}

/* ------------------------------------------------------------------------
 * Integridade do banco compilado (tools/generate_crossword.py) -- os testes
 * load-bearing desta suite.
 * ------------------------------------------------------------------------ */

void test_bank_has_at_least_five_puzzles(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(5, ratimos_cruzadinha_puzzle_count());
}

void test_bank_no_orphan_cells(void)
{
    size_t count = ratimos_cruzadinha_puzzle_count();
    for (size_t p = 0; p < count; p++) {
        ratimos_cruzadinha_puzzle_t pz;
        TEST_ASSERT_TRUE(ratimos_cruzadinha_get_puzzle(p, &pz));

        ratimos_cruzadinha_grid_t grid;
        ratimos_cruzadinha_number_grid(&pz, &grid);

        for (uint8_t r = 0; r < pz.grid_h; r++) {
            for (uint8_t c = 0; c < pz.grid_w; c++) {
                if (!grid.is_cell[r][c]) {
                    continue;
                }
                uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
                uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
                TEST_ASSERT_TRUE(ratimos_cruzadinha_entry_at(&pz, r, c, &across, &down));
                TEST_ASSERT_TRUE_MESSAGE(
                    across != RATIMOS_CRUZADINHA_NO_ENTRY || down != RATIMOS_CRUZADINHA_NO_ENTRY,
                    "celula orfa encontrada no banco compilado");
            }
        }
    }
}

void test_bank_crossing_letters_agree(void)
{
    size_t count = ratimos_cruzadinha_puzzle_count();
    for (size_t p = 0; p < count; p++) {
        ratimos_cruzadinha_puzzle_t pz;
        TEST_ASSERT_TRUE(ratimos_cruzadinha_get_puzzle(p, &pz));

        for (uint8_t r = 0; r < pz.grid_h; r++) {
            for (uint8_t c = 0; c < pz.grid_w; c++) {
                uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
                uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
                ratimos_cruzadinha_entry_at(&pz, r, c, &across, &down);
                if (across == RATIMOS_CRUZADINHA_NO_ENTRY || down == RATIMOS_CRUZADINHA_NO_ENTRY) {
                    continue; /* nao e celula de intersecao */
                }

                const ratimos_cruzadinha_word_t * wa = &pz.words[across];
                const ratimos_cruzadinha_word_t * wd = &pz.words[down];
                char letter_a = wa->answer[c - wa->col];
                char letter_d = wd->answer[r - wd->row];
                TEST_ASSERT_EQUAL_INT8_MESSAGE(letter_a, letter_d,
                                                "letra de intersecao diverge entre across e down");
            }
        }
    }
}

void test_bank_generator_numbering_matches_engine_numbering(void)
{
    size_t count = ratimos_cruzadinha_puzzle_count();
    for (size_t p = 0; p < count; p++) {
        ratimos_cruzadinha_puzzle_t pz;
        TEST_ASSERT_TRUE(ratimos_cruzadinha_get_puzzle(p, &pz));

        ratimos_cruzadinha_grid_t grid;
        ratimos_cruzadinha_number_grid(&pz, &grid);

        for (uint8_t i = 0; i < pz.word_count; i++) {
            const ratimos_cruzadinha_word_t * w = &pz.words[i];
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(w->clue_number, grid.numbers[w->row][w->col],
                                             "numeracao do gerador diverge da numeracao do motor");
        }
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_number_grid_fixture_assigns_expected_numbers);

    RUN_TEST(test_entry_at_returns_across_and_down_at_shared_start_cell);
    RUN_TEST(test_entry_at_returns_no_entry_for_missing_direction);
    RUN_TEST(test_entry_at_returns_false_out_of_bounds);

    RUN_TEST(test_get_puzzle_out_of_range_returns_false_and_leaves_out_untouched);

    RUN_TEST(test_set_letter_rejects_coordinate_that_is_not_a_letter_cell);
    RUN_TEST(test_set_letter_rejects_non_letter_character);
    RUN_TEST(test_set_letter_accepts_letter_normalizes_case_and_clears_with_space);

    RUN_TEST(test_is_complete_true_only_when_every_entry_matches);
    RUN_TEST(test_next_unsolved_cycles_in_clue_number_order_and_wraps);

    RUN_TEST(test_bank_has_at_least_five_puzzles);
    RUN_TEST(test_bank_no_orphan_cells);
    RUN_TEST(test_bank_crossing_letters_agree);
    RUN_TEST(test_bank_generator_numbering_matches_engine_numbering);

    return UNITY_END();
}
