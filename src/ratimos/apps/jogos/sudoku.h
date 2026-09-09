#ifndef RATIMOS_JOGOS_SUDOKU_H
#define RATIMOS_JOGOS_SUDOKU_H

#include "lvgl.h"

/*
 * Sudoku (JOGOS-01) -- tela do jogo (cache-once, igual a jogos_app.c /
 * conexo.c). O motor (sudoku_engine.h) e quem a suite Unity exercita; este
 * arquivo so cuida de LVGL + persistencia via Storage API.
 */
void ratimos_sudoku_show(lv_event_t * e);

#endif
