#include "splash.h"
#include "app_shell.h"
#include "boot_styles.h"
#include "fonts/ratimos_fonts.h"
#include "home_screen.h"
#include "theme.h"
#include "theme_catalog.h"
#include "../storage/content_api.h"

/*
 * Tabela de passos de boot (D-02/D-03): cada passo e trabalho REAL de
 * inicializacao da Storage API, um por tick do lv_timer -- nunca um timer
 * cosmetico. O periodo do timer e derivado de SPLASH_TOTAL_MS /
 * SPLASH_STEP_COUNT, entao a duracao total do splash continua ~2s (D-02).
 * Cada passo tem o rotulo mostrado enquanto ele roda (rotulo do passo REAL).
 */
typedef struct {
    void (*fn)(void);
    const char * label;
} splash_step_t;

static const splash_step_t s_steps[] = {
    { ratimos_storage_mount, "montando cartão sd..." },
    { ratimos_storage_index_letters, "verificando cartas novas..." },
    { ratimos_storage_index_photos, "carregando fotos..." },
    { ratimos_storage_index_tracks, "carregando músicas..." },
    { ratimos_storage_index_games, "preparando jogos..." },
    { ratimos_storage_index_settings, "lendo configurações..." },
    { ratimos_storage_index_game_state, "abrindo o castelo..." },
};

#define SPLASH_STEP_COUNT ((int) (sizeof(s_steps) / sizeof(s_steps[0])))
#define SPLASH_TOTAL_MS 2000
/* Periodo do tick que acende 1 unidade (so' enquanto shown < alvo). */
#define SPLASH_UNIT_MS 16
#define SPLASH_WAVE_MS 260
#define SPLASH_BG_TICK_MS 50
/* Segura o "pronto!" um instante antes de abrir a proxima tela. */
#define SPLASH_DONE_HOLD_MS 400
/* Fade-in do boot (Fase 1 D-01, ~400ms aprovado pela usuaria). */
#define SPLASH_FADE_MS 400
/* Sombra do texto: preto a ~75%, deslocada (+1,+1) -- o `text()` do sketch 010. */
#define SPLASH_SHADOW_PCT 75

/*
 * A tela tem EXATAMENTE 5 filhos fixos e nenhum lv_image:
 *   0 stage (320x480 sem estilo; LV_EVENT_DRAW_MAIN_END desenha fundo +
 *     carregamento -- nenhum objeto por quadro)
 *   1 sombra do percentual   2 percentual
 *   3 sombra do passo        4 passo
 */
static lv_obj_t * s_screen;
static lv_obj_t * s_stage;
static lv_obj_t * s_pct_shadow;
static lv_obj_t * s_pct;
static lv_obj_t * s_step_shadow;
static lv_obj_t * s_step;

static lv_timer_t * s_step_timer;
static lv_timer_t * s_fill_timer;
static lv_timer_t * s_wave_timer;
static lv_timer_t * s_bg_timer;
static lv_timer_t * s_hold_timer;

static int s_step_index;
static int32_t s_units;
static bool s_real_boot;
static bool s_done;
static uint8_t s_wave_phase;
static lv_opa_t s_fade_opa;
static uint32_t s_bg_prev_ms;
static void (*s_on_done)(void);

static ratimos_theme_id_t s_theme_id = RATIMOS_THEME_DEFAULT;
static ratimos_boot_style_t s_style = RATIMOS_BOOT_STYLE_DEFAULT;

static ratimos_splash_bg_fn_t s_bg_override;
static uint32_t s_fade_ms = SPLASH_FADE_MS;

/* ---------------------------------------------------------------------- *
 * Modelo de progresso
 * ---------------------------------------------------------------------- */

static int32_t target_units(void)
{
    return (int32_t) s_step_index * RATIMOS_SPLASH_UNITS / SPLASH_STEP_COUNT;
}

uint8_t ratimos_splash_units_shown(void)
{
    return (uint8_t) s_units;
}

