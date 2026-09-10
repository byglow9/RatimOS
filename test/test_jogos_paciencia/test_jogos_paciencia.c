/*
 * Suite do motor do paciencia/klondike (JOGOS-01) --
 * src/ratimos/apps/jogos/klondike_engine.c.
 *
 * Exercita apenas o motor, deliberadamente livre de LVGL para nao precisar
 * de SDL/display para rodar via PlatformIO native Unity. As duas checagens
 * load-bearing sao: (1) um baralho novo contem as 52 cartas exatamente uma
 * vez, e (2) uma jogada ilegal deixa o estado byte-a-byte identico -- ver
 * comentario em klondike_engine.h sobre o unico ponto de entrada de
 * mutacao nunca pular a checagem de legalidade.
 */
#include <string.h>
#include <unity.h>

#include "ratimos/apps/jogos/klondike_engine.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------------
 * ratimos_klondike_deal
 * ------------------------------------------------------------------------ */

void test_deal_contains_all_52_cards_exactly_once(void)
{
    ratimos_klondike_state_t st;
    ratimos_klondike_deal(&st, 42, false);

    int seen[4][14];
    memset(seen, 0, sizeof(seen));

    for (int c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        for (uint8_t i = 0; i < st.tableau[c].count; i++) {
            ratimos_card_t card = st.tableau[c].cards[i];
            seen[ratimos_card_suit(card)][ratimos_card_rank(card)]++;
        }
    }
    for (int f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        for (uint8_t i = 0; i < st.foundation[f].count; i++) {
            ratimos_card_t card = st.foundation[f].cards[i];
            seen[ratimos_card_suit(card)][ratimos_card_rank(card)]++;
        }
    }
    for (uint8_t i = 0; i < st.stock.count; i++) {
        ratimos_card_t card = st.stock.cards[i];
        seen[ratimos_card_suit(card)][ratimos_card_rank(card)]++;
    }
    for (uint8_t i = 0; i < st.waste.count; i++) {
        ratimos_card_t card = st.waste.cards[i];
        seen[ratimos_card_suit(card)][ratimos_card_rank(card)]++;
    }

    int total = 0;
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            TEST_ASSERT_EQUAL_INT(1, seen[suit][rank]);
            total++;
        }
    }
    TEST_ASSERT_EQUAL_INT(52, total);
}

void test_deal_tableau_columns_have_correct_size_and_exactly_one_face_up(void)
{
    ratimos_klondike_state_t st;
    ratimos_klondike_deal(&st, 7, false);

    for (int c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        TEST_ASSERT_EQUAL_UINT8((uint8_t) (c + 1), st.tableau[c].count);

        int face_up_count = 0;
        for (uint8_t i = 0; i < st.tableau[c].count; i++) {
            if (ratimos_card_is_face_up(st.tableau[c].cards[i])) {
                face_up_count++;
                TEST_ASSERT_EQUAL_UINT8((uint8_t) (st.tableau[c].count - 1), i);
            }
        }
        TEST_ASSERT_EQUAL_INT(1, face_up_count);
    }

    TEST_ASSERT_EQUAL_UINT8(24, st.stock.count);
    TEST_ASSERT_EQUAL_UINT8(0, st.waste.count);
    for (int f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        TEST_ASSERT_EQUAL_UINT8(0, st.foundation[f].count);
    }
}

void test_deal_same_seed_is_byte_identical(void)
{
    ratimos_klondike_state_t a, b;
    ratimos_klondike_deal(&a, 1234, true);
    ratimos_klondike_deal(&b, 1234, true);
    TEST_ASSERT_EQUAL_MEMORY(&a, &b, sizeof(a));
}

void test_deal_different_seeds_differ(void)
{
    ratimos_klondike_state_t a, b;
    ratimos_klondike_deal(&a, 1234, false);
    ratimos_klondike_deal(&b, 5678, false);
    TEST_ASSERT_TRUE(memcmp(&a, &b, sizeof(a)) != 0);
}

