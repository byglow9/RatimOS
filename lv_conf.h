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

/*
 * LVGL's builtin allocator default (LV_MEM_SIZE, when not overridden here)
 * is 64KB — sized for tiny embedded demos, not a retained multi-screen app.
 * This project's app/screen files (src/ratimos/apps/*.c) build a brand-new
 * lv_obj_t screen on every visit with no delete-on-navigate-away, so heap
 * usage grows unboundedly across a session; confirmed via reproduction that
 * 64KB is exhausted (`lv_realloc: couldn't reallocate memory`) after only
 * ~3-4 repeated visits to a single app screen. native_sim runs on a PC with
 * abundant RAM, so a generous bump here costs nothing and buys headroom for
 * the remaining Phase 1 plans (more storage domains, more rendered rows).
 * NOTE: this value is native_sim-only — Phase 3's esp32s3 environment must
 * define its own hardware-measured LV_MEM_SIZE (and the screen-retention
 * leak itself should get a proper fix, e.g. cache-or-delete-on-navigate,
 * before shipping to real hardware with real RAM limits).
 */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (512 * 1024U)

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
