#ifndef RATIMOS_JOGOS_CONEXO_PUZZLES_H
#define RATIMOS_JOGOS_CONEXO_PUZZLES_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Banco de quebra-cabecas do conexo (D-11).
 *
 * Nao existe conteudo PT-BR pronto e licenciavel para este jogo, entao o
 * banco e escrito a mao e compilado no firmware — nunca lido de diretorio.
 * Formato pensado para crescer: acrescentar um item ao array e a unica coisa
 * necessaria para publicar um quebra-cabeca novo, sem mudar codigo.
 *
 * Conteudo: portugues brasileiro do dia a dia, tudo minusculo e sem acento
 * (as fontes bitmap embutidas do LVGL so trazem ASCII), carinhoso no tom.
 */

typedef enum {
    RATIMOS_CONEXO_DIFF_YELLOW = 0,  /* mais facil */
    RATIMOS_CONEXO_DIFF_GREEN,
    RATIMOS_CONEXO_DIFF_BLUE,
    RATIMOS_CONEXO_DIFF_PURPLE       /* mais dificil */
} ratimos_conexo_difficulty_t;

typedef struct {
    char category_name[32];
    ratimos_conexo_difficulty_t difficulty;
    char words[4][16];
} ratimos_conexo_group_t;

typedef struct {
    char id[16];
    ratimos_conexo_group_t groups[4];
} ratimos_conexo_puzzle_t;

size_t ratimos_conexo_puzzle_count(void);

/* Copia o quebra-cabeca `index` para `out`. Retorna false (sem ler nada fora
 * do array) quando o indice esta fora do banco. */
bool ratimos_conexo_get_puzzle(size_t index, ratimos_conexo_puzzle_t * out);

#endif
