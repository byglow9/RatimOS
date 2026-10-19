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

/* UAT 02.1-17 (caso da usuaria): ARARA contra PASTA. A resposta tem 2 'a';
 * o 'a' final acerta a posicao e consome uma contagem, entao so' o 1o 'a' do
 * palpite sobra como presente; o 2o 'a' e os 2 'r' ficam ausentes. */
void test_score_guess_arara_against_pasta_two_pass(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("arara", "pasta", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[0]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[1]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[2]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_CORRECT, fb[4]);
}

/* ARARA contra CARTA: 'r' existe uma vez, so' o 1o 'r' do palpite e' presente. */
void test_score_guess_arara_against_carta_two_pass(void)
{
    ratimos_termo_feedback_t fb[RATIMOS_TERMO_WORD_LEN];
    ratimos_termo_score_guess("arara", "carta", fb);

    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[0]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_PRESENT, fb[1]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[2]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_ABSENT, fb[3]);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_FEEDBACK_CORRECT, fb[4]);
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

/*
 * Fix do checkpoint 02.1-14: "peste" (palavra comum) era recusada porque o
 * bloqueio listas/negativas do fserb/pt-br tambem cortava a lista de
 * PALPITES. O bloqueio agora vale so' pro pool de respostas: palavras
 * bloqueadas sao palpites validos, mas nunca resposta do dia.
 */
void test_blocklisted_common_words_are_accepted_guesses_but_never_answers(void)
{
    const char * blocked[] = { "peste", "bagre", "burro", "droga", "fenda" };
    for (size_t i = 0; i < sizeof(blocked) / sizeof(blocked[0]); i++) {
        TEST_ASSERT_TRUE_MESSAGE(ratimos_termo_is_accepted_guess(blocked[i]), blocked[i]);
        for (size_t a = 0; a < ratimos_termo_answer_count(); a++) {
            TEST_ASSERT_TRUE_MESSAGE(strcmp(ratimos_termo_answer_at(a), blocked[i]) != 0, blocked[i]);
        }
    }

    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");
    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "peste"));
    TEST_ASSERT_EQUAL_UINT8(1, st.tries_used);
}

/* Estado de uma letra (maiuscula ou minuscula) no vetor de 26. */
static uint8_t key(const uint8_t ks[26], char c)
{
    return ks[(c | 0x20) - 'a'];
}

/*
 * Fix de checkpoint 02.1-14: teclado reflete o estado das letras (Termo
 * real). posse x RESTO, TESTE: S/E verdes (E repetida em TESTE sai cinza na
 * 1a posicao mas a letra existe -> continua verde, nunca desabilitada),
 * O amarela, R/T fora (desabilitadas), P/Z nunca tentadas.
 */
void test_key_states_termo_correct_beats_present_and_absent_letters(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");
    uint8_t ks[26];

    ratimos_termo_key_states(&st, ks);
    for (int i = 0; i < 26; i++) {
        TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_UNUSED, ks[i]);
    }

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "resto"));
    ratimos_termo_key_states(&st, ks);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'E'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'S'));

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "teste"));
    ratimos_termo_key_states(&st, ks);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'R'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'T'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'O'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'S'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'E'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_UNUSED, key(ks, 'P'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_UNUSED, key(ks, 'Z'));
}

/*
 * Dueto: so' contam os boards ainda em jogo. Antes de resolver "posse", nada
 * fica desabilitado (cada letra existe em algum board). Depois, so' "carta"
 * conta: E/S/O/P (fora de carta) viram ABSENT -- S inclusive, que era verde
 * no board ja resolvido -- e R/T mantem o tom vindo de carta.
 */
void test_key_states_dueto_only_count_boards_still_in_play(void)
{
    ratimos_termo_state_t st;
    make_dueto_state(&st, "posse", "carta");
    uint8_t ks[26];

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "resto"));
    ratimos_termo_key_states(&st, ks);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'R'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'E'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'S'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'T'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'O'));

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "posse"));
    TEST_ASSERT_EQUAL_UINT8(1, st.boards[0].solved);
    ratimos_termo_key_states(&st, ks);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_PRESENT, key(ks, 'R'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'T'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'S'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'E'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'O'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'P'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_UNUSED, key(ks, 'C'));
}