/* ------------------------------------------------------------------------
 * Legalidade -- tableau-to-tableau
 * ------------------------------------------------------------------------ */

void test_tableau_to_tableau_legal_descending_alternating_color(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    /* Coluna 0: 7 de copas (vermelho) no topo, face para cima. */
    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 7, true);
    st.tableau[0].count = 1;

    /* Coluna 1: 6 de paus (preto), face para cima -- aceita o 7 vermelho. */
    st.tableau[1].cards[0] = ratimos_card_make(RATIMOS_SUIT_CLUBS, 6, true);
    st.tableau[1].count = 1;

    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 1, 0, 1));
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 1, 0, 1));

    TEST_ASSERT_EQUAL_UINT8(0, st.tableau[1].count);
    TEST_ASSERT_EQUAL_UINT8(2, st.tableau[0].count);
    TEST_ASSERT_EQUAL_UINT8(6, ratimos_card_rank(st.tableau[0].cards[1]));
}

void test_tableau_to_tableau_illegal_same_color_rejected(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 7, true);
    st.tableau[0].count = 1;
    /* Coluna 1: 6 de copas (vermelho) -- mesma cor do 7, ilegal. */
    st.tableau[1].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 6, true);
    st.tableau[1].count = 1;

    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 1, 0, 1));
}

void test_tableau_to_tableau_onto_empty_column_requires_king(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_SPADES, 7, true);
    st.tableau[0].count = 1;
    /* Coluna 1 vazia. */

    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 0, 1, 1));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_SPADES, 13, true);
    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 0, 1, 1));
}

/* ------------------------------------------------------------------------
 * Legalidade -- tableau-to-foundation
 * ------------------------------------------------------------------------ */

void test_tableau_to_foundation_ace_onto_empty_accepted(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_DIAMONDS, 1, true);
    st.tableau[0].count = 1;

    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(1, st.foundation[0].count);
    TEST_ASSERT_EQUAL_UINT8(0, st.tableau[0].count);
}

void test_tableau_to_foundation_wrong_suit_or_non_ascending_rejected(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.foundation[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_DIAMONDS, 1, true);
    st.foundation[0].count = 1;

    /* Naipe errado sobre um as de ouros ja colocado. */
    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 2, true);
    st.tableau[0].count = 1;
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));

    /* Naipe certo mas valor nao-ascendente (pula o 2). */
    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_DIAMONDS, 3, true);
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));

    /* Naipe certo e valor ascendente correto -- legal. */
    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_DIAMONDS, 2, true);
    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));
}

/* ------------------------------------------------------------------------
 * Legalidade -- descarte (waste) segue as mesmas regras do tableau
 * ------------------------------------------------------------------------ */

void test_waste_to_tableau_and_waste_to_foundation_follow_same_rules(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.waste.cards[0] = ratimos_card_make(RATIMOS_SUIT_CLUBS, 5, true);
    st.waste.count = 1;

    /* tableau vazio exige rei -- um 5 e ilegal numa coluna vazia. */
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU, 0, 0, 1));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 6, true);
    st.tableau[0].count = 1;
    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU, 0, 0, 1));
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU, 0, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, st.waste.count);
    TEST_ASSERT_EQUAL_UINT8(2, st.tableau[0].count);

    /* waste-to-foundation: so aceita as numa fundacao vazia. */
    memset(&st, 0, sizeof(st));
    st.waste.cards[0] = ratimos_card_make(RATIMOS_SUIT_SPADES, 5, true);
    st.waste.count = 1;
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION, 0, 0, 1));

    st.waste.cards[0] = ratimos_card_make(RATIMOS_SUIT_SPADES, 1, true);
    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION, 0, 0, 1));
}

/* ------------------------------------------------------------------------
 * Estoque / descarte -- compra e reciclagem
 * ------------------------------------------------------------------------ */

