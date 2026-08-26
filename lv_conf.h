/*
 * lv_conf.h — configuração mínima do LVGL para o RatimOS.
 *
 * Qualquer macro NÃO definida aqui recebe o valor padrão do LVGL
 * (via lv_conf_internal.h) — por isso este arquivo só lista os
 * pontos onde o RatimOS se desvia do padrão: profundidade de cor,
 * driver de simulação em SDL2 e as fontes usadas no design system.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

#define LV_USE_OS LV_OS_NONE

/* Driver de janela/mouse para rodar a UI no PC (Fase 0 do plano) */
#define LV_USE_SDL 1
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>
    #define LV_SDL_BUF_COUNT 1
    #define LV_SDL_FULLSCREEN 0
    #define LV_SDL_DIRECT_EXIT 1
#endif

/* Fontes do design system do RatimOS */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_BUILD_EXAMPLES 0

#endif /*LV_CONF_H*/
