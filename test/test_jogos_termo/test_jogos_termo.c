/*
 * Suite do motor do termo/dueto/quarteto (JOGOS-03) --
 * src/ratimos/apps/jogos/termo_engine.c.
 *
 * Os 4 primeiros testes (letra duplicada) sao os load-bearing: um clone de
 * Wordle que credita de mais uma letra duplicada e o bug mais comum
 * documentado pelo RESEARCH -- ficam agrupados aqui de proposito, escritos
 * antes de qualquer outro teste, para que uma regressao seja obvia.
 */
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "ratimos/apps/jogos/termo_engine.h"
#include "ratimos/apps/jogos/termo_words.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------------
 * ratimos_termo_score_guess -- regra duas-passadas duplicate-safe.
 * ------------------------------------------------------------------------ */

/* Bullet do plano: "Scoring `posse` against `posse` marks all five
 * positions correct." */
void test_score_guess_exact_match_all_correct(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("posse", "posse", fb);

    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_CORRECT, fb[i]);
    }
}

/* Duplicate #1 -- o bug classico de over-crediting: a resposta tem 'a'
 * exatamente uma vez (posicao 0), o palpite tem 'a' duas vezes (posicoes 0
 * e 1). A posicao 0 acerta e consome a unica contagem de 'a' -- a segunda
 * 'a' do palpite (posicao 1) NAO pode virar "presente", tem que ficar
 * ausente. */
void test_score_guess_duplicate_letter_matched_correct_marks_extra_absent(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("aaxyz", "abcde", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_CORRECT, fb[0]);   /* 'a' bate na posicao 0 */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[1]);    /* 2o 'a' -- sem contagem sobrando */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[2]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[4]);
}

/* Duplicate #2 -- resposta com 'a' duas vezes (posicoes 1 e 3), palpite com
 * 'a' duas vezes (posicoes 0 e 2), nenhuma delas na posicao certa: as DUAS
 * devem virar "presente" (a contagem de 2 cobre as duas). */
void test_score_guess_duplicate_letters_both_misplaced_marks_both_present(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("axaxx", "bacad", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[0]);   /* 'a' presente, posicao errada */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[1]);    /* 'x' nao esta em "bacad" */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[2]);   /* 2o 'a' -- ainda ha contagem */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[4]);
}

/* Duplicate #3 -- resposta com 'a' exatamente uma vez (posicao 2), palpite
 * com 'a' tres vezes (posicoes 0, 1, 2): exatamente uma marca
 * presente/correta (a que bate a posicao), o resto ausente. */
void test_score_guess_triple_duplicate_letter_one_in_answer_marks_one_hit(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("aaaxy", "bcade", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[0]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[1]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_CORRECT, fb[2]);   /* unica 'a' da resposta, na posicao certa */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[4]);

    int hits = 0;
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        if (fb[i] == RATIMOS_TERMO_FEEDBACK_CORRECT || fb[i] == RATIMOS_TERMO_FEEDBACK_PRESENT) {
            hits++;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, hits);
}

/* Duplicate #4 -- resposta com 'a' duas vezes (posicoes 0 e 3), palpite com
 * 'd' uma vez e 'a' duas vezes, NENHUMA na posicao certa: as duas 'a' viram
 * presente e o 'd' (presente na resposta, posicao 4) tambem vira presente
 * -- confirma que a passada 2 nao se limita a uma unica letra duplicada por
 * vez. */
void test_score_guess_duplicate_letter_mixed_present_only(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("daaxx", "abcad", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[0]);   /* 'd' presente (resposta[4]) */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[1]);   /* 1o 'a' presente */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[2]);   /* 2o 'a' presente -- ainda ha contagem */
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[4]);
}

/* ------------------------------------------------------------------------
 * Tabela de modos.
 * ------------------------------------------------------------------------ */

