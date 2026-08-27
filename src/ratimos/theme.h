#ifndef RATIMOS_THEME_H
#define RATIMOS_THEME_H

#include "lvgl.h"

/*
 * Paleta RatimOS — amostrada diretamente do logo oficial (logo/RatimOS.png,
 * decisão D-17): fundo quase-preto dominante, painéis violeta escuro,
 * acento vermelho-rook, textos com leve matiz violeta. Substitui a paleta
 * placeholder da Fase 0 (indigo/teal), aplicada em toda a UI (home, splash
 * e as 5 telas de app), não apenas na splash.
 */
#define RATIMOS_COLOR_BG            lv_color_hex(0x000000)
#define RATIMOS_COLOR_PANEL         lv_color_hex(0x2a123f)
#define RATIMOS_COLOR_PANEL_ACTIVE  lv_color_hex(0x4e2277)
#define RATIMOS_COLOR_ACCENT        lv_color_hex(0xe6010f)
#define RATIMOS_COLOR_TEXT          lv_color_hex(0xf5f2f8)
#define RATIMOS_COLOR_TEXT_MUTED    lv_color_hex(0xa997ba)

#define RATIMOS_SCREEN_W 320
#define RATIMOS_SCREEN_H 480

void ratimos_theme_apply_screen(lv_obj_t * scr);
lv_obj_t * ratimos_panel_create(lv_obj_t * parent);
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * letter);

#endif