/* Estado de tecla do caso ARARA x PASTA: 'a' verde (o acerto vence o presente),
 * 'r' cinza (descartada). */
void test_key_states_arara_against_pasta(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "pasta");
    uint8_t ks[26];

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "arara"));
    ratimos_termo_key_states(&st, ks);
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_CORRECT, key(ks, 'A'));
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_KEY_ABSENT, key(ks, 'R'));
}

/*
 * UAT 02.1-17: NENHUMA letra e' pre-colocada (o pre-preenchimento do 2c292eb
 * foi rejeitado). Depois de um palpite com letra verde, a linha de entrada
 * continua exatamente como a jogadora deixou -- o motor nunca escreve nela.
 */
void test_submit_never_prefills_the_current_guess(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "resto")); /* S verde na posicao 2 */
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN + 1; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, (uint8_t) st.current_guess[i]);
    }

    /* E o motor nao mexe no que ja' estava la' (quem zera e' a tela). */
    memset(st.current_guess, 0, sizeof(st.current_guess));
    st.current_guess[0] = 'x';
    TEST_ASSERT_TRUE(ratimos_termo_submit(&st, "teste"));
    TEST_ASSERT_EQUAL_CHAR('x', st.current_guess[0]);
    TEST_ASSERT_EQUAL_UINT8(0, (uint8_t) st.current_guess[1]);
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

/* ------------------------------------------------------------------------
 * Linha de entrada posicional (plano 02.1-17). `_` = casa vazia.
 * ------------------------------------------------------------------------ */

static void row_set(char row[RATIMOS_TERMO_WORD_LEN + 1], const char * pat)
{
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        row[i] = (pat[i] == '_') ? '\0' : pat[i];
    }
    row[RATIMOS_TERMO_WORD_LEN] = '\0';
}

/* Linha como padrao de 5 chars ('_' = vazia) pra comparar com TEST_ASSERT_EQUAL_STRING. */
static const char * row_str(const char row[RATIMOS_TERMO_WORD_LEN + 1])
{
    static char out[RATIMOS_TERMO_WORD_LEN + 1];
    for (int i = 0; i < RATIMOS_TERMO_WORD_LEN; i++) {
        out[i] = row[i] ? row[i] : '_';
    }
    out[RATIMOS_TERMO_WORD_LEN] = '\0';
    return out;
}

void test_row_type_fills_in_order_and_ends_with_cursor_end(void)
{
    char row[RATIMOS_TERMO_WORD_LEN + 1];
    row_set(row, "_____");
    const char * word = "posse";
    const uint8_t expect_cursor[] = { 1, 2, 3, 4, RATIMOS_TERMO_CURSOR_END };
    uint8_t cursor = 0;
    for (int i = 0; i < 5; i++) {
        cursor = ratimos_termo_row_type(row, cursor, word[i]);
        TEST_ASSERT_EQUAL_UINT8(expect_cursor[i], cursor);
    }
    TEST_ASSERT_EQUAL_STRING("posse", row_str(row));
}

void test_row_type_from_the_middle_wraps_to_first_empty(void)
{
    char row[RATIMOS_TERMO_WORD_LEN + 1];
    row_set(row, "_____");
    uint8_t cursor = 2;
    cursor = ratimos_termo_row_type(row, cursor, 's');
    TEST_ASSERT_EQUAL_STRING("__s__", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(3, cursor);
    cursor = ratimos_termo_row_type(row, cursor, 't');
    TEST_ASSERT_EQUAL_STRING("__st_", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(4, cursor);
    cursor = ratimos_termo_row_type(row, cursor, 'a');
    TEST_ASSERT_EQUAL_STRING("__sta", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(0, cursor); /* volta pra 1a vazia */
    cursor = ratimos_termo_row_type(row, cursor, 'P'); /* maiuscula vira minuscula */
    TEST_ASSERT_EQUAL_STRING("p_sta", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(1, cursor);
}

void test_row_type_overwrites_and_ignores_when_full_or_invalid(void)
{
    char row[RATIMOS_TERMO_WORD_LEN + 1];
    row_set(row, "posse");
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_CURSOR_END, ratimos_termo_row_type(row, 1, 'a'));
    TEST_ASSERT_EQUAL_STRING("passe", row_str(row)); /* sobrescreve */

    TEST_ASSERT_EQUAL_UINT8(RATIMOS_TERMO_CURSOR_END,
                            ratimos_termo_row_type(row, RATIMOS_TERMO_CURSOR_END, 'z'));
    TEST_ASSERT_EQUAL_STRING("passe", row_str(row)); /* cheia + END: nada muda */

    row_set(row, "_____");
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_termo_row_type(row, 0, '1'));
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_termo_row_type(row, 0, '#'));
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_termo_row_type(row, 0, (char) 0xc3));
    TEST_ASSERT_EQUAL_STRING("_____", row_str(row));
}

