#ifndef RATIMOS_FONTS_H
#define RATIMOS_FONTS_H

#include "lvgl.h"

/*
 * Fontes proprias do RatimOS, todas geradas por tools/convert_title_font.sh
 * (SIL OFL 1.1, nao editar os .c a mao):
 *
 * - ratimos_font_mono_10/11/12: JetBrains Mono -- corpo/chrome do sketch
 *   003-C (plano 02.1-14). 12 e' o LV_FONT_DEFAULT (lv_conf.h); 10 = marca
 *   do topbar e descricoes de linha; 11 = breadcrumb da sectionbar. Tem
 *   `.fallback = &lv_font_montserrat_14`, entao LV_SYMBOL_* continua
 *   renderizando num label mono.
 * - ratimos_font_title_8: Press Start 2P 8px -- titulos de linha (003-C).
 * - ratimos_font_title_16/_20: Press Start 2P -- titulos de secao e
 *   banners de celebracao/display (D-08).
 */
extern const lv_font_t ratimos_font_mono_10;
extern const lv_font_t ratimos_font_mono_11;
extern const lv_font_t ratimos_font_mono_12;
extern const lv_font_t ratimos_font_title_8;
extern const lv_font_t ratimos_font_title_16;
extern const lv_font_t ratimos_font_title_20;

#endif