uint8_t ratimos_splash_units_target(void)
{
    return (uint8_t) target_units();
}

ratimos_theme_id_t ratimos_splash_active_theme(void)
{
    return s_theme_id;
}

ratimos_boot_style_t ratimos_splash_active_style(void)
{
    return s_style;
}

void ratimos_splash_set_background(ratimos_splash_bg_fn_t fn)
{
    s_bg_override = fn;
}

void ratimos_splash_set_fade_ms_for_test(uint32_t ms)
{
    s_fade_ms = ms;
}

/* ---------------------------------------------------------------------- *
 * Fundo (etapa SEPARADA e trocavel) e desenho do stage
 * ---------------------------------------------------------------------- */

/* Etapa de fundo: chamada ANTES do carregamento, desenha o boot_bg do tema
 * (ou o seam de teste). Qualquer apagamento futuro usa esta funcao, nunca uma
 * cor fixa. */
static void splash_draw_background(lv_layer_t * layer, const lv_area_t * area)
{
    if (s_bg_override != NULL) {
        s_bg_override(layer, area);
        return;
    }
    ratimos_theme_boot_bg_draw(ratimos_theme_get(s_theme_id), layer, area, lv_tick_get());
}

static void stage_draw_cb(lv_event_t * e)
{
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target(e), &area);

    splash_draw_background(layer, &area);

    ratimos_boot_frame_t frame = {
        .units = s_units,
        .units_total = RATIMOS_SPLASH_UNITS,
        .done = s_done,
        .wave_phase = s_wave_phase,
        .opa = s_fade_opa,
        .draw_background = splash_draw_background,
    };
    ratimos_boot_style_draw(s_style, layer, &frame);
}

/* ---------------------------------------------------------------------- *
 * Textos
 * ---------------------------------------------------------------------- */

static void apply_text_opa(void)
{
    lv_opa_t shadow = (lv_opa_t) ((int) s_fade_opa * SPLASH_SHADOW_PCT / 100);
    if (s_pct) {
        lv_obj_set_style_text_opa(s_pct, s_fade_opa, 0);
        lv_obj_set_style_text_opa(s_pct_shadow, shadow, 0);
        lv_obj_set_style_text_opa(s_step, s_fade_opa, 0);
        lv_obj_set_style_text_opa(s_step_shadow, shadow, 0);
    }
}

static void set_pair_text(lv_obj_t * shadow, lv_obj_t * label, const char * text)
{
    lv_label_set_text(shadow, text);
    lv_label_set_text(label, text);
}

static void update_pct_text(void)
{
    char buf[32];
    ratimos_boot_style_format_pct(s_style, s_units, buf, sizeof(buf));
    set_pair_text(s_pct_shadow, s_pct, buf);
}

static void update_step_text(void)
{
    /* Rotulo do passo REAL em andamento; depois do ultimo, mantem o ultimo
     * ate o "pronto!". */
    int idx = s_step_index < SPLASH_STEP_COUNT ? s_step_index : SPLASH_STEP_COUNT - 1;
    lv_label_set_text_static(s_step_shadow, s_steps[idx].label);
    lv_label_set_text_static(s_step, s_steps[idx].label);
}

static lv_obj_t * text_label_create(lv_obj_t * parent, const lv_font_t * font, lv_color_t color, int32_t x,
                                    int32_t y)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(l, RATIMOS_SCREEN_W);
    lv_obj_set_pos(l, x, y);
    return l;
}

/* ---------------------------------------------------------------------- *
 * Fade-in (~400ms): opa dos draws do carregamento + text_opa dos labels.
 * NUNCA opa de objeto no stage (criaria uma camada de tela cheia). O fundo
 * nao esmaece.
 * ---------------------------------------------------------------------- */

static void fade_exec_cb(void * var, int32_t v)
{
    (void) var;
    s_fade_opa = (lv_opa_t) v;
    apply_text_opa();
    if (s_stage) {
        lv_obj_invalidate(s_stage);
    }
}