void test_row_erase_follows_the_cursor_rules(void)
{
    char row[RATIMOS_TERMO_WORD_LEN + 1];

    row_set(row, "posse");
    uint8_t cursor = ratimos_termo_row_erase(row, RATIMOS_TERMO_CURSOR_END);
    TEST_ASSERT_EQUAL_STRING("poss_", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(4, cursor);
    cursor = ratimos_termo_row_erase(row, cursor); /* casa 4 vazia: apaga a 3 */
    TEST_ASSERT_EQUAL_STRING("pos__", row_str(row));
    TEST_ASSERT_EQUAL_UINT8(3, cursor);

    row_set(row, "p_s__");
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_termo_row_erase(row, 1)); /* vazia: apaga a cheia a esquerda */
    TEST_ASSERT_EQUAL_STRING("__s__", row_str(row));

    row_set(row, "p_s__");
    TEST_ASSERT_EQUAL_UINT8(2, ratimos_termo_row_erase(row, 2)); /* cheia: limpa e fica */
    TEST_ASSERT_EQUAL_STRING("p____", row_str(row));

    row_set(row, "_____");
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_termo_row_erase(row, 0));
    TEST_ASSERT_EQUAL_STRING("_____", row_str(row));
}

void test_submit_row_reports_why_it_refuses_and_clears_on_ok(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");

    row_set(st.current_guess, "pos_e");
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_SUBMIT_INCOMPLETE, ratimos_termo_submit_row(&st));
    TEST_ASSERT_EQUAL_STRING("pos_e", row_str(st.current_guess));
    TEST_ASSERT_EQUAL_UINT8(0, st.tries_used);

    row_set(st.current_guess, "zzzzz");
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_SUBMIT_NOT_ACCEPTED, ratimos_termo_submit_row(&st));
    TEST_ASSERT_EQUAL_STRING("zzzzz", row_str(st.current_guess));
    TEST_ASSERT_EQUAL_UINT8(0, st.tries_used);

    row_set(st.current_guess, "resto");
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_SUBMIT_OK, ratimos_termo_submit_row(&st));
    TEST_ASSERT_EQUAL_UINT8(1, st.tries_used);
    TEST_ASSERT_EQUAL_STRING("_____", row_str(st.current_guess));
    TEST_ASSERT_EQUAL_STRING("resto", st.history[0]);

    row_set(st.current_guess, "posse");
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_SUBMIT_OK, ratimos_termo_submit_row(&st));
    TEST_ASSERT_EQUAL_UINT8(1, st.finished);

    row_set(st.current_guess, "carta");
    TEST_ASSERT_EQUAL_INT(RATIMOS_TERMO_SUBMIT_FINISHED, ratimos_termo_submit_row(&st));
    TEST_ASSERT_EQUAL_UINT8(2, st.tries_used);
}

