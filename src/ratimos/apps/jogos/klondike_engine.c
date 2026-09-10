/*
 * Motor do paciencia/klondike (JOGOS-01) -- baralho, distribuicao semeada,
 * legalidade por tipo de jogada, virada automatica, deteccao de vitoria.
 *
 * Toda mutacao passa por ratimos_klondike_move(), que SEMPRE chama
 * ratimos_klondike_can_move() antes de tocar em qualquer byte -- a jogada
 * ilegal mais comum de um clone de paciencia mal feito (RESEARCH
 * "Don't Hand-Roll") e aceitar silenciosamente um movimento que quebra as
 * regras; aqui isso e estruturalmente impossivel porque so existe um unico
 * ponto de entrada de mutacao e ele nunca pula a checagem.
 */
#include "klondike_engine.h"

#include <string.h>

/* PRNG local (xorshift32) -- a distribuicao NUNCA usa o gerador
 * pseudoaleatorio global da libc, para que os testes sejam deterministas e
 * independentes de ordem de chamada. Semente 0 travaria o xorshift em 0
 * para sempre; forcada para 1. */
static uint32_t klondike_xorshift32(uint32_t * state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static ratimos_card_t pile_top(const ratimos_pile_t * pile)
{
    if (!pile || pile->count == 0) {
        return 0;
    }
    return pile->cards[pile->count - 1];
}

/* Copia `count` cartas do topo de `src` para o topo de `dest`, preservando
 * a ordem relativa entre elas. O chamador ja garantiu (via can_move + uma
 * checagem de capacidade propria) que `count <= src->count` e que
 * `dest->count + count` cabe em RATIMOS_KLONDIKE_MAX_PILE antes de chamar
 * isto -- esta funcao nao valida nada, so move bytes. */
static void move_cards(ratimos_pile_t * src, ratimos_pile_t * dest, uint8_t count)
{
    uint8_t start = (uint8_t) (src->count - count);
    for (uint8_t i = 0; i < count; i++) {
        dest->cards[dest->count + i] = src->cards[start + i];
    }
    dest->count = (uint8_t) (dest->count + count);
    src->count = (uint8_t) (src->count - count);
}

/* Vira automaticamente a nova carta do topo de uma coluna do tableau,
 * quando ela ainda esta de face para baixo -- chamado apos qualquer jogada
 * que remova a carta face-para-cima anterior dessa coluna. */
static void tableau_auto_flip(ratimos_pile_t * pile)
{
    if (pile->count == 0) {
        return;
    }
    ratimos_card_t top = pile->cards[pile->count - 1];
    if (!ratimos_card_is_face_up(top)) {
        pile->cards[pile->count - 1] = (ratimos_card_t) (top | 0x40u);
    }
}

/* Uma coluna do tableau so aceita `card` se: a coluna esta vazia e `card` e
 * um rei, ou a coluna tem uma carta no topo (sempre face-para-cima, por
 * construcao) de cor oposta e valor uma unidade acima de `card`. */
static bool tableau_accepts_card(const ratimos_pile_t * dest, ratimos_card_t card)
{
    if (dest->count == 0) {
        return ratimos_card_rank(card) == 13;
    }
    ratimos_card_t top = pile_top(dest);
    if (!ratimos_card_is_face_up(top)) {
        return false; /* defensivo -- nunca deveria acontecer numa coluna valida */
    }
    if (ratimos_card_is_red(top) == ratimos_card_is_red(card)) {
        return false;
    }
    return ratimos_card_rank(top) == ratimos_card_rank(card) + 1;
}

/* Uma fundacao so aceita `card` se: esta vazia e `card` e um as, ou tem uma
 * carta no topo do mesmo naipe de `card` com valor uma unidade abaixo. */
static bool foundation_accepts_card(const ratimos_pile_t * dest, ratimos_card_t card)
{
    if (dest->count == 0) {
        return ratimos_card_rank(card) == 1;
    }
    ratimos_card_t top = pile_top(dest);
    return ratimos_card_suit(top) == ratimos_card_suit(card)
        && (uint8_t) (ratimos_card_rank(top) + 1) == ratimos_card_rank(card);
}

/* Confirma que as `count` cartas do topo de `pile` formam, por si so, uma
 * sequencia valida de descida com cores alternadas e que todas estao com a
 * face para cima -- exigido antes de mover um bloco de varias cartas de
 * uma coluna do tableau para outra. */
static bool tableau_run_is_valid(const ratimos_pile_t * pile, uint8_t count)
{
    if (!pile || count == 0 || count > pile->count) {
        return false;
    }
    uint8_t start = (uint8_t) (pile->count - count);
    for (uint8_t i = start; i < pile->count; i++) {
        if (!ratimos_card_is_face_up(pile->cards[i])) {
            return false;
        }
        if (i > start) {
            ratimos_card_t prev = pile->cards[i - 1];
            ratimos_card_t cur = pile->cards[i];
            if (ratimos_card_is_red(prev) == ratimos_card_is_red(cur)) {
                return false;
            }
            if (ratimos_card_rank(prev) != (uint8_t) (ratimos_card_rank(cur) + 1)) {
                return false;
            }
        }
    }
    return true;
}

static bool can_move_tableau_to_tableau(const ratimos_klondike_state_t * st, uint8_t from, uint8_t to,
                                         uint8_t count)
{
    if (from >= RATIMOS_KLONDIKE_TABLEAU_COLS || to >= RATIMOS_KLONDIKE_TABLEAU_COLS || from == to) {
        return false;
    }
    const ratimos_pile_t * src = &st->tableau[from];
    if (count == 0 || count > src->count) {
        return false;
    }
    if (!tableau_run_is_valid(src, count)) {
        return false;
    }
    ratimos_card_t moving = src->cards[src->count - count];
    return tableau_accepts_card(&st->tableau[to], moving);
}

static bool can_move_tableau_to_foundation(const ratimos_klondike_state_t * st, uint8_t from, uint8_t to)
{
    if (from >= RATIMOS_KLONDIKE_TABLEAU_COLS || to >= RATIMOS_KLONDIKE_FOUNDATIONS) {
        return false;
    }
    const ratimos_pile_t * src = &st->tableau[from];
    if (src->count == 0) {
        return false;
    }
    ratimos_card_t top = pile_top(src);
    if (!ratimos_card_is_face_up(top)) {
        return false; /* uma carta de face para baixo nunca pode ser origem de jogada */
    }
    return foundation_accepts_card(&st->foundation[to], top);
}

static bool can_move_waste_to_tableau(const ratimos_klondike_state_t * st, uint8_t to)
{
    if (to >= RATIMOS_KLONDIKE_TABLEAU_COLS || st->waste.count == 0) {
        return false;
    }
    return tableau_accepts_card(&st->tableau[to], pile_top(&st->waste));
}

static bool can_move_waste_to_foundation(const ratimos_klondike_state_t * st, uint8_t to)
{
    if (to >= RATIMOS_KLONDIKE_FOUNDATIONS || st->waste.count == 0) {
        return false;
    }
    return foundation_accepts_card(&st->foundation[to], pile_top(&st->waste));
}

static bool can_move_foundation_to_tableau(const ratimos_klondike_state_t * st, uint8_t from, uint8_t to)
{
    if (from >= RATIMOS_KLONDIKE_FOUNDATIONS || to >= RATIMOS_KLONDIKE_TABLEAU_COLS) {
        return false;
    }
    const ratimos_pile_t * src = &st->foundation[from];
    if (src->count == 0) {
        return false;
    }
    return tableau_accepts_card(&st->tableau[to], pile_top(src));
}

static bool can_move_stock_to_waste(const ratimos_klondike_state_t * st)
{
    return st->stock.count > 0;
}

static bool can_move_recycle_waste(const ratimos_klondike_state_t * st)
{
    return st->stock.count == 0 && st->waste.count > 0;
}

bool ratimos_klondike_can_move(const ratimos_klondike_state_t * st, ratimos_klondike_move_kind_t kind,
                                uint8_t from_index, uint8_t to_index, uint8_t card_count)
{
    if (!st) {
        return false;
    }

    switch (kind) {
        case RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU:
            return can_move_tableau_to_tableau(st, from_index, to_index, card_count);
        case RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION:
            return can_move_tableau_to_foundation(st, from_index, to_index);
        case RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU:
            return can_move_waste_to_tableau(st, to_index);
        case RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION:
            return can_move_waste_to_foundation(st, to_index);
        case RATIMOS_KLONDIKE_MOVE_FOUNDATION_TO_TABLEAU:
            return can_move_foundation_to_tableau(st, from_index, to_index);
        case RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE:
            return can_move_stock_to_waste(st);
        case RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE:
            return can_move_recycle_waste(st);
        default:
            return false;
    }
}

bool ratimos_klondike_move(ratimos_klondike_state_t * st, ratimos_klondike_move_kind_t kind,
                            uint8_t from_index, uint8_t to_index, uint8_t card_count)
{
    if (!ratimos_klondike_can_move(st, kind, from_index, to_index, card_count)) {
        return false;
    }

    switch (kind) {
        case RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU: {
            ratimos_pile_t * src = &st->tableau[from_index];
            ratimos_pile_t * dest = &st->tableau[to_index];
            if ((uint16_t) dest->count + card_count > RATIMOS_KLONDIKE_MAX_PILE) {
                return false; /* defensivo: pilha restaurada de disco poderia estar quase cheia */
            }
            move_cards(src, dest, card_count);
            tableau_auto_flip(src);
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION: {
            ratimos_pile_t * src = &st->tableau[from_index];
            ratimos_pile_t * dest = &st->foundation[to_index];
            if (dest->count >= RATIMOS_KLONDIKE_MAX_PILE) {
                return false;
            }
            move_cards(src, dest, 1);
            tableau_auto_flip(src);
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU: {
            ratimos_pile_t * dest = &st->tableau[to_index];
            if ((uint16_t) dest->count + 1 > RATIMOS_KLONDIKE_MAX_PILE) {
                return false;
            }
            move_cards(&st->waste, dest, 1);
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION: {
            ratimos_pile_t * dest = &st->foundation[to_index];
            if (dest->count >= RATIMOS_KLONDIKE_MAX_PILE) {
                return false;
            }
            move_cards(&st->waste, dest, 1);
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_FOUNDATION_TO_TABLEAU: {
            ratimos_pile_t * src = &st->foundation[from_index];
            ratimos_pile_t * dest = &st->tableau[to_index];
            if ((uint16_t) dest->count + 1 > RATIMOS_KLONDIKE_MAX_PILE) {
                return false;
            }
            move_cards(src, dest, 1);
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE: {
            uint8_t n = RATIMOS_KLONDIKE_DRAW_COUNT;
            if (n > st->stock.count) {
                n = st->stock.count;
            }
            if ((uint16_t) st->waste.count + n > RATIMOS_KLONDIKE_MAX_PILE) {
                return false;
            }
            for (uint8_t i = 0; i < n; i++) {
                ratimos_card_t card = st->stock.cards[st->stock.count - 1];
                st->stock.count--;
                card = (ratimos_card_t) (card | 0x40u); /* vira para cima ao entrar no descarte */
                st->waste.cards[st->waste.count] = card;
                st->waste.count++;
            }
            break;
        }
        case RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE: {
            /* Devolve o descarte ao estoque virando cada carta para baixo,
             * preservando a ordem de compra: a carta mais antiga do
             * descarte (indice 0) vira a proxima a ser comprada (topo do
             * novo estoque), exatamente a mesma sequencia da rodada
             * anterior. */
            uint8_t n = st->waste.count;
            for (uint8_t i = 0; i < n; i++) {
                ratimos_card_t card = st->waste.cards[n - 1 - i];
                card = (ratimos_card_t) (card & (uint8_t) ~0x40u);
                st->stock.cards[i] = card;
            }
            st->stock.count = n;
            st->waste.count = 0;
            break;
        }
        default:
            return false;
    }

    st->moves++;
    st->won = ratimos_klondike_is_won(st) ? 1 : 0;
    return true;
}

bool ratimos_klondike_is_won(const ratimos_klondike_state_t * st)
{
    if (!st) {
        return false;
    }
    for (int i = 0; i < RATIMOS_KLONDIKE_FOUNDATIONS; i++) {
        if (st->foundation[i].count != 13) {
            return false;
        }
    }
    return true;
}

uint16_t ratimos_klondike_auto_collect(ratimos_klondike_state_t * st)
{
    if (!st) {
        return 0;
    }

    uint16_t applied = 0;
    for (uint16_t i = 0; i < RATIMOS_KLONDIKE_AUTOCOLLECT_MAX; i++) {
        bool moved = false;

        for (uint8_t f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS && !moved; f++) {
            if (ratimos_klondike_move(st, RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION, 0, f, 1)) {
                moved = true;
            }
        }
        if (!moved) {
            for (uint8_t c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS && !moved; c++) {
                for (uint8_t f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS && !moved; f++) {
                    if (ratimos_klondike_move(st, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, c, f, 1)) {
                        moved = true;
                    }
                }
            }
        }

        if (!moved) {
            break; /* nenhuma jogada de fundacao legal restante nesta passada */
        }
        applied++;
    }

    return applied;
}

void ratimos_klondike_deal(ratimos_klondike_state_t * out, uint32_t seed, bool daily)
{
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));

    ratimos_card_t deck[52];
    int idx = 0;
    for (uint8_t suit = 0; suit < 4; suit++) {
        for (uint8_t rank = 1; rank <= 13; rank++) {
            deck[idx++] = ratimos_card_make(suit, rank, false);
        }
    }

    uint32_t rng = seed != 0 ? seed : 1u;
    for (int i = 51; i > 0; i--) {
        int j = (int) (klondike_xorshift32(&rng) % (uint32_t) (i + 1));
        ratimos_card_t tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }

    int pos = 0;
    for (int col = 0; col < RATIMOS_KLONDIKE_TABLEAU_COLS; col++) {
        for (int j = 0; j <= col; j++) {
            ratimos_card_t card = deck[pos++];
            bool face_up = (j == col);
            if (face_up) {
                card = (ratimos_card_t) (card | 0x40u);
            }
            out->tableau[col].cards[out->tableau[col].count] = card;
            out->tableau[col].count++;
        }
    }

    for (; pos < 52; pos++) {
        out->stock.cards[out->stock.count] = deck[pos];
        out->stock.count++;
    }

    out->seed = seed;
    out->daily = daily ? 1 : 0;
}