void test_mode_table_board_counts_and_max_tries(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, ratimos_termo_board_count(RATIMOS_TERMO_MODE_TERMO));
    TEST_ASSERT_EQUAL_UINT8(2, ratimos_termo_board_count(RATIMOS_TERMO_MODE_DUETO));
    TEST_ASSERT_EQUAL_UINT8(4, ratimos_termo_board_count(RATIMOS_TERMO_MODE_QUARTETO));

    TEST_ASSERT_EQUAL_UINT8(6, ratimos_termo_max_tries(RATIMOS_TERMO_MODE_TERMO));
    TEST_ASSERT_EQUAL_UINT8(7, ratimos_termo_max_tries(RATIMOS_TERMO_MODE_DUETO));
    TEST_ASSERT_EQUAL_UINT8(9, ratimos_termo_max_tries(RATIMOS_TERMO_MODE_QUARTETO));
}

/* ------------------------------------------------------------------------
 * ratimos_termo_start_daily -- sorteio diario determinista.
 * ------------------------------------------------------------------------ */

void test_start_daily_same_day_same_mode_identical_answers(void)
{
    ratimos_termo_state_t a, b;
    ratimos_termo_start_daily(&a, RATIMOS_TERMO_MODE_QUARTETO, 12345);
    ratimos_termo_start_daily(&b, RATIMOS_TERMO_MODE_QUARTETO, 12345);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_STRING(a.boards[i].answer, b.boards[i].answer);
    }
}

void test_start_daily_same_day_different_mode_different_answers(void)
{
    ratimos_termo_state_t termo_state, dueto_state;
    ratimos_termo_start_daily(&termo_state, RATIMOS_TERMO_MODE_TERMO, 777);
    ratimos_termo_start_daily(&dueto_state, RATIMOS_TERMO_MODE_DUETO, 777);

    TEST_ASSERT_NOT_EQUAL(0, strcmp(termo_state.boards[0].answer, dueto_state.boards[0].answer));
}

void test_start_daily_dueto_answers_distinct(void)
{
    ratimos_termo_state_t st;
    ratimos_termo_start_daily(&st, RATIMOS_TERMO_MODE_DUETO, 42);

    TEST_ASSERT_NOT_EQUAL(0, strcmp(st.boards[0].answer, st.boards[1].answer));
}

void test_start_daily_quarteto_answers_distinct(void)
{
    ratimos_termo_state_t st;
    ratimos_termo_start_daily(&st, RATIMOS_TERMO_MODE_QUARTETO, 9001);

    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            TEST_ASSERT_NOT_EQUAL(0, strcmp(st.boards[i].answer, st.boards[j].answer));
        }
    }
}

/* ------------------------------------------------------------------------
 * ratimos_termo_submit -- palpite compartilhado, tentativas, vitoria/derrota.
 * ------------------------------------------------------------------------ */

static void make_termo_state(ratimos_termo_state_t * st, const char * answer)
{
    memset(st, 0, sizeof(*st));
    st->mode = RATIMOS_TERMO_MODE_TERMO;
    st->board_count = ratimos_termo_board_count(RATIMOS_TERMO_MODE_TERMO);
    st->max_tries = ratimos_termo_max_tries(RATIMOS_TERMO_MODE_TERMO);
    snprintf(st->boards[0].answer, sizeof(st->boards[0].answer), "%s", answer);
}

static void make_dueto_state(ratimos_termo_state_t * st, const char * answer0, const char * answer1)
{
    memset(st, 0, sizeof(*st));
    st->mode = RATIMOS_TERMO_MODE_DUETO;
    st->board_count = ratimos_termo_board_count(RATIMOS_TERMO_MODE_DUETO);
    st->max_tries = ratimos_termo_max_tries(RATIMOS_TERMO_MODE_DUETO);
    snprintf(st->boards[0].answer, sizeof(st->boards[0].answer), "%s", answer0);
    snprintf(st->boards[1].answer, sizeof(st->boards[1].answer), "%s", answer1);
}

void test_submit_rejects_guess_not_in_accepted_list_without_consuming_try(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    TEST_ASSERT_FALSE(ratimos_termo_submit(&st, "zzzzz"));
    TEST_ASSERT_EQUAL_UINT8(0, st.tries_used);
    TEST_ASSERT_EQUAL_UINT8(0, st.finished);
}

