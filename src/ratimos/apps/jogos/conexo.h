#ifndef RATIMOS_JOGOS_CONEXO_H
#define RATIMOS_JOGOS_CONEXO_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#include "conexo_puzzles.h"
#include "puzzle_history.h"

/*
 * Conexo (JOGOS-05) — agrupar 16 palavras em 4 categorias escondidas.
 *
 * O motor (start/submit/shuffle) e separado da tela de proposito: e ele que a
 * suite Unity linka e exercita, sem precisar de LVGL, SDL nem de um tabuleiro
 * renderizado.
 */

#define RATIMOS_CONEXO_MAX_MISTAKES 4

/* 0xFF = slot ainda nao usado em solved_group_order. */
#define RATIMOS_CONEXO_NO_GROUP 0xFF

typedef struct {
    char puzzle_id[16];
    uint16_t puzzle_index;

    /* tile_word[slot] = id achatado da palavra (grupo * 4 + palavra) que esta
     * naquele slot do grid. Embaralhar so permuta ESTE array. */
    uint8_t tile_word[16];

    uint8_t selected_count;
    uint8_t selected[4];              /* indices de slot */

    uint8_t solved_group_order[4];    /* ids de grupo, na ordem em que foram resolvidos */
    uint8_t solved_count;

    uint8_t mistakes;
    uint8_t finished;                 /* 0 jogando / 1 venceu / 2 revelado */

    /* Evita contar a vitoria diaria duas vezes ao reentrar numa tela ja
     * vencida (persistido, entao sobrevive a um relaunch). */
    uint8_t daily_win_recorded;

    /* Anel de quebra-cabecas recentes (compartilhado com outros jogos de
     * banco): sobrevive ao save/restore para que "novo jogo" nao repita o
     * mesmo quebra-cabeca de imediato. */
    ratimos_puzzle_history_t history;
} ratimos_conexo_state_t;

typedef enum {
    RATIMOS_CONEXO_SUBMIT_REJECTED_INCOMPLETE = 0, /* menos de 4 selecionadas: nao conta erro */
    RATIMOS_CONEXO_SUBMIT_CORRECT,
    RATIMOS_CONEXO_SUBMIT_ONE_AWAY,                /* exatamente 3 do mesmo grupo */
    RATIMOS_CONEXO_SUBMIT_WRONG
} ratimos_conexo_submit_t;

void ratimos_conexo_start(ratimos_conexo_state_t * state, uint16_t puzzle_index, uint32_t seed);

ratimos_conexo_submit_t ratimos_conexo_submit(ratimos_conexo_state_t * state,
                                              const ratimos_conexo_puzzle_t * puzzle);

/* Permuta apenas os slots de grupos ainda NAO resolvidos. Nunca muda a que
 * grupo uma palavra pertence. */
void ratimos_conexo_shuffle(ratimos_conexo_state_t * state, uint32_t seed);

/* Registra a vitoria diaria (PROGRESSAO-01) EXATAMENTE uma vez por board
 * vencido: no-op (retorna false) se o jogo ainda nao foi vencido, se ja foi
 * revelado por derrota, ou se essa vitoria ja tinha sido contada antes --
 * reentrar numa tela ja vencida nunca soma o castelo de novo. Chama a
 * Storage API diretamente (sem LVGL), entao e testavel pela suite Unity sem
 * precisar renderizar nada. */
bool ratimos_conexo_record_win_if_needed(ratimos_conexo_state_t * state);

/* Tela do jogo (cache-once, igual a jogos_app.c). */
void ratimos_conexo_show(lv_event_t * e);

#endif
