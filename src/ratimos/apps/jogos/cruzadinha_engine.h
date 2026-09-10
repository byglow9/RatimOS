#ifndef RATIMOS_JOGOS_CRUZADINHA_ENGINE_H
#define RATIMOS_JOGOS_CRUZADINHA_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "cruzadinha_puzzles.h"
#include "puzzle_history.h"

#include "../../../storage/content_api.h"

/*
 * Motor do cruzadinha (JOGOS-04) -- puro C, sem LVGL, exercitado diretamente
 * pela suite Unity (test/test_jogos_cruzadinha), igual a sudoku_engine.h/
 * termo_engine.h.
 *
 * A numeracao das dicas e SEMPRE derivada da geometria das celulas aqui
 * (ratimos_cruzadinha_number_grid), nunca confiada do `clue_number` gravado
 * pelo gerador -- isso torna um eventual descompasso entre os dois
 * detectavel por teste (o teste de integridade do banco compara os dois).
 */

typedef struct {
    char letters[RATIMOS_CRUZADINHA_MAX_DIM][RATIMOS_CRUZADINHA_MAX_DIM];
    uint8_t numbers[RATIMOS_CRUZADINHA_MAX_DIM][RATIMOS_CRUZADINHA_MAX_DIM];
    uint8_t is_cell[RATIMOS_CRUZADINHA_MAX_DIM][RATIMOS_CRUZADINHA_MAX_DIM];
} ratimos_cruzadinha_grid_t;

/* Indice de entrada "nenhuma" -- usado por entry_at (quando uma direcao nao
 * existe naquela celula) e por next_unsolved (quando tudo ja foi resolvido). */
#define RATIMOS_CRUZADINHA_NO_ENTRY 0xFF

typedef struct {
    uint16_t puzzle_index;
    char entered[RATIMOS_CRUZADINHA_MAX_DIM][RATIMOS_CRUZADINHA_MAX_DIM];
    uint8_t cursor_row;
    uint8_t cursor_col;
    uint8_t active_entry;
    uint8_t active_is_across;
    uint8_t complete;
    uint8_t daily_win_recorded;
    ratimos_puzzle_history_t history;
} ratimos_cruzadinha_state_t;

/* O estado serializado precisa caber no blob opaco da Storage API -- mesmo
 * padrao de sudoku_engine.h/termo_engine.h: se um plano futuro engordar a
 * struct alem do limite, o build quebra aqui em vez do save comecar a
 * falhar silenciosamente. Se 11x11 mais o historico algum dia estourar,
 * reduza RATIMOS_CRUZADINHA_MAX_DIM para 9 e registre o motivo aqui -- nao
 * cresca o blob. */
_Static_assert(sizeof(ratimos_cruzadinha_state_t) <= RATIMOS_GAME_STATE_BLOB_SIZE,
               "ratimos_cruzadinha_state_t nao cabe no blob de save");

/* Preenche `out->is_cell`/`letters` a partir das palavras de `pz`, depois
 * roda a passada de numeracao padrao (varredura esquerda->direita,
 * cima->baixo; uma celula recebe o proximo numero se inicia uma entrada
 * horizontal e/ou vertical, com uma celula que inicia as duas
 * compartilhando um unico numero) -- ver tools/generate_crossword.py para a
 * mesma regra do lado do gerador. */
void ratimos_cruzadinha_number_grid(const ratimos_cruzadinha_puzzle_t * pz, ratimos_cruzadinha_grid_t * out);

/* Bounds-checka (row, col) contra grid_h/grid_w. Fora dos limites: retorna
 * false, sem tocar em *out_across/*out_down. Dentro dos limites: retorna
 * true e escreve o indice da entrada horizontal e/ou vertical que cobre a
 * celula em *out_across/*out_down, ou RATIMOS_CRUZADINHA_NO_ENTRY em cada
 * uma que nao existir ali (incluindo quando a celula nem e uma celula de
 * letra). */
bool ratimos_cruzadinha_entry_at(const ratimos_cruzadinha_puzzle_t * pz, uint8_t row, uint8_t col,
                                  uint8_t * out_across, uint8_t * out_down);

/* Escreve `letter` em (row, col) de `st->entered`. Aceita uma letra ASCII
 * A-Z (qualquer caixa, normalizada para maiuscula) ou ' '/0 para apagar a
 * celula. Recusa (retorna false, SEM mutar nada) uma coordenada fora dos
 * limites, uma coordenada que nao e celula de letra, ou um caractere que
 * nao e letra/espaco/nulo. */
bool ratimos_cruzadinha_set_letter(const ratimos_cruzadinha_puzzle_t * pz, ratimos_cruzadinha_state_t * st,
                                    uint8_t row, uint8_t col, char letter);

/* Verdadeiro quando toda celula da entrada `entry_index` em `st->entered`
 * bate com a resposta daquela entrada. Um indice fora de [0, word_count)
 * retorna false. */
bool ratimos_cruzadinha_entry_is_solved(const ratimos_cruzadinha_puzzle_t * pz,
                                         const ratimos_cruzadinha_state_t * st, uint8_t entry_index);

/* Verdadeiro somente quando TODA entrada do quebra-cabeca esta resolvida. */
bool ratimos_cruzadinha_is_complete(const ratimos_cruzadinha_puzzle_t * pz, const ratimos_cruzadinha_state_t * st);

/* Percorre as entradas em ordem de clue_number (a ordem do array `words`,
 * ja emitida nessa ordem pelo gerador) a partir de `from_entry` (exclusive),
 * dando a volta, e retorna o indice da primeira ainda nao resolvida.
 * `from_entry` >= word_count (incluindo RATIMOS_CRUZADINHA_NO_ENTRY) comeca
 * a busca do indice 0. Retorna RATIMOS_CRUZADINHA_NO_ENTRY quando tudo ja
 * esta resolvido. */
uint8_t ratimos_cruzadinha_next_unsolved(const ratimos_cruzadinha_puzzle_t * pz,
                                          const ratimos_cruzadinha_state_t * st, uint8_t from_entry);

#endif /* RATIMOS_JOGOS_CRUZADINHA_ENGINE_H */
