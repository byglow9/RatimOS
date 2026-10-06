#include "splash.h"
#include "theme.h"
#include "logo_image.h"
#include "fonts/ratimos_fonts.h"
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
 * Barra de progresso retro (fixes de checkpoint 02.1-14, rodada 3):
 * moldura bevel 003-C (borda escura 2px + friso claro + faixa escura,
 * ratimos_bevel_apply) com RATIMOS_SPLASH_BLOCKS blocos pixel-art
 * desenhados num callback LV_EVENT_DRAW_MAIN_END da propria barra -- zero
 * objetos por bloco, custo de heap de um unico lv_obj.
 *
 * Cada bloco aceso tem a cor do degrade RatimOS na sua posicao (roxo ->
 * magenta -> rosa -> laranja, as paradas do fundo 001-C com o roxo inicial
 * clareado pra PANEL_ACTIVE, senao some no preto), 1px de brilho em cima e
 * 1px de sombra embaixo; blocos apagados sao fendas escuras.
 *
 * Preenchimento CONTINUO, um bloco por vez: um timer de animacao acende um
 * bloco a cada SPLASH_BLOCK_MS ate alcancar o ALVO, e o alvo vem SO' dos
 * passos reais de boot ja concluidos (D-02/D-03) -- a barra nunca passa do
 * progresso real. Depois do ultimo passo ela completa 100% e so' entao a
 * home abre.
 */
#define SPLASH_BLOCK_W 7
#define SPLASH_BLOCK_H 14
#define SPLASH_BLOCK_GAP 1
#define SPLASH_BLOCK_MS 60
/* borda 2 + friso 1 + faixa escura 2 do bevel, +1 de respiro */
#define SPLASH_FRAME_INSET 6

static lv_obj_t * s_bar;
static lv_obj_t * s_pct_label;
static lv_timer_t * s_step_timer;
static lv_timer_t * s_fill_timer;
static int s_step_index = 0;
static int s_lit_blocks = 0;

static int target_blocks(void)
{
    return s_step_index * RATIMOS_SPLASH_BLOCKS / SPLASH_STEP_COUNT;
}

uint8_t ratimos_splash_blocks_lit(void)
{
    return (uint8_t) s_lit_blocks;
}

uint8_t ratimos_splash_blocks_target(void)
{
    return (uint8_t) target_blocks();
}

/* Cor do degrade RatimOS na fracao t (0..255) -- 4 paradas, 3 segmentos. */
static lv_color_t gradient_at(int32_t t)
{
    static const uint32_t stops[4] = { 0x4e2277, 0x7a1560, 0xc81f4c, 0xe8630f };
    int32_t seg = t * 3 / 256;
    if (seg > 2) {
        seg = 2;
    }
    int32_t local = t * 3 - seg * 256; /* 0..255 dentro do segmento */
    return lv_color_mix(lv_color_hex(stops[seg + 1]), lv_color_hex(stops[seg]), (lv_opa_t) local);
}

static void fill_rect(lv_layer_t * layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t c)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = c;
    d.bg_opa = LV_OPA_COVER;
    d.border_width = 0;
    lv_area_t a = { x1, y1, x2, y2 };
    lv_draw_rect(layer, &d, &a);
}

static void splash_bar_draw_cb(lv_event_t * e)
{
    lv_obj_t * bar = lv_event_get_target(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t c;
    lv_obj_get_coords(bar, &c);
    int32_t y1 = c.y1 + SPLASH_FRAME_INSET;
    int32_t y2 = y1 + SPLASH_BLOCK_H - 1;

    for (int i = 0; i < RATIMOS_SPLASH_BLOCKS; i++) {
        int32_t x1 = c.x1 + SPLASH_FRAME_INSET + i * (SPLASH_BLOCK_W + SPLASH_BLOCK_GAP);
        int32_t x2 = x1 + SPLASH_BLOCK_W - 1;
        if (i < s_lit_blocks) {
            lv_color_t base = gradient_at(i * 255 / (RATIMOS_SPLASH_BLOCKS - 1));
            fill_rect(layer, x1, y1, x2, y2, base);
            fill_rect(layer, x1, y1, x2, y1, lv_color_mix(lv_color_white(), base, LV_OPA_40));
            fill_rect(layer, x1, y2, x2, y2, lv_color_mix(lv_color_black(), base, LV_OPA_50));
        } else {
            /* Fenda apagada: escura, com a borda de cima mais escura ainda
             * (parece afundada na moldura). */
            fill_rect(layer, x1, y1, x2, y2, lv_color_hex(0x1a0b29));
            fill_rect(layer, x1, y1, x2, y1, lv_color_hex(0x0d0515));
        }
    }
}

static void update_pct_label(void)
{
    lv_label_set_text_fmt(s_pct_label, "carregando %03d%%", s_lit_blocks * 100 / RATIMOS_SPLASH_BLOCKS);
}

static void splash_fill_cb(lv_timer_t * t)
{
    if (s_lit_blocks < target_blocks()) {
        s_lit_blocks++;
        update_pct_label();
        lv_obj_invalidate(s_bar);
    }
    if (s_step_index >= SPLASH_STEP_COUNT && s_lit_blocks >= RATIMOS_SPLASH_BLOCKS) {
        lv_timer_delete(t);
        s_fill_timer = NULL;
        ratimos_home_screen_show(NULL);
    }
}

static lv_obj_t * splash_bar_create(lv_obj_t * parent)
{
    lv_obj_t * bar = lv_obj_create(parent);
    ratimos_bevel_apply(bar);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_size(bar,
                    RATIMOS_SPLASH_BLOCKS * SPLASH_BLOCK_W + (RATIMOS_SPLASH_BLOCKS - 1) * SPLASH_BLOCK_GAP
                        + 2 * SPLASH_FRAME_INSET,
                    SPLASH_BLOCK_H + 2 * SPLASH_FRAME_INSET);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(bar, splash_bar_draw_cb, LV_EVENT_DRAW_MAIN_END, NULL);
    return bar;
}

static void splash_step_cb(lv_timer_t * t)
{
    s_steps[s_step_index].fn();
    s_step_index++;

    if (s_step_index >= SPLASH_STEP_COUNT) {
        /* Ultimo passo real: o timer de preenchimento completa 100% e
         * abre a home (splash_fill_cb). */
        lv_timer_delete(t);
        s_step_timer = NULL;
    }
}

void ratimos_splash_show(void)
{
    s_step_index = 0;
    s_lit_blocks = 0;

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
    lv_obj_set_style_pad_row(scr, 12, 0);

    /* Logo (D-01): fade-in via lv_obj_fade_in, sem nenhum handler de toque
     * registrado -- D-04 e satisfeito simplesmente por nunca conectar
     * input nesta tela. */
    lv_obj_t * logo = lv_image_create(scr);
    lv_image_set_src(logo, &ratimos_logo_desc);
    lv_obj_set_style_opa(logo, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(logo, 2000, 0);

    /* Barra de progresso (D-02/D-03) + rotulo de percentual: o alvo vem
     * dos passos reais; o timer de preenchimento so' anda atras dele. */
    s_bar = splash_bar_create(scr);

    s_pct_label = lv_label_create(scr);
    lv_obj_set_style_text_font(s_pct_label, &ratimos_font_title_8, 0);
    lv_obj_set_style_text_color(s_pct_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    update_pct_label();

    lv_screen_load(scr);

    s_step_timer = lv_timer_create(splash_step_cb, SPLASH_TOTAL_MS / SPLASH_STEP_COUNT, NULL);
    s_fill_timer = lv_timer_create(splash_fill_cb, SPLASH_BLOCK_MS, NULL);
}