void test_stock_to_waste_turns_configured_count_and_flips_face_up(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.stock.cards[0] = ratimos_card_make(RATIMOS_SUIT_CLUBS, 9, false);
    st.stock.cards[1] = ratimos_card_make(RATIMOS_SUIT_DIAMONDS, 10, false);
    st.stock.count = 2;

    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE, 0, 0, 0));
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE, 0, 0, 0));

    TEST_ASSERT_EQUAL_UINT8(RATIMOS_KLONDIKE_DRAW_COUNT, st.waste.count);
    TEST_ASSERT_EQUAL_UINT8((uint8_t) (2 - RATIMOS_KLONDIKE_DRAW_COUNT), st.stock.count);
    TEST_ASSERT_TRUE(ratimos_card_is_face_up(st.waste.cards[st.waste.count - 1]));
    TEST_ASSERT_EQUAL_UINT8(10, ratimos_card_rank(st.waste.cards[st.waste.count - 1]));
}

void test_stock_to_waste_illegal_when_stock_empty(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE, 0, 0, 0));
}

void test_recycle_waste_legal_only_when_stock_empty_and_waste_non_empty(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    /* Ambos vazios -- ilegal. */
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0));

    st.waste.cards[0] = ratimos_card_make(RATIMOS_SUIT_CLUBS, 2, true);
    st.waste.cards[1] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 3, true);
    st.waste.count = 2;

    /* Estoque ainda nao-vazio -- ilegal reciclar. */
    st.stock.cards[0] = ratimos_card_make(RATIMOS_SUIT_SPADES, 9, false);
    st.stock.count = 1;
    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0));

    /* Estoque vazio, descarte nao-vazio -- legal; ordem de compra preservada. */
    st.stock.count = 0;
    TEST_ASSERT_TRUE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0));
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0));

    TEST_ASSERT_EQUAL_UINT8(0, st.waste.count);
    TEST_ASSERT_EQUAL_UINT8(2, st.stock.count);
    for (uint8_t i = 0; i < st.stock.count; i++) {
        TEST_ASSERT_FALSE(ratimos_card_is_face_up(st.stock.cards[i]));
    }
    /* A carta mais antiga do descarte (indice 0, o 2 de paus) volta pro
     * topo do estoque -- proxima a ser comprada, mesma ordem de antes. */
    TEST_ASSERT_EQUAL_UINT8(2, ratimos_card_rank(st.stock.cards[st.stock.count - 1]));
}

/* ------------------------------------------------------------------------
 * Virada automatica / carta de face para baixo nunca e origem
 * ------------------------------------------------------------------------ */

void test_moving_last_face_up_card_auto_flips_exposed_card(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_CLUBS, 5, false); /* fica exposta */
    st.tableau[0].cards[1] = ratimos_card_make(RATIMOS_SUIT_SPADES, 13, true); /* rei, movivel */
    st.tableau[0].count = 2;
    /* Coluna 1 vazia -- so aceita rei. */

    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 0, 1, 1));

    TEST_ASSERT_EQUAL_UINT8(1, st.tableau[0].count);
    TEST_ASSERT_TRUE(ratimos_card_is_face_up(st.tableau[0].cards[0]));
    TEST_ASSERT_EQUAL_UINT8(5, ratimos_card_rank(st.tableau[0].cards[0]));
}

void test_face_down_card_can_never_be_move_source(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    /* Um as de face para baixo no topo de uma coluna -- estado
     * estruturalmente anomalo, mas o motor deve recusar independentemente
     * de por que a face-down chegou la (defesa contra estado restaurado de
     * disco adulterado, T-02.1-02). */
    st.tableau[0].cards[0] = ratimos_card_make(RATIMOS_SUIT_HEARTS, 1, false);
    st.tableau[0].count = 1;

    TEST_ASSERT_FALSE(ratimos_klondike_can_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));
    TEST_ASSERT_FALSE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 0, 1));
}

/* ------------------------------------------------------------------------
 * Vitoria
 * ------------------------------------------------------------------------ */

