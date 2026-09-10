#ifndef RATIMOS_JOGOS_KLONDIKE_ENGINE_H
#define RATIMOS_JOGOS_KLONDIKE_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "../../../storage/content_api.h"

/*
 * Motor do paciencia/klondike (JOGOS-01) -- puro C, sem LVGL, espelhando o
 * split motor/tela ja estabelecido em sudoku_engine.h/conexo.c (02.1-01/04).
 *
 * A legalidade de uma jogada NUNCA e decidida em semantica livre de
 * arrastar-e-soltar: cada tipo de movimento (enum abaixo) tem sua propria
 * funcao auxiliar dentro de ratimos_klondike_can_move(), testavel
 * isoladamente -- aceitar uma jogada ilegal em silencio e o bug classico de
 * clone de paciencia (RESEARCH "Don't Hand-Roll") e essa arquitetura torna
 * isso estruturalmente impossivel, nao apenas improvavel.
 */

/*
 * Carta empacotada num unico byte, para que o baralho inteiro caiba
 * confortavelmente no blob de save de 512 bytes: bits 0-3 = valor (1-13),
 * bits 4-5 = naipe (0-3), bit 6 = flag de face para cima. Valor 0 = nenhuma
 * carta (usado para preencher slots vazios de pilha, nunca aparece como
 * carta real ja que valor minimo e 1).
 */
typedef uint8_t ratimos_card_t;

#define RATIMOS_SUIT_CLUBS    0
#define RATIMOS_SUIT_DIAMONDS 1
#define RATIMOS_SUIT_HEARTS   2
#define RATIMOS_SUIT_SPADES   3

static inline uint8_t ratimos_card_rank(ratimos_card_t c)
{
    return (uint8_t) (c & 0x0Fu);
}

static inline uint8_t ratimos_card_suit(ratimos_card_t c)
{
    return (uint8_t) ((c >> 4) & 0x03u);
}

/*
 * Vermelho = ouros ou copas. Deliberadamente NAO usa um atalho de paridade
 * de bit sobre o naipe (`suit & 1`) -- com a ordenacao clubs=0/diamonds=1/
 * hearts=2/spades=3 esse atalho classificaria copas (2) como preta, o que
 * esta errado. A cor e checada explicitamente contra os dois naipes
 * vermelhos em vez disso.
 */
static inline bool ratimos_card_is_red(ratimos_card_t c)
{
    uint8_t suit = ratimos_card_suit(c);
    return suit == RATIMOS_SUIT_DIAMONDS || suit == RATIMOS_SUIT_HEARTS;
}

static inline bool ratimos_card_is_face_up(ratimos_card_t c)
{
    return (c & 0x40u) != 0;
}

static inline ratimos_card_t ratimos_card_make(uint8_t suit, uint8_t rank, bool face_up)
{
    return (ratimos_card_t) (((suit & 0x03u) << 4) | (rank & 0x0Fu) | (face_up ? 0x40u : 0u));
}

#define RATIMOS_KLONDIKE_TABLEAU_COLS 7
#define RATIMOS_KLONDIKE_FOUNDATIONS  4
/* Precisa cobrir o pior caso real, nao so a coluna mais longa do tableau:
 * o estoque comeca a partida com 24 cartas (52 - as 28 distribuidas no
 * tableau), e uma reciclagem tardia pode devolver ate 24 ao estoque de
 * novo -- um teto de 20 (o numero considerado inicialmente so pela
 * sequencia maxima teorica de uma coluna) estoura o buffer do estoque no
 * deal inicial. 32 cobre estoque/descarte (24) e a coluna do tableau mais
 * longa plausivel com folga. */
#define RATIMOS_KLONDIKE_MAX_PILE     32

typedef struct {
    ratimos_card_t cards[RATIMOS_KLONDIKE_MAX_PILE];
    uint8_t count;
} ratimos_pile_t;