/* T-02.1-44: save corrompido nunca causa leitura fora do buffer. */
void test_state_sanitize_cleans_current_guess_and_missing_nuls(void)
{
    ratimos_termo_state_t st;
    make_termo_state(&st, "posse");
    const char dirty[] = { 'P', 0x01, 's', '#', 'e', 'x' };
    memcpy(st.current_guess, dirty, sizeof(dirty));
    TEST_ASSERT_TRUE(ratimos_termo_state_sanitize(&st));
    const char want[] = { 'p', 0, 's', 0, 'e', 0 };
    TEST_ASSERT_EQUAL_MEMORY(want, st.current_guess, sizeof(want));

    /* formato antigo (prefixo) continua posicional: "pe___" */
    make_termo_state(&st, "posse");
    memcpy(st.current_guess, "pe\0\0\0\0", 6);
    TEST_ASSERT_TRUE(ratimos_termo_state_sanitize(&st));
    TEST_ASSERT_EQUAL_STRING("pe___", row_str(st.current_guess));

    /* historico sem NUL no byte 5 */
    make_termo_state(&st, "posse");
    memset(st.history, 'a', sizeof(st.history));
    TEST_ASSERT_TRUE(ratimos_termo_state_sanitize(&st));
    for (int r = 0; r < RATIMOS_TERMO_MAX_TRIES; r++) {
        TEST_ASSERT_EQUAL_UINT8(0, (uint8_t) st.history[r][RATIMOS_TERMO_WORD_LEN]);
        TEST_ASSERT_EQUAL_UINT32(5, (uint32_t) strlen(st.history[r]));
    }
}

void test_state_sanitize_rejects_bad_answers_mode_and_finished(void)
{
    ratimos_termo_state_t st;

    make_termo_state(&st, "posse");
    memcpy(st.boards[0].answer, "possex", 6); /* sem NUL */
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st)); /* sem NUL: save invalido */

    make_termo_state(&st, "posse");
    st.boards[0].answer[2] = 'S'; /* fora de a-z */
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st));

    make_termo_state(&st, "posse");
    st.boards[0].answer[4] = '\0'; /* curta */
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st));

    make_dueto_state(&st, "posse", "cesta");
    st.boards[1].answer[0] = (char) 0xe3;
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st));

    make_termo_state(&st, "posse");
    st.mode = (ratimos_termo_mode_t) 9;
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st));

    make_termo_state(&st, "posse");
    st.finished = 3;
    TEST_ASSERT_FALSE(ratimos_termo_state_sanitize(&st));
}

void test_state_sanitize_recomputes_counts_and_limits_tries(void)
{
    ratimos_termo_state_t st;
    make_dueto_state(&st, "posse", "cesta");
    st.board_count = 4;
    st.max_tries = 200;
    st.tries_used = 250;
    st.boards[0].guesses_made = 99;
    TEST_ASSERT_TRUE(ratimos_termo_state_sanitize(&st));
    TEST_ASSERT_EQUAL_UINT8(2, st.board_count);
    TEST_ASSERT_EQUAL_UINT8(7, st.max_tries);
    TEST_ASSERT_EQUAL_UINT8(7, st.tries_used);
    TEST_ASSERT_TRUE(st.boards[0].guesses_made <= st.tries_used);
}

/* ------------------------------------------------------------------------
 * Lista aceita com flexoes comuns + formas acentuadas (plano 02.1-17).
 * ------------------------------------------------------------------------ */

/* Sonda do UAT 2026-10-07: palavras comuns flexionadas que o lexico (so'
 * lemas) recusava. Todas tem de ser palpites aceitos. */
void test_accepted_list_takes_the_common_inflections_probe(void)
{
    static const char * const probe[] = {
        "falou", "comeu", "vamos", "fomos", "sejam", "andei", "casas", "pesos",
        "gatos", "ficou", "disse", "podem", "fazia", "deram", "achei", "amava",
        "senti", "olhei", "dizia", "temos", "sabem", "novas", "bolos", "dados",
    };
    TEST_ASSERT_EQUAL_UINT32(24, (uint32_t) (sizeof(probe) / sizeof(probe[0])));
    for (size_t i = 0; i < sizeof(probe) / sizeof(probe[0]); i++) {
        TEST_ASSERT_TRUE_MESSAGE(ratimos_termo_is_accepted_guess(probe[i]), probe[i]);
    }
}

void test_every_answer_is_an_accepted_guess_and_the_pool_is_unchanged(void)
{
    TEST_ASSERT_EQUAL_UINT32(577, (uint32_t) ratimos_termo_answer_count());
    for (size_t a = 0; a < ratimos_termo_answer_count(); a++) {
        TEST_ASSERT_TRUE_MESSAGE(ratimos_termo_is_accepted_guess(ratimos_termo_answer_at(a)),
                                 ratimos_termo_answer_at(a));
    }
}

/* Entrada sem acento equivale a palavra acentuada (teclado sem acento);
 * qualquer byte nao ASCII e' recusado sem crash. */
