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
};

#define SPLASH_STEP_COUNT ((int) (sizeof(s_steps) / sizeof(s_steps[0])))
#define SPLASH_TOTAL_MS 2000

static lv_obj_t * s_bar;
static int s_step_index = 0;

static void splash_step_cb(lv_timer_t * t)
{
    s_steps[s_step_index].fn();
    s_step_index++;

    lv_bar_set_value(s_bar, (s_step_index * 100) / SPLASH_STEP_COUNT, LV_ANIM_ON);

    if (s_step_index >= SPLASH_STEP_COUNT) {
        lv_timer_delete(t);
        ratimos_home_screen_show(NULL);
    }
}

void ratimos_splash_show(void)
{
    s_step_index = 0;

    lv_obj_t * scr = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr);
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

    /* Barra de progresso (D-02/D-03): valor real, atualizado pelo timer de
     * passos abaixo -- nunca uma animacao decorativa de duracao fixa. */
    s_bar = lv_bar_create(scr);
    lv_obj_set_size(s_bar, 200, 8);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, RATIMOS_COLOR_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, RATIMOS_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_screen_load(scr);

    lv_timer_create(splash_step_cb, SPLASH_TOTAL_MS / SPLASH_STEP_COUNT, NULL);
}
