#include "splash.h"
#include "theme.h"
#include "logo_image.h"
#include "home_screen.h"
#include "../storage/content_api.h"

/*
 * Tabela de passos de boot (D-02/D-03): cada passo e trabalho REAL de
 * inicializacao da Storage API, um por tick do lv_timer -- nunca um timer
 * cosmetico. Extensivel: o plano 01-03 acrescenta as 4 entradas restantes
 * (index_photos/tracks/games/settings) abaixo das 2 ja existentes de 01-01;
 * o periodo do timer e derivado de SPLASH_TOTAL_MS / SPLASH_STEP_COUNT,
 * entao a duracao total do splash continua ~2s (D-02) sem tocar na logica
 * do timer abaixo.
 */
typedef struct {
    void (*fn)(void);
} splash_step_t;

static const splash_step_t s_steps[] = {
    { ratimos_storage_mount },
    { ratimos_storage_index_letters },
    { ratimos_storage_index_photos },
    { ratimos_storage_index_tracks },
    { ratimos_storage_index_games },
    { ratimos_storage_index_settings },
    { ratimos_storage_index_game_state },
};

#define SPLASH_STEP_COUNT ((int) (sizeof(s_steps) / sizeof(s_steps[0])))
#define SPLASH_TOTAL_MS 2000

/*
 * Barra de progresso retro (fix de checkpoint 02.1-14): moldura bevel 003-C
 * (borda escura 2px + friso claro, ratimos_bevel_apply) com blocos
 * quadrados discretos -- SPLASH_BLOCKS_PER_STEP blocos acendem de uma vez
 * a cada passo REAL de boot (D-02/D-03), sem animacao suave nem cantos
 * arredondados (antes era um lv_bar vermelho arredondado animado).
 */
#define SPLASH_BLOCKS_PER_STEP 3
#define SPLASH_BLOCK_COUNT (SPLASH_STEP_COUNT * SPLASH_BLOCKS_PER_STEP)
#define SPLASH_BLOCK_W 8
#define SPLASH_BLOCK_H 10
#define SPLASH_BLOCK_GAP 2
/* borda 2 + friso 1 + faixa escura 2 do bevel, +1 de respiro */
#define SPLASH_FRAME_INSET 6

static lv_obj_t * s_bar;
static int s_step_index = 0;

static void splash_bar_set_steps(int steps_done)
{
    int lit = steps_done * SPLASH_BLOCKS_PER_STEP;
    for (int i = 0; i < SPLASH_BLOCK_COUNT; i++) {
        lv_obj_t * block = lv_obj_get_child(s_bar, i);
        bool on = i < lit;
        lv_obj_set_style_bg_color(block, on ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_BG, 0);
        lv_obj_set_style_bg_opa(block, on ? LV_OPA_COVER : LV_OPA_60, 0);
    }
}

static lv_obj_t * splash_bar_create(lv_obj_t * parent)
{
    lv_obj_t * bar = lv_obj_create(parent);
    ratimos_bevel_apply(bar);
    lv_obj_set_style_pad_all(bar, SPLASH_FRAME_INSET - 2, 0); /* + borda 2 = inset */
    lv_obj_set_style_pad_column(bar, SPLASH_BLOCK_GAP, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(bar,
                    SPLASH_BLOCK_COUNT * SPLASH_BLOCK_W + (SPLASH_BLOCK_COUNT - 1) * SPLASH_BLOCK_GAP
                        + 2 * SPLASH_FRAME_INSET,
                    SPLASH_BLOCK_H + 2 * SPLASH_FRAME_INSET);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < SPLASH_BLOCK_COUNT; i++) {
        lv_obj_t * block = lv_obj_create(bar);
        lv_obj_remove_style_all(block);
        lv_obj_set_size(block, SPLASH_BLOCK_W, SPLASH_BLOCK_H);
        lv_obj_set_style_radius(block, 0, 0);
        lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    }
    return bar;
}

static void splash_step_cb(lv_timer_t * t)
{
    s_steps[s_step_index].fn();
    s_step_index++;

    splash_bar_set_steps(s_step_index);

    if (s_step_index >= SPLASH_STEP_COUNT) {
        lv_timer_delete(t);
        ratimos_home_screen_show(NULL);
    }
}

void ratimos_splash_show(void)
{
    s_step_index = 0;

    /* Boot com fundo preto liso (plano 02.1-14) -- sem o degrade
     * ditherizado de ratimos_theme_apply_screen(), que fica pras demais
     * telas. O logo (logo_image.c) tem fundo preto proprio, entao se
     * funde com a tela em vez de virar uma caixa sobre o degrade. */
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_top(scr, 24, 0);
    lv_obj_set_style_pad_bottom(scr, 24, 0);
    lv_obj_set_style_pad_row(scr, 16, 0);

    /* Logo (D-01): fade-in via lv_obj_fade_in, sem nenhum handler de toque
     * registrado -- D-04 e satisfeito simplesmente por nunca conectar
     * input nesta tela. */
    lv_obj_t * logo = lv_image_create(scr);
    lv_image_set_src(logo, &ratimos_logo_desc);
    lv_obj_set_style_opa(logo, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(logo, 2000, 0);

    /* Barra de progresso (D-02/D-03): blocos acesos por passo real,
     * atualizados pelo timer de passos -- nunca uma animacao decorativa. */
    s_bar = splash_bar_create(scr);
    splash_bar_set_steps(0);

    lv_screen_load(scr);

    lv_timer_create(splash_step_cb, SPLASH_TOTAL_MS / SPLASH_STEP_COUNT, NULL);
}