/* ---------------------------------------------------------------------- *
 * Timers
 * ---------------------------------------------------------------------- */

static void delete_timer(lv_timer_t ** t)
{
    if (*t != NULL) {
        lv_timer_delete(*t);
        *t = NULL;
    }
}

static void stop_all_timers(void)
{
    delete_timer(&s_step_timer);
    delete_timer(&s_fill_timer);
    delete_timer(&s_wave_timer);
    delete_timer(&s_bg_timer);
    delete_timer(&s_hold_timer);
}

static void splash_step_cb(lv_timer_t * t)
{
    if (s_real_boot && s_step_index < SPLASH_STEP_COUNT) {
        s_steps[s_step_index].fn(); /* trabalho REAL; a previa nao executa nada */
    }
    s_step_index++;
    update_step_text();

    if (s_step_index >= SPLASH_STEP_COUNT) {
        /* Ultimo passo: o timer de preenchimento completa e abre a proxima tela. */
        lv_timer_delete(t);
        s_step_timer = NULL;
    }
}

static void splash_hold_cb(lv_timer_t * t)
{
    lv_timer_delete(t);
    s_hold_timer = NULL;

    void (*cb)(void) = s_on_done;
    s_on_done = NULL;
    /* O boot acabou: nada mais deve animar ou invalidar. */
    stop_all_timers();
    if (cb != NULL) {
        cb(); /* normalmente ratimos_screen_load(...), que deleta esta tela */
    }
}

static void splash_finish(void)
{
    s_done = true;
    /* "pronto!" UMA vez, so' na linha do percentual; a linha do passo some. */
    set_pair_text(s_pct_shadow, s_pct, "pronto!");
    lv_obj_add_flag(s_step, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_step_shadow, LV_OBJ_FLAG_HIDDEN);

    delete_timer(&s_fill_timer);
    s_hold_timer = lv_timer_create(splash_hold_cb, SPLASH_DONE_HOLD_MS, NULL);
}

static void splash_fill_cb(lv_timer_t * t)
{
    (void) t;
    if (s_units < target_units()) {
        int32_t before = s_units;
        s_units++;
        update_pct_text();
        lv_area_t dirty;
        if (ratimos_boot_style_dirty_area(s_style, before, s_units, &dirty)) {
            lv_obj_invalidate_area(s_stage, &dirty);
        }
    }
    if (s_step_index >= SPLASH_STEP_COUNT && s_units >= RATIMOS_SPLASH_UNITS) {
        splash_finish();
    }
}

static void splash_wave_cb(lv_timer_t * t)
{
    (void) t;
    s_wave_phase ^= 1u;
    lv_area_t a;
    if (ratimos_boot_style_wave_area(s_style, s_done, &a)) {
        lv_obj_invalidate_area(s_stage, &a);
    }
}

static void splash_bg_cb(lv_timer_t * t)
{
    (void) t;
    uint32_t now = lv_tick_get();
    ratimos_theme_boot_bg_tick(ratimos_theme_get(s_theme_id), s_bg_prev_ms, now, s_stage);
    s_bg_prev_ms = now;
}

/* ---------------------------------------------------------------------- *
 * Ciclo de vida
 * ---------------------------------------------------------------------- */

static void screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    /* A tela e' deletada pelo mecanismo do 02.1-15 (ratimos_screen_load da
     * proxima tela). Aqui so' limpa: timers, animacao e ponteiros. */
    stop_all_timers();
    if (s_stage) {
        lv_anim_delete(s_stage, fade_exec_cb);
    }
    s_screen = NULL;
    s_stage = NULL;
    s_pct_shadow = NULL;
    s_pct = NULL;
    s_step_shadow = NULL;
    s_step = NULL;
    s_on_done = NULL;
}

static void go_home_cb(void)
{
    ratimos_home_screen_show(NULL);
}

