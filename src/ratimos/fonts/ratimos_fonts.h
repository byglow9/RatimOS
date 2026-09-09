#ifndef RATIMOS_FONTS_H
#define RATIMOS_FONTS_H

#include "lvgl.h"

/*
 * Fontes pixel/bitmap do tier Heading/Display (D-08) -- geradas por
 * tools/convert_title_font.sh a partir de Press Start 2P (SIL OFL 1.1).
 * Corpo de texto continua em Montserrat (LV_FONT_DEFAULT, lv_conf.h);
 * estas duas so' sao usadas para titulos de secao (16px) e banners de
 * celebracao/display (20px). LV_SYMBOL_* (icones) nunca usam estas
 * fontes -- ver comentario em status_bar.c.
 */
extern const lv_font_t ratimos_font_title_16;
extern const lv_font_t ratimos_font_title_20;

#endif
