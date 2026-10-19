#ifndef RATIMOS_THEME_ART_H
#define RATIMOS_THEME_ART_H

/* GERADO por tools/generate_theme_art.py -- nao editar a mao. */
#include <stdint.h>
#include "lvgl.h"

/* Noite estrelada (boot): faixas, 70 estrelas piscando e a lua. */
#define RATIMOS_NOITE_STAR_COUNT 70
#define RATIMOS_NOITE_BAND0 0x07030f /* y 0..159   */
#define RATIMOS_NOITE_BAND1 0x0b0518 /* y 160..319 */
#define RATIMOS_NOITE_BAND2 0x110822 /* y 320..479 */
#define RATIMOS_NOITE_BIG_CROSS 0x6a4f8e
#define RATIMOS_NOITE_STAR_PEAK 0xf5f2f8 /* fase 3 */
#define RATIMOS_NOITE_MOON_X 249
#define RATIMOS_NOITE_MOON_Y 43
typedef struct {
    int16_t x;
    int16_t y;
    uint16_t period_ms; /* 500 + rnd*900 */
    uint8_t big;        /* ganha cruz nas fases >= 2 */
    uint32_t col;       /* 0xrrggbb nas fases 1 e 2 */
} ratimos_star_t;
extern const ratimos_star_t ratimos_noite_stars[RATIMOS_NOITE_STAR_COUNT];
extern const lv_image_dsc_t ratimos_noite_moon_desc;

/* Castelo ao entardecer (boot): as janelas que apagam sao repintadas em runtime. */
#define RATIMOS_CASTELO_WINDOW_COUNT 5
#define RATIMOS_CASTELO_WIN_W 4
#define RATIMOS_CASTELO_WIN_H 6
#define RATIMOS_CASTELO_WIN_ON 0xffc04a
#define RATIMOS_CASTELO_WIN_OFF 0x7a4a10
#define RATIMOS_CASTELO_WIN_TOP 0x1f0a1c
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t i; /* indice da janela no padrao (floor(now/800) + i) % 5 */
} ratimos_window_t;
extern const ratimos_window_t ratimos_castelo_windows[RATIMOS_CASTELO_WINDOW_COUNT];

/* Xadrez: cores do tabuleiro (chessRender do 010). */
#define RATIMOS_XADREZ_A   { 44, 14, 70 }
#define RATIMOS_XADREZ_B   { 12, 4, 22 }
#define RATIMOS_XADREZ_M   { 28, 9, 46 }
#define RATIMOS_XADREZ_SKY0 { 6, 2, 12 }
#define RATIMOS_XADREZ_SKY1 { 40, 14, 64 }
#define RATIMOS_XADREZ_LINE 0x4e2277

extern const lv_image_dsc_t ratimos_home_bg_noite_desc;
extern const lv_image_dsc_t ratimos_home_bg_castelo_desc;
extern const lv_image_dsc_t ratimos_home_bg_xadrez_desc;
extern const lv_image_dsc_t ratimos_boot_bg_castelo_desc;
extern const lv_image_dsc_t ratimos_thumb_home_classico_desc;
extern const lv_image_dsc_t ratimos_thumb_home_noite_desc;
extern const lv_image_dsc_t ratimos_thumb_home_castelo_desc;
extern const lv_image_dsc_t ratimos_thumb_home_xadrez_desc;
extern const lv_image_dsc_t ratimos_thumb_bootbg_classico_desc;
extern const lv_image_dsc_t ratimos_thumb_bootbg_noite_desc;
extern const lv_image_dsc_t ratimos_thumb_bootbg_castelo_desc;
extern const lv_image_dsc_t ratimos_thumb_bootbg_xadrez_desc;

#endif