void test_accent_insensitive_input_and_non_ascii_rejected(void)
{
    TEST_ASSERT_TRUE(ratimos_termo_is_accepted_guess("aurea"));
    TEST_ASSERT_TRUE(ratimos_termo_is_accepted_guess("AUREA"));
    TEST_ASSERT_TRUE(ratimos_termo_is_accepted_guess("aviao"));

    TEST_ASSERT_FALSE(ratimos_termo_is_accepted_guess("\xc3\xa1urea")); /* "áurea": 6 bytes */
    TEST_ASSERT_FALSE(ratimos_termo_is_accepted_guess("\xc3\xa1" "rea")); /* 5 bytes, 1 nao ASCII */
    TEST_ASSERT_FALSE(ratimos_termo_is_accepted_guess("\xff\xfe\xfd\xfc\xfb"));
    TEST_ASSERT_FALSE(ratimos_termo_is_accepted_guess(NULL));
    TEST_ASSERT_FALSE(ratimos_termo_is_accepted_guess("zzzzz"));
}

/* UTF-8 de exibicao (so' o conjunto PT-BR das fontes) -> ASCII minusculo.
 * Devolve quantos code points tinha, ou -1 se aparecer algo fora do conjunto. */
static int display_to_ascii(const char * s, char out[RATIMOS_TERMO_WORD_LEN + 1])
{
    int cps = 0;
    for (const unsigned char * p = (const unsigned char *) s; *p; cps++) {
        char c = 0;
        if (*p < 0x80) {
            c = (char) ((*p >= 'A' && *p <= 'Z') ? *p - 'A' + 'a' : *p);
            p++;
        } else if (p[0] == 0xc3) {
            switch (p[1]) {
            case 0x81: case 0x80: case 0x83: case 0x82: c = 'a'; break;
            case 0x89: case 0x8a: c = 'e'; break;
            case 0x8d: c = 'i'; break;
            case 0x93: case 0x94: case 0x95: c = 'o'; break;
            case 0x9a: c = 'u'; break;
            case 0x87: c = 'c'; break;
            default: return -1;
            }
            p += 2;
        } else {
            return -1;
        }
        if (cps >= RATIMOS_TERMO_WORD_LEN) {
            return cps + 1;
        }
        out[cps] = c;
    }
    out[cps < RATIMOS_TERMO_WORD_LEN ? cps : RATIMOS_TERMO_WORD_LEN] = '\0';
    return cps;
}

