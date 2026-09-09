#ifndef RATIMOS_JOGOS_SUDOKU_ENGINE_H
#define RATIMOS_JOGOS_SUDOKU_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "../../../storage/content_api.h"

/*
 * Motor do sudoku (JOGOS-01) -- puro C, sem LVGL, para ser exercitado
 * diretamente pela suite Unity (test/test_jogos_sudoku).
 *
 * A checagem de unicidade (count_solutions, parar em 2) e o unico requisito
 * de corretude nao-negociavel deste jogo: um tabuleiro com zero ou multiplas
 * solucoes e um bug de presente irreversivel, nunca um detalhe de UX a
 * simplificar. Ver comentario em sudoku_engine.c sobre a contagem de pistas
 * como proxy de dificuldade -- essa, sim, e uma simplificacao aceita.
 */

typedef enum {
    RATIMOS_SUDOKU_FACIL = 0,
    RATIMOS_SUDOKU_MEDIO,
    RATIMOS_SUDOKU_DIFICIL,
    RATIMOS_SUDOKU_DIARIO,
    RATIMOS_SUDOKU_MODE_COUNT
} ratimos_sudoku_mode_t;

typedef struct {
    uint8_t given[9][9];      /* tabuleiro-pista imutavel (0 = vazio) */
    uint8_t filled[9][9];     /* entradas do jogador (0 = vazio) */
    uint8_t solution[9][9];   /* grade completa gerada pelo motor */
    ratimos_sudoku_mode_t mode;
    uint8_t cursor_row;
    uint8_t cursor_col;
    uint8_t solved;
    uint8_t daily_win_recorded;
} ratimos_sudoku_state_t;

/* O estado serializado precisa caber no blob opaco da Storage API -- se
 * algum plano futuro engordar a struct alem do limite, o build quebra aqui
 * em vez de o save comecar a falhar silenciosamente (mesmo padrao de
 * conexo.c). */
_Static_assert(sizeof(ratimos_sudoku_state_t) <= RATIMOS_GAME_STATE_BLOB_SIZE,
               "ratimos_sudoku_state_t nao cabe no blob de save");

/* Valida se `digit` (1-9) pode ocupar (row,col) sem duplicar em
 * linha/coluna/box, ignorando o proprio (row,col). */
bool ratimos_sudoku_is_valid_placement(const uint8_t board[9][9], int row, int col, uint8_t digit);

/* Solver contador que para assim que atinge `limit` solucoes -- unico jeito
 * aceito de provar unicidade (RESEARCH "Don't Hand-Roll": nunca substituir
 * por heuristica). Destroi e restaura `board` durante a busca (volta ao
 * estado original ao retornar). Um tabuleiro e unicamente soluvel
 * exatamente quando count_solutions(board, 2) == 1. */
int ratimos_sudoku_count_solutions(uint8_t board[9][9], int limit);

/* Gera um tabuleiro unicamente soluvel na faixa de pistas do `mode`, usando
 * um PRNG local semeado por `seed` (nunca o gerador pseudoaleatorio global da
 * libc -- os testes precisam ser deterministas e independentes de ordem de
 * chamada). Tenta ate
 * `max_attempts` vezes; retorna false e NAO toca em `*out` se exaurir o
 * orcamento sem convergir (nunca trava o loop de eventos do LVGL). */
bool ratimos_sudoku_generate(ratimos_sudoku_state_t * out, ratimos_sudoku_mode_t mode,
                              uint32_t seed, int max_attempts);

/* Atalho para o modo diario: mesmo dia (mesmo day_seed) sempre gera o mesmo
 * tabuleiro, sem nenhum acesso a rede. */
bool ratimos_sudoku_generate_daily(ratimos_sudoku_state_t * out, uint32_t day_seed);

/* Recusa escrever numa celula cuja pista (`given`) e nao-zero e recusa
 * digito acima de 9 (0 = apagar e permitido). */
bool ratimos_sudoku_set_cell(ratimos_sudoku_state_t * st, int row, int col, uint8_t digit);

/* Reporta se a entrada (pista ou jogador) em (row,col) duplica outra entrada
 * nao-zero na mesma linha/coluna/box, considerando pistas + preenchidas
 * combinadas. */
bool ratimos_sudoku_cell_conflicts(const ratimos_sudoku_state_t * st, int row, int col);

/* Verdadeiro apenas quando toda celula (pista ou jogador) esta preenchida e
 * nenhuma celula conflita. */
bool ratimos_sudoku_is_solved(const ratimos_sudoku_state_t * st);

#endif