void test_submit_consumes_exactly_one_try_per_guess(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "carta"));
    TEST_ASSERT_EQUAL_UINT8(1, st.tries_used);
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].guesses_made);
    TEST_ASSERT_EQUAL_UINT8(0, st.finished); /* errou, mas ainda ha tentativas */
}

void test_submit_wins_when_all_boards_solved(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "posse"));
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].solved);
    TEST_ASSERT_EQUAL_UINT8(1, st.finished);
}

void test_submit_loses_when_tries_exhausted_with_unsolved_board(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    const char * wrong_guesses[6] = { "carta", "casal", "trigo", "verde", "preto", "praia" };
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_TRUE(ratimos_termo_submit(&st, wrong_guesses[i]));
    }

    TEST_ASSERT_EQUAL_UINT8(6, st.tries_used);
    TEST_ASSERT_EQUAL_UINT8(0, st.boards[0].solved);
    TEST_ASSERT_EQUAL_UINT8(2, st.finished);
}

void test_submit_rejects_after_finished(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");
    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "posse"));
    TEST_ASSERT_EQUAL_UINT8(1, st.finished);

    TEST_ASSERT_FALSE(ratimos_termo_submit(&st, "carta"));
    TEST_ASSERT_EQUAL_UINT8(1, st.tries_used); /* nao mudou */
}

/* Aplica o mesmo palpite a todo board ainda nao resolvido e a NENHUM board
 * ja resolvido -- um board resolvido congela (para de ganhar linha de
 * feedback) enquanto o outro continua jogando. */
void test_submit_applies_shared_guess_to_unsolved_boards_only(void)
{
    ratimos_termo_state_t st;
    make_dueto_state(&st, "posse", "carta");

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "posse"));
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].solved);   /* board 0 resolvido */
    TEST_ASSERT_EQUAL_UINT8(0, st.boards[1].solved);   /* board 1 ainda jogando */
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].guesses_made);
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[1].guesses_made);
    TEST_ASSERT_EQUAL_UINT8(1, st.tries_used);
    TEST_ASSERT_EQUAL_UINT8(0, st.finished);

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "carta"));
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].guesses_made); /* board 0 congelado -- nao ganhou nova linha */
    TEST_ASSERT_EQUAL_UINT8(2, st.boards[1].guesses_made);
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[1].solved);
    TEST_ASSERT_EQUAL_UINT8(2, st.tries_used);
    TEST_ASSERT_EQUAL_UINT8(1, st.finished); /* ambos resolvidos agora */
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_score_guess_exact_match_all_correct);
    RUN_TEST(test_score_guess_duplicate_letter_matched_correct_marks_extra_absent);
    RUN_TEST(test_score_guess_duplicate_letters_both_misplaced_marks_both_present);
    RUN_TEST(test_score_guess_triple_duplicate_letter_one_in_answer_marks_one_hit);
    RUN_TEST(test_score_guess_duplicate_letter_mixed_present_only);

    RUN_TEST(test_mode_table_board_counts_and_max_tries);

    RUN_TEST(test_start_daily_same_day_same_mode_identical_answers);
    RUN_TEST(test_start_daily_same_day_different_mode_different_answers);
    RUN_TEST(test_start_daily_dueto_answers_distinct);
    RUN_TEST(test_start_daily_quarteto_answers_distinct);

    RUN_TEST(test_submit_rejects_guess_not_in_accepted_list_without_consuming_try);
    RUN_TEST(test_submit_consumes_exactly_one_try_per_guess);
    RUN_TEST(test_submit_wins_when_all_boards_solved);
    RUN_TEST(test_submit_loses_when_tries_exhausted_with_unsolved_board);
    RUN_TEST(test_submit_rejects_after_finished);
    RUN_TEST(test_submit_applies_shared_guess_to_unsolved_boards_only);

    return UNITY_END();
}
