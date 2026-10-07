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
 * is 64KB — sized for tiny embedded demos, not a multi-screen app.
 *
 * Screen retention (plan 02.1-15, deferred-items #1 — FIXED): app/game
 * screens used to be built once and cached forever, and together they
 * exhausted this 512KB heap in one "visit every game" session. Now only the
 * home screen stays cached; every other screen is deleted when the user
 * navigates away (ratimos_screen_load(), src/ratimos/app_shell.h) and rebuilt
 * from storage on the next visit. Peak use is now home + the jogos list +
 * the heaviest single screen (see test/test_navigation_memory, which
 * measures it with ratimos_heap_used(), src/ratimos/heap_probe.h).
 *
 * NOTE: this value is native_sim-only — Phase 3's esp32s3 environment must
 * define its own hardware-measured LV_MEM_SIZE, sized against the peak that
 * test_navigation_memory prints ("session peak").
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

/*
 * Tipografia do sketch 003-C (plano 02.1-14): corpo/chrome em JetBrains
 * Mono (ratimos_font_mono_10/11/12) e titulos de linha em Press Start 2P 8px
 * (ratimos_font_title_8); ratimos_font_title_16/_20 continuam pros banners/
 * titulos de secao (D-08). Todas geradas por tools/convert_title_font.sh,
 * const (ficam na flash, nao no heap do LVGL).
 *
 * LV_FONT_DEFAULT e' a mono 12. Ela NAO tem os glifos LV_SYMBOL_* (area de
 * uso privado do FontAwesome): as tres mono tem `.fallback =
 * &lv_font_montserrat_14`, entao qualquer LV_SYMBOL_* num label mono cai
 * na Montserrat built-in em vez de virar caixa vazia. Por isso
 * LV_FONT_MONTSERRAT_14 acima precisa continuar ligado.
 */
#define LV_FONT_CUSTOM_DECLARE \
    LV_FONT_DECLARE(ratimos_font_mono_10) \
    LV_FONT_DECLARE(ratimos_font_mono_11) \
    LV_FONT_DECLARE(ratimos_font_mono_12) \
    LV_FONT_DECLARE(ratimos_font_title_8) \
    LV_FONT_DECLARE(ratimos_font_title_16) \
    LV_FONT_DECLARE(ratimos_font_title_20)

#define LV_FONT_DEFAULT &ratimos_font_mono_12

/* lv_font_conv emite bitmaps de glifo comprimidos por padrao (menor
 * footprint em flash, o que importa no ESP32-S3 real) -- sem isto os
 * dois fonts acima falham silenciosamente ao desenhar cada glifo
 * ("Couldn't get the bitmap of a glyph"). */
#define LV_USE_FONT_COMPRESSED 1

/*
 * Assert do LVGL ABORTA em vez do `while(1);` padrao (plano 02.1-14): um
 * esgotamento de heap (lv_realloc NULL -> LV_ASSERT_MALLOC) congelava o
 * simulador e travou uma suite de teste por ~2h girando a 100% de CPU. Com
 * abort() o simulador sai com SIGABRT e o `pio test` falha na hora. O ESP32
 * (Fase 3) define o seu (reset/log), este lv_conf e' o do native_sim.
 */
#define LV_ASSERT_HANDLER_INCLUDE <stdlib.h>
#define LV_ASSERT_HANDLER abort();

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_BUILD_EXAMPLES 0

#endif /*LV_CONF_H*/
