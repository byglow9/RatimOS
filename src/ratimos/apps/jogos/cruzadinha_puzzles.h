/*
 * GERADO por tools/generate_crossword.py a partir de
 * assets/crosswords/wordlists.json -- nao editar a mao. Reexecute o script
 * se a lista de palavras/dicas fonte mudar (ver assets/crosswords/README.md).
 */

#ifndef RATIMOS_JOGOS_CRUZADINHA_PUZZLES_H
#define RATIMOS_JOGOS_CRUZADINHA_PUZZLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Banco de quebra-cabecas do cruzadinha (D-11/JOGOS-04).
 *
 * Layout de grid: linha/coluna 0-based, is_across=true para uma entrada
 * horizontal (esquerda->direita) e false para uma entrada vertical
 * (cima->baixo). clue_number segue a convencao padrao de numeracao de
 * cruzadinha -- uma celula que inicia tanto uma entrada horizontal quanto
 * vertical compartilha o mesmo numero entre as duas.
 */
#define RATIMOS_CRUZADINHA_MAX_WORDS 16
#define RATIMOS_CRUZADINHA_MAX_DIM 11

typedef struct {
    uint8_t row;
    uint8_t col;
    uint8_t length;
    bool is_across;
    uint8_t clue_number;
    char clue_text[96];
    char answer[16];
} ratimos_cruzadinha_word_t;

typedef struct {
    char id[16];
    uint8_t grid_w;
    uint8_t grid_h;
    uint8_t word_count;
    ratimos_cruzadinha_word_t words[RATIMOS_CRUZADINHA_MAX_WORDS];
} ratimos_cruzadinha_puzzle_t;

size_t ratimos_cruzadinha_puzzle_count(void);

/* Copia o quebra-cabeca `index` para `out`. Retorna false (sem ler nada
 * fora do array) quando o indice esta fora do banco. */
bool ratimos_cruzadinha_get_puzzle(size_t index, ratimos_cruzadinha_puzzle_t * out);

#endif /* RATIMOS_JOGOS_CRUZADINHA_PUZZLES_H */
