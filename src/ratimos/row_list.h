#ifndef RATIMOS_ROW_LIST_H
#define RATIMOS_ROW_LIST_H

#include "lvgl.h"

/*
 * Linha padrão de lista usada por vários apps (jogos, config, cartas,
 * música): selo com letra + título + legenda, ocupando a largura toda.
 * `click_cb` pode ser NULL para uma linha não-clicável.
 */
lv_obj_t * ratimos_row_create(lv_obj_t * parent,
                               const char * letter,
                               const char * title,
                               const char * subtitle,
                               lv_event_cb_t click_cb);

#endif
