#ifndef RATIMOS_THEME_H
#define RATIMOS_THEME_H

#include "lvgl.h"

/*
 * Paleta RatimOS — inspirada em handhelds retro (referência: colombiaOS),
 * mas com identidade própria: indigo profundo + acento teal, em vez do
 * azul/roxo puro da referência.
 */
#define RATIMOS_COLOR_BG            lv_color_hex(0x14102b)
#define RATIMOS_COLOR_PANEL         lv_color_hex(0x2b2358)
#define RATIMOS_COLOR_PANEL_ACTIVE  lv_color_hex(0x4a3f8f)
#define RATIMOS_COLOR_ACCENT        lv_color_hex(0x6dd6c4)
#define RATIMOS_COLOR_TEXT          lv_color_hex(0xf2eeff)
#define RATIMOS_COLOR_TEXT_MUTED    lv_color_hex(0xa89fd1)

#define RATIMOS_SCREEN_W 320
#define RATIMOS_SCREEN_H 480

void ratimos_theme_apply_screen(lv_obj_t * scr);
lv_obj_t * ratimos_panel_create(lv_obj_t * parent);
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * letter);

#endif
