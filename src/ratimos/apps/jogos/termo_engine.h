#ifndef RATIMOS_JOGOS_TERMO_ENGINE_H
#define RATIMOS_JOGOS_TERMO_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "termo_words.h"

#include "../../../storage/content_api.h"

/*
 * Motor do termo/dueto/quarteto (JOGOS-03) -- puro C, sem LVGL, exercitado
 * diretamente pela suite Unity (test/test_jogos_termo), igual a
 * sudoku_engine.h.
 *
 * A regra de feedback duas-passadas (greens-then-yellows) e o unico
 * requisito de corretude nao-negociavel deste motor: um guess com letra
 * duplicada nunca pode ser creditado de mais -- esse e o bug mais comum
 * documentado pelo RESEARCH em clones de Wordle, e por isso ganha os
 * primeiros 4+ testes dedicados da suite, escritos antes de qualquer outra
 * coisa.
 */

typedef enum {
    RATIMOS_TERMO_FEEDBACK_ABSENT = 0,
    RATIMOS_TERMO_FEEDBACK_PRESENT,
    RATIMOS_TERMO_FEEDBACK_CORRECT
} ratimos_termo_feedback_t;

typedef enum {
    RATIMOS_TERMO_MODE_TERMO = 0,
    RATIMOS_TERMO_MODE_DUETO,
    RATIMOS_TERMO_MODE_QUARTETO,
    RATIMOS_TERMO_MODE_COUNT
} ratimos_termo_mode_t;

#define RATIMOS_TERMO_MAX_BOARDS 4
#define RATIMOS_TERMO_MAX_TRIES  9

typedef struct {
    char answer[RATIMOS_TERMO_WORD_LEN + 1];
    uint8_t feedback[RATIMOS_TERMO_MAX_TRIES][RATIMOS_TERMO_WORD_LEN];
    uint8_t guesses_made;
    uint8_t solved;
} ratimos_termo_board_t;

/* `history` guarda os palpites compartilhados na ordem de submissao -- a
 * linha `feedback[i]` de cada board corresponde a `history[i]`. `finished`:
 * 0 = jogando, 1 = venceu (todos os boards resolvidos), 2 = tries
 * esgotados com pelo menos um board nao resolvido. */
typedef struct {
    ratimos_termo_mode_t mode;
    ratimos_termo_board_t boards[RATIMOS_TERMO_MAX_BOARDS];
    char history[RATIMOS_TERMO_MAX_TRIES][RATIMOS_TERMO_WORD_LEN + 1];
    uint8_t board_count;
    uint8_t max_tries;
    uint8_t tries_used;
    uint8_t finished;
    uint8_t daily_win_recorded;
    char current_guess[RATIMOS_TERMO_WORD_LEN + 1];
} ratimos_termo_state_t;

/* O estado serializado precisa caber no blob opaco da Storage API -- mesmo
 * padrao de sudoku_engine.h/conexo.c: se um plano futuro engordar a struct
 * alem do limite, o build quebra aqui em vez do save comecar a falhar
 * silenciosamente. */
_Static_assert(sizeof(ratimos_termo_state_t) <= RATIMOS_GAME_STATE_BLOB_SIZE,
               "ratimos_termo_state_t nao cabe no blob de save");

/* Regra duas-passadas duplicate-safe: primeiro marca os acertos de posicao
 * exata (consumindo a contagem de letras da resposta), so DEPOIS marca
 * presenca nas posicoes restantes, e so enquanto a contagem daquela letra
 * ainda for > 0. Um loop de uma passada so e o bug classico de
 * over-crediting documentado pelo RESEARCH -- proibido. */
void ratimos_termo_score_guess(const char * guess, const char * answer,
                                ratimos_termo_feedback_t out[RATIMOS_TERMO_WORD_LEN]);

/* Monta uma sessao diaria determinista: mesmo `day_index` + mesmo `mode`
 * sempre produz as mesmas respostas (sem nenhum acesso a rede/relogio --
 * quem chama passa o dia). Os `board_count` boards de uma mesma sessao
 * nunca repetem resposta entre si (reject-on-duplicate). */
void ratimos_termo_start_daily(ratimos_termo_state_t * out, ratimos_termo_mode_t mode, uint32_t day_index);

/* Submete `guess` (RATIMOS_TERMO_WORD_LEN caracteres, qualquer caixa) para
 * TODOS os boards ainda nao resolvidos da sessao, consumindo exatamente uma
 * tentativa do orcamento total. Rejeita (retorna false, SEM consumir
 * tentativa) quando a sessao ja terminou, o comprimento esta errado, ou a
 * palavra nao esta na lista de palpites aceitos. */
bool ratimos_termo_submit(ratimos_termo_state_t * st, const char * guess);

uint8_t ratimos_termo_board_count(ratimos_termo_mode_t mode);
uint8_t ratimos_termo_max_tries(ratimos_termo_mode_t mode);

#endif /* RATIMOS_JOGOS_TERMO_ENGINE_H */
