#ifndef RATIMOS_JOGOS_TERMO_H
#define RATIMOS_JOGOS_TERMO_H

#include "lvgl.h"

/*
 * Termo/Dueto/Quarteto (JOGOS-03) -- tela do jogo (cache-once, igual a
 * sudoku.c/conexo.c). O motor (termo_engine.h) e quem a suite Unity
 * exercita; este arquivo so cuida de LVGL + persistencia via Storage API.
 */
void ratimos_termo_show(lv_event_t * e);

#endif