void test_display_form_shows_the_accented_answer(void)
{
    char out[RATIMOS_TERMO_DISPLAY_MAX];

    TEST_ASSERT_TRUE(ratimos_termo_display_form("aviao", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("AVI\xc3\x83O", out); /* AVIAO com til: "AVIÃO" */
    TEST_ASSERT_TRUE(ratimos_termo_display_form("AVIAO", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("AVI\xc3\x83O", out);

    TEST_ASSERT_TRUE(ratimos_termo_display_form("posse", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("POSSE", out);
    /* Fora das respostas: ASCII maiusculo. */
    TEST_ASSERT_TRUE(ratimos_termo_display_form("falou", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("FALOU", out);
}

void test_display_form_rejects_bad_arguments(void)
{
    char out[RATIMOS_TERMO_DISPLAY_MAX];
    TEST_ASSERT_FALSE(ratimos_termo_display_form(NULL, out, sizeof(out)));
    TEST_ASSERT_FALSE(ratimos_termo_display_form("aviao", NULL, sizeof(out)));
    TEST_ASSERT_FALSE(ratimos_termo_display_form("aviao", out, RATIMOS_TERMO_DISPLAY_MAX - 1)); /* buffer pequeno */
    TEST_ASSERT_FALSE(ratimos_termo_display_form("avia", out, sizeof(out)));
    TEST_ASSERT_FALSE(ratimos_termo_display_form("aviaos", out, sizeof(out)));
    TEST_ASSERT_FALSE(ratimos_termo_display_form("avi\xc3\xa3", out, sizeof(out))); /* 5 bytes nao ASCII */
}

/* Toda forma da tabela tem 5 code points e, normalizada, volta a uma chave
 * ASCII que e' resposta e que a propria funcao traduz de volta a ela. */
void test_display_table_forms_normalize_back_to_their_ascii_key(void)
{
    TEST_ASSERT_TRUE(ratimos_termo_display_count() > 0);
    for (size_t i = 0; i < ratimos_termo_display_count(); i++) {
        const char * form = ratimos_termo_display_at(i);
        TEST_ASSERT_NOT_NULL(form);
        char key[RATIMOS_TERMO_WORD_LEN + 2];
        TEST_ASSERT_EQUAL_INT_MESSAGE(5, display_to_ascii(form, key), form);

        bool is_answer = false;
        for (size_t a = 0; a < ratimos_termo_answer_count() && !is_answer; a++) {
            is_answer = strcmp(ratimos_termo_answer_at(a), key) == 0;
        }
        TEST_ASSERT_TRUE_MESSAGE(is_answer, form);

        char out[RATIMOS_TERMO_DISPLAY_MAX];
        TEST_ASSERT_TRUE(ratimos_termo_display_form(key, out, sizeof(out)));
        TEST_ASSERT_EQUAL_STRING(form, out);
    }
    TEST_ASSERT_NULL(ratimos_termo_display_at(ratimos_termo_display_count()));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_score_guess_exact_match_all_correct);
    RUN_TEST(test_score_guess_duplicate_letter_matched_correct_marks_extra_absent);
    RUN_TEST(test_score_guess_duplicate_letters_both_misplaced_marks_both_present);
    RUN_TEST(test_score_guess_triple_duplicate_letter_one_in_answer_marks_one_hit);
    RUN_TEST(test_score_guess_duplicate_letter_mixed_present_only);
    RUN_TEST(test_score_guess_arara_against_pasta_two_pass);
    RUN_TEST(test_score_guess_arara_against_carta_two_pass);

    RUN_TEST(test_mode_table_board_counts_and_max_tries);

    RUN_TEST(test_start_daily_same_day_same_mode_identical_answers);
    RUN_TEST(test_start_daily_same_day_different_mode_different_answers);
    RUN_TEST(test_start_daily_dueto_answers_distinct);
    RUN_TEST(test_start_daily_quarteto_answers_distinct);

    RUN_TEST(test_submit_rejects_guess_not_in_accepted_list_without_consuming_try);
    RUN_TEST(test_blocklisted_common_words_are_accepted_guesses_but_never_answers);
    RUN_TEST(test_key_states_termo_correct_beats_present_and_absent_letters);
    RUN_TEST(test_key_states_dueto_only_count_boards_still_in_play);
    RUN_TEST(test_key_states_arara_against_pasta);
    RUN_TEST(test_submit_never_prefills_the_current_guess);
    RUN_TEST(test_submit_consumes_exactly_one_try_per_guess);
    RUN_TEST(test_submit_wins_when_all_boards_solved);
    RUN_TEST(test_submit_loses_when_tries_exhausted_with_unsolved_board);
    RUN_TEST(test_submit_rejects_after_finished);
    RUN_TEST(test_submit_applies_shared_guess_to_unsolved_boards_only);

    RUN_TEST(test_row_type_fills_in_order_and_ends_with_cursor_end);
    RUN_TEST(test_row_type_from_the_middle_wraps_to_first_empty);
    RUN_TEST(test_row_type_overwrites_and_ignores_when_full_or_invalid);
    RUN_TEST(test_row_erase_follows_the_cursor_rules);
    RUN_TEST(test_submit_row_reports_why_it_refuses_and_clears_on_ok);
    RUN_TEST(test_state_sanitize_cleans_current_guess_and_missing_nuls);
    RUN_TEST(test_state_sanitize_rejects_bad_answers_mode_and_finished);
    RUN_TEST(test_state_sanitize_recomputes_counts_and_limits_tries);

    RUN_TEST(test_accepted_list_takes_the_common_inflections_probe);
    RUN_TEST(test_every_answer_is_an_accepted_guess_and_the_pool_is_unchanged);
    RUN_TEST(test_accent_insensitive_input_and_non_ascii_rejected);
    RUN_TEST(test_display_form_shows_the_accented_answer);
    RUN_TEST(test_display_form_rejects_bad_arguments);
    RUN_TEST(test_display_table_forms_normalize_back_to_their_ascii_key);

    return UNITY_END();
}
