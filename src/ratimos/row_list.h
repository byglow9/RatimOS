#ifndef RATIMOS_ROW_LIST_H
#define RATIMOS_ROW_LIST_H

#include "lvgl.h"

/*
 * Linha padrão de lista usada por vários apps (jogos, config, cartas,
 * música): selo + título + legenda, ocupando a largura toda. `click_cb`
 * pode ser NULL para uma linha não-clicável.
 *
 * `letter` é, apesar do nome herdado, um icon id resolvido primeiro via
 * ratimos_icon_by_id() (src/ratimos/icons.h) — numa correspondência, o selo
 * mostra o ícone de pixel art (VISUAL-01/D-07); em NULL ou sem
 * correspondência, cai para o selo original de círculo+letra (`letter` é
 * então tratado como o texto a exibir). Mesmo contrato de
 * ratimos_badge_create() (theme.h), que este helper delega diretamente.
 */
lv_obj_t * ratimos_row_create(lv_obj_t * parent,
                               const char * letter,
                               const char * title,
                               const char * subtitle,
                               lv_event_cb_t click_cb);

#endif