static void fill_all_foundations(ratimos_klondike_state_t * st)
{
    for (uint8_t suit = 0; suit < RATIMOS_KLONDIKE_FOUNDATIONS; suit++) {
        st->foundation[suit].count = 13;
        for (uint8_t rank = 1; rank <= 13; rank++) {
            st->foundation[suit].cards[rank - 1] = ratimos_card_make(suit, rank, true);
        }
    }
}

void test_is_won_true_exactly_when_all_foundations_complete(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));
    TEST_ASSERT_FALSE(ratimos_klondike_is_won(&st));

    fill_all_foundations(&st);
    TEST_ASSERT_TRUE(ratimos_klondike_is_won(&st));

    st.foundation[3].count = 12; /* uma fundacao incompleta */
    TEST_ASSERT_FALSE(ratimos_klondike_is_won(&st));
}

/* ------------------------------------------------------------------------
 * Jogada ilegal deixa o estado inteiro byte-a-byte inalterado (load-bearing)
 * ------------------------------------------------------------------------ */

void test_illegal_move_leaves_state_byte_identical(void)
{
    ratimos_klondike_state_t st;
    ratimos_klondike_deal(&st, 99, false);

    ratimos_klondike_state_t before;
    memcpy(&before, &st, sizeof(st));

    /* Coluna 0 para coluna 1 com contagem absurda -- ilegal por varios
     * motivos (capacidade/sequencia), nunca deve tocar em nada. */
    TEST_ASSERT_FALSE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, 0, 1, 5));
    TEST_ASSERT_EQUAL_MEMORY(&before, &st, sizeof(st));

    /* Reciclar com o estoque cheio (dado que o deal sempre deixa o
     * estoque com 24 cartas) tambem e ilegal. */
    TEST_ASSERT_FALSE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0));
    TEST_ASSERT_EQUAL_MEMORY(&before, &st, sizeof(st));
}

/* ------------------------------------------------------------------------
 * Task 3: campo `won` apos a ultima jogada, auto-collect limitado, e
 * round-trip do estado atraves do blob de save.
 * ------------------------------------------------------------------------ */

void test_move_sets_won_field_true_when_final_foundation_completes(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    fill_all_foundations(&st);
    /* Esvazia so a ultima fundacao e poe uma unica carta no tableau pronta
     * pra completa-la -- a jogada que fecha essa fundacao deve deixar
     * st.won == 1 como efeito colateral de ratimos_klondike_move(). */
    st.foundation[3].count = 12;
    st.tableau[0].cards[0] = ratimos_card_make(3, 13, true);
    st.tableau[0].count = 1;

    TEST_ASSERT_EQUAL_UINT8(0, st.won);
    TEST_ASSERT_TRUE(ratimos_klondike_move(&st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, 0, 3, 1));
    TEST_ASSERT_EQUAL_UINT8(1, st.won);
    TEST_ASSERT_TRUE(ratimos_klondike_is_won(&st));
}

void test_auto_collect_returns_zero_and_terminates_when_no_foundation_move_available(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));
    /* Estado inteiramente vazio -- nenhuma pilha tem carta nenhuma, entao
     * nenhuma jogada de fundacao pode existir; a chamada tem que voltar
     * imediatamente (0 jogadas) em vez de rodar RATIMOS_KLONDIKE_AUTOCOLLECT_MAX
     * vezes inutilmente. */
    TEST_ASSERT_EQUAL_UINT16(0, ratimos_klondike_auto_collect(&st));
}