typedef struct {
    ratimos_pile_t tableau[RATIMOS_KLONDIKE_TABLEAU_COLS];
    ratimos_pile_t foundation[RATIMOS_KLONDIKE_FOUNDATIONS];
    ratimos_pile_t stock;
    ratimos_pile_t waste;
    uint32_t seed;
    uint16_t moves;
    uint8_t won;
    uint8_t daily;
    uint8_t daily_win_recorded;
} ratimos_klondike_state_t;

/* O estado serializado precisa caber no blob opaco da Storage API -- se
 * algum plano futuro engordar a struct alem do limite, o build quebra aqui
 * em vez de o save comecar a falhar silenciosamente (mesmo padrao de
 * sudoku_engine.h). Com RATIMOS_KLONDIKE_MAX_PILE=32 o tamanho real fica
 * bem abaixo do teto de 512 bytes, entao nao ha necessidade de reduzir a
 * capacidade das pilhas. */
_Static_assert(sizeof(ratimos_klondike_state_t) <= RATIMOS_GAME_STATE_BLOB_SIZE,
               "ratimos_klondike_state_t nao cabe no blob de save");

/* Vocabulario explicito de jogadas -- nunca semantica livre de arrastar. */
typedef enum {
    RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU = 0,
    RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION,
    RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU,
    RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION,
    RATIMOS_KLONDIKE_MOVE_FOUNDATION_TO_TABLEAU,
    RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE,
    RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE
} ratimos_klondike_move_kind_t;

/* Quantas cartas o estoque vira por toque -- draw-1, a variante mais simples
 * de operar numa tela pequena via toque (RESEARCH nao trava um numero
 * especifico; draw-1 e a escolha deste plano). */
#define RATIMOS_KLONDIKE_DRAW_COUNT 1

/* Monta um baralho novo de 52 cartas embaralhado por Fisher-Yates a partir
 * de um PRNG local semeado por `seed` (nunca o gerador pseudoaleatorio
 * global da libc), depois distribui 1..7 cartas pelas sete colunas do
 * tableau (so a ultima de cada coluna vira para cima) e poe o restante no
 * estoque, com descarte e fundacoes vazios. `daily` so marca a flag de
 * modo diario no estado -- quem decide QUAL seed usar para o dia e o
 * chamador (via ratimos_daily_seed()), nunca esta funcao. */
void ratimos_klondike_deal(ratimos_klondike_state_t * out, uint32_t seed, bool daily);

/* Decide se a jogada `kind` de `from_index` para `to_index` movendo
 * `card_count` cartas e legal, sem tocar em `st`. O significado de
 * from_index/to_index/card_count depende de `kind`:
 *   TABLEAU_TO_TABLEAU:    from/to = colunas do tableau (0-6); card_count = tamanho da sequencia movida
 *   TABLEAU_TO_FOUNDATION: from = coluna do tableau; to = fundacao (0-3); card_count ignorado (sempre 1)
 *   WASTE_TO_TABLEAU:      to = coluna do tableau; from/card_count ignorados
 *   WASTE_TO_FOUNDATION:   to = fundacao (0-3); from/card_count ignorados
 *   FOUNDATION_TO_TABLEAU: from = fundacao (0-3); to = coluna do tableau; card_count ignorado
 *   STOCK_TO_WASTE:        todos os indices ignorados
 *   RECYCLE_WASTE:         todos os indices ignorados
 */
bool ratimos_klondike_can_move(const ratimos_klondike_state_t * st, ratimos_klondike_move_kind_t kind,
                                uint8_t from_index, uint8_t to_index, uint8_t card_count);

/* Chama ratimos_klondike_can_move() primeiro e retorna false SEM tocar um
 * unico byte de `st` quando a jogada e ilegal. Numa jogada legal, move as
 * cartas, incrementa `moves`, vira automaticamente a carta agora exposta no
 * topo de uma coluna de origem do tableau, e recalcula `won`. */
bool ratimos_klondike_move(ratimos_klondike_state_t * st, ratimos_klondike_move_kind_t kind,
                            uint8_t from_index, uint8_t to_index, uint8_t card_count);

/* Verdadeiro exatamente quando as quatro fundacoes tem treze cartas cada. */
bool ratimos_klondike_is_won(const ratimos_klondike_state_t * st);

#endif
