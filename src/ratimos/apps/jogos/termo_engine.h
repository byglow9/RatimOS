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
    /* Linha de entrada em andamento, POSICIONAL (plano 02.1-17, imitando o
     * term.ooo): os bytes 0..4 sao 'a'..'z' ou 0 (casa vazia) e o byte 5 e'
     * sempre 0. So' o que a jogadora DIGITOU -- nunca pre-preenchida pelo
     * jogo. O tamanho da struct nao mudou: saves antigos (prefixo "pe" +
     * zeros, que ja e' posicional, e o formato do 2c292eb, que pode ter
     * letras em outras casas -- inofensivo) continuam carregando. */
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

/*
 * Estado de cada letra a-z pro teclado (fix de checkpoint 02.1-14, como no
 * Termo real). Considera so' os boards AINDA EM JOGO (nao resolvidos); se
 * todos ja foram resolvidos, considera todos.
 * - CORRECT: a letra ja saiu verde em algum desses boards (vence PRESENT);
 * - PRESENT: ja saiu amarela em algum deles;
 * - ABSENT: ja foi tentada e nao existe na resposta de NENHUM deles --
 *   a tecla fica desabilitada;
 * - UNUSED: nunca tentada (ou tentada mas ainda possivel, ex. letra repetida).
 */
typedef enum {
    RATIMOS_TERMO_KEY_UNUSED = 0,
    RATIMOS_TERMO_KEY_ABSENT,
    RATIMOS_TERMO_KEY_PRESENT,
    RATIMOS_TERMO_KEY_CORRECT
} ratimos_termo_key_state_t;

void ratimos_termo_key_states(const ratimos_termo_state_t * st, uint8_t out[26]);

/*
 * Linha de entrada posicional (plano 02.1-17). Notacao: `_` = casa vazia.
 * `cursor` e' o indice 0..4 da casa selecionada, ou RATIMOS_TERMO_CURSOR_END
 * quando a linha esta cheia / nao ha casa selecionada.
 */
#define RATIMOS_TERMO_CURSOR_END RATIMOS_TERMO_WORD_LEN

/* Maiuscula vira minuscula; qualquer byte fora de a-z vira 0 (casa vazia); o
 * byte 5 vira 0. Nunca le alem de 6 bytes. */
void ratimos_termo_row_sanitize(char row[RATIMOS_TERMO_WORD_LEN + 1]);

bool ratimos_termo_row_is_full(const char * row);

/* Primeira casa vazia, ou RATIMOS_TERMO_CURSOR_END se a linha esta cheia. */
uint8_t ratimos_termo_row_first_empty(const char * row);

/* Digita `letter` (a-z/A-Z; outro caractere nao faz nada) na casa do cursor,
 * sobrescrevendo. Cursor END usa a 1a vazia; linha cheia com cursor END nao
 * muda nada. Devolve o novo cursor: a 1a vazia em cursor+1..4; senao a 1a
 * vazia em 0..cursor-1; senao END. */
uint8_t ratimos_termo_row_type(char * row, uint8_t cursor, char letter);

/* Apaga: se a casa do cursor esta cheia, limpa e mantem o cursor; senao limpa
 * a casa cheia mais proxima a ESQUERDA (a partir da 4 quando o cursor e' END)
 * e devolve a posicao dela; se nao ha nenhuma, nao faz nada. */
uint8_t ratimos_termo_row_erase(char * row, uint8_t cursor);

typedef enum {
    RATIMOS_TERMO_SUBMIT_OK = 0,
    RATIMOS_TERMO_SUBMIT_INCOMPLETE,
    RATIMOS_TERMO_SUBMIT_NOT_ACCEPTED,
    RATIMOS_TERMO_SUBMIT_FINISHED
} ratimos_termo_submit_result_t;

/* Envia st->current_guess. Ordem das recusas: FINISHED (partida encerrada),
 * INCOMPLETE (linha nao cheia), NOT_ACCEPTED (fora da lista). Em toda recusa
 * a linha fica intacta e nenhuma tentativa e' gasta. No OK chama
 * ratimos_termo_submit e ZERA a linha. */
ratimos_termo_submit_result_t ratimos_termo_submit_row(ratimos_termo_state_t * st);

/* Valida/normaliza um estado vindo do disco (T-02.1-02/T-02.1-44): false se
 * mode/finished sao invalidos ou se a resposta de algum dos board_count
 * tabuleiros nao tem 5 letras a-z; senao recalcula board_count/max_tries,
 * limita tries_used, forca NUL no byte 5 de todo campo de string, normaliza
 * o historico e a linha de entrada. Nunca le fora dos buffers, mesmo com
 * bytes corrompidos. */
bool ratimos_termo_state_sanitize(ratimos_termo_state_t * st);

uint8_t ratimos_termo_board_count(ratimos_termo_mode_t mode);
uint8_t ratimos_termo_max_tries(ratimos_termo_mode_t mode);

#endif /* RATIMOS_JOGOS_TERMO_ENGINE_H */