void test_auto_collect_clears_every_available_foundation_move_within_cap(void)
{
    ratimos_klondike_state_t st;
    memset(&st, 0, sizeof(st));

    /* Um as de cada naipe, cada um sozinho no topo de uma coluna do
     * tableau -- todos legais pra fundacao de cara, sem depender de ordem. */
    for (uint8_t suit = 0; suit < 4; suit++) {
        st.tableau[suit].cards[0] = ratimos_card_make(suit, 1, true);
        st.tableau[suit].count = 1;
    }

    uint16_t applied = ratimos_klondike_auto_collect(&st);
    TEST_ASSERT_EQUAL_UINT16(4, applied);
    TEST_ASSERT_TRUE(applied <= RATIMOS_KLONDIKE_AUTOCOLLECT_MAX);

    for (uint8_t suit = 0; suit < 4; suit++) {
        TEST_ASSERT_EQUAL_UINT8(0, st.tableau[suit].count);
        TEST_ASSERT_EQUAL_UINT8(1, st.foundation[suit].count);
    }
}

void test_state_round_trips_through_game_state_blob(void)
{
    ratimos_klondike_state_t original;
    ratimos_klondike_deal(&original, 555, true);
    original.won = 0;
    original.daily = 1;
    original.daily_win_recorded = 1;
    original.moves = 7;

    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &original, sizeof(original));
    blob.used = sizeof(original);

    ratimos_klondike_state_t restored;
    memset(&restored, 0, sizeof(restored));
    TEST_ASSERT_EQUAL_size_t(sizeof(original), blob.used);
    memcpy(&restored, blob.bytes, sizeof(restored));

    TEST_ASSERT_EQUAL_UINT8(original.won, restored.won);
    TEST_ASSERT_EQUAL_UINT8(original.daily, restored.daily);
    TEST_ASSERT_EQUAL_UINT8(original.daily_win_recorded, restored.daily_win_recorded);
    TEST_ASSERT_EQUAL_UINT16(original.moves, restored.moves);
    for (int c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        TEST_ASSERT_EQUAL_MEMORY(&original.tableau[c], &restored.tableau[c], sizeof(original.tableau[c]));
    }
    for (int f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        TEST_ASSERT_EQUAL_MEMORY(&original.foundation[f], &restored.foundation[f], sizeof(original.foundation[f]));
    }
    TEST_ASSERT_EQUAL_MEMORY(&original.stock, &restored.stock, sizeof(original.stock));
    TEST_ASSERT_EQUAL_MEMORY(&original.waste, &restored.waste, sizeof(original.waste));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_deal_contains_all_52_cards_exactly_once);
    RUN_TEST(test_deal_tableau_columns_have_correct_size_and_exactly_one_face_up);
    RUN_TEST(test_deal_same_seed_is_byte_identical);
    RUN_TEST(test_deal_different_seeds_differ);

    RUN_TEST(test_tableau_to_tableau_legal_descending_alternating_color);
    RUN_TEST(test_tableau_to_tableau_illegal_same_color_rejected);
    RUN_TEST(test_tableau_to_tableau_onto_empty_column_requires_king);

    RUN_TEST(test_tableau_to_foundation_ace_onto_empty_accepted);
    RUN_TEST(test_tableau_to_foundation_wrong_suit_or_non_ascending_rejected);

    RUN_TEST(test_waste_to_tableau_and_waste_to_foundation_follow_same_rules);

    RUN_TEST(test_stock_to_waste_turns_configured_count_and_flips_face_up);
    RUN_TEST(test_stock_to_waste_illegal_when_stock_empty);
    RUN_TEST(test_recycle_waste_legal_only_when_stock_empty_and_waste_non_empty);

    RUN_TEST(test_moving_last_face_up_card_auto_flips_exposed_card);
    RUN_TEST(test_face_down_card_can_never_be_move_source);

    RUN_TEST(test_is_won_true_exactly_when_all_foundations_complete);

    RUN_TEST(test_illegal_move_leaves_state_byte_identical);

    RUN_TEST(test_move_sets_won_field_true_when_final_foundation_completes);
    RUN_TEST(test_auto_collect_returns_zero_and_terminates_when_no_foundation_move_available);
    RUN_TEST(test_auto_collect_clears_every_available_foundation_move_within_cap);
    RUN_TEST(test_state_round_trips_through_game_state_blob);

    return UNITY_END();
}