static void splash_play(ratimos_theme_id_t theme, ratimos_boot_style_t style, bool real_boot,
                        void (*on_done)(void))
{
    /* Uma splash por vez: se uma anterior sobrou, solta seus timers. */
    stop_all_timers();

    s_theme_id = ((int) theme >= 0 && (int) theme < (int) RATIMOS_THEME_COUNT) ? theme : RATIMOS_THEME_DEFAULT;
    s_style = ((int) style >= 0 && (int) style < (int) RATIMOS_BOOT_STYLE_COUNT) ? style
                                                                                   : RATIMOS_BOOT_STYLE_DEFAULT;
    s_real_boot = real_boot;
    s_on_done = on_done;
    s_step_index = 0;
    s_units = 0;
    s_done = false;
    s_wave_phase = 0;
    s_fade_opa = s_fade_ms == 0 ? LV_OPA_COVER : LV_OPA_TRANSP;
    s_bg_prev_ms = lv_tick_get();

    /* Tela preta opaca, sem flex, sem imagem. O fundo do tema e' desenhado
     * pelo stage (etapa separada), nao por um objeto. */
    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_screen, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_screen, screen_deleted_cb, LV_EVENT_DELETE, NULL);

    s_stage = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_stage);
    lv_obj_set_size(s_stage, RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_obj_set_pos(s_stage, 0, 0);
    lv_obj_clear_flag(s_stage, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_stage, stage_draw_cb, LV_EVENT_DRAW_MAIN_END, NULL);

    /* Textos embaixo do carregamento: percentual (Press Start 2P 8) e rotulo
     * do passo real (JetBrains Mono 10), cada um com sombra de 1px. A sombra
     * vem ANTES do texto na ordem dos filhos (desenha por baixo). */
    int32_t y = ratimos_boot_style_text_y(s_style);
    lv_color_t black = lv_color_black();
    s_pct_shadow = text_label_create(s_screen, &ratimos_font_title_8, black, 1, y + 1);
    s_pct = text_label_create(s_screen, &ratimos_font_title_8, RATIMOS_COLOR_BOOT_TEXT, 0, y);
    s_step_shadow = text_label_create(s_screen, &ratimos_font_mono_10, black, 1, y + 17);
    s_step = text_label_create(s_screen, &ratimos_font_mono_10, RATIMOS_COLOR_TEXT_MUTED, 0, y + 16);
    update_pct_text();
    update_step_text();
    apply_text_opa();

    /* Nenhum handler de toque (Fase 1 D-04): o boot nunca aceita entrada. */
    ratimos_screen_load(s_screen);

    if (s_fade_ms > 0) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, s_stage);
        lv_anim_set_exec_cb(&a, fade_exec_cb);
        lv_anim_set_values(&a, 0, LV_OPA_COVER);
        lv_anim_set_duration(&a, s_fade_ms);
        lv_anim_start(&a);
    }

    s_step_timer = lv_timer_create(splash_step_cb, SPLASH_TOTAL_MS / SPLASH_STEP_COUNT, NULL);
    s_fill_timer = lv_timer_create(splash_fill_cb, SPLASH_UNIT_MS, NULL);
    s_wave_timer = lv_timer_create(splash_wave_cb, SPLASH_WAVE_MS, NULL);
    if (ratimos_theme_boot_bg_is_animated(ratimos_theme_get(s_theme_id))) {
        s_bg_timer = lv_timer_create(splash_bg_cb, SPLASH_BG_TICK_MS, NULL);
    }
}

void ratimos_splash_show(void)
{
    /* Tema e carregamento sao lidos da Storage API ANTES do primeiro passo:
     * nao dependem do mount do SD (no dispositivo ficam em NVS). */
    splash_play(ratimos_storage_get_theme_id(), ratimos_storage_get_boot_style(), true, go_home_cb);
}

void ratimos_splash_preview(ratimos_theme_id_t theme, ratimos_boot_style_t style, void (*on_done)(void))
{
    splash_play(theme, style, false, on_done);
}
