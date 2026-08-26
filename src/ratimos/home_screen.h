#ifndef RATIMOS_HOME_SCREEN_H
#define RATIMOS_HOME_SCREEN_H

#include "lvgl.h"

/* Compatível com lv_event_cb_t (usado como callback de "voltar" pelos apps).
 * `e` pode ser NULL — é ignorado, só serve pra carregar a tela inicial. */
void ratimos_home_screen_show(lv_event_t * e);

#endif
