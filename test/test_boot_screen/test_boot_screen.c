/*
 * Suite da tela de boot (plano 02.1-16): modelo de progresso, fundo do tema
 * como etapa separada, carregamentos e textos. LVGL headless (display 320x480
 * com buffer FULL lido direto depois de lv_refr_now -- mesmo harness do
 * test_theme/test_castelo_render), sem SDL.
 *
 * main() faz mkdtemp + chdir ANTES de qualquer chamada: a splash real roda os
 * passos da Storage API e o settings.bin vive em assets/save/ -- nada disto
 * toca o assets/save/ do desenvolvedor.
 *
 * Variavel de ambiente RATIMOS_DUMP_DIR=<dir>: grava cada quadro amostrado
 * como PPM (ferramenta de conferencia visual, nao afeta o resultado).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unity.h>

#include "ratimos/app_shell.h"
#include "ratimos/bg_images.h"
#include "ratimos/boot_art.h"
#include "ratimos/boot_styles.h"
#include "ratimos/fonts/ratimos_fonts.h"
#include "ratimos/heap_probe.h"
#include "ratimos/logo_image.h"
#include "ratimos/splash.h"
#include "ratimos/theme.h"
#include "ratimos/theme_art.h"
#include "ratimos/theme_catalog.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

/* ------------------------------------------------------------------ *
 * Helpers
 * ------------------------------------------------------------------ */

static void pump_ms(int ms)
{
    for (int t = 0; t < ms; t += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

static uint16_t px565(int32_t x, int32_t y)
{
    size_t idx = ((size_t) y * (size_t) RATIMOS_SCREEN_W + (size_t) x) * 2u;
    return (uint16_t) (s_disp_buf[idx] | (uint16_t) (s_disp_buf[idx + 1] << 8));
}

static void rgb565_to_rgb8(uint16_t v, int rgb[3])
{
    int r = (v >> 11) & 0x1f, g = (v >> 5) & 0x3f, b = v & 0x1f;
    rgb[0] = (r << 3) | (r >> 2);
    rgb[1] = (g << 2) | (g >> 4);
    rgb[2] = (b << 3) | (b >> 2);
}

static uint16_t desc_px565(const lv_image_dsc_t * d, int32_t x, int32_t y)
{
    const uint8_t * p = d->data + (size_t) y * d->header.stride + (size_t) x * 2;
    return (uint16_t) (p[0] | (p[1] << 8));
}

static int abs_i(int v) { return v < 0 ? -v : v; }

static bool near_rgb(const int a[3], const int b[3], int tol)
{
    return abs_i(a[0] - b[0]) <= tol && abs_i(a[1] - b[1]) <= tol && abs_i(a[2] - b[2]) <= tol;
}

/* Fundo esperado do Classico no boot: bg_dither com preto a dim_pct por cima. */
static void expected_classic_bg(int32_t x, int32_t y, int dim_pct, int rgb[3])
{
    rgb565_to_rgb8(desc_px565(&ratimos_bg_dither_desc, x, y), rgb);
    int op = dim_pct * 255 / 100;
    for (int i = 0; i < 3; i++) {
        rgb[i] = rgb[i] * (255 - op) / 255;
    }
}

/* O pixel da tela e' "o fundo" (dentro de tolerancia de quantizacao RGB565)? */
static bool is_classic_bg(int32_t x, int32_t y)
{
    int got[3], want[3];
    rgb565_to_rgb8(px565(x, y), got);
    expected_classic_bg(x, y, 55, want);
    return near_rgb(got, want, 17);
}

/* Quantos pixels do retangulo NAO sao o fundo do Classico (= tem algo desenhado). */
static int count_non_bg(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int n = 0;
    for (int32_t y = y1; y <= y2; y++) {
        for (int32_t x = x1; x <= x2; x++) {
            if (!is_classic_bg(x, y)) {
                n++;
            }
        }
    }
    return n;
}

static void maybe_dump(const char * name)
{
    const char * dir = getenv("RATIMOS_DUMP_DIR");
    if (dir == NULL) {
        return;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    FILE * f = fopen(path, "wb");
    if (!f) {
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    for (int y = 0; y < RATIMOS_SCREEN_H; y++) {
        for (int x = 0; x < RATIMOS_SCREEN_W; x++) {
            int c[3];
            rgb565_to_rgb8(px565(x, y), c);
            fputc(c[0], f);
            fputc(c[1], f);
            fputc(c[2], f);
        }
    }
    fclose(f);
}

static void refresh(const char * dump_name)
{
    lv_refr_now(NULL);
    if (dump_name) {
        maybe_dump(dump_name);
    }
}

/* Troca a tela ativa por uma qualquer, deletando a splash (e soltando os
 * timers dela pelo LV_EVENT_DELETE). */
static void leave_splash(void)
{
    lv_obj_t * s = lv_obj_create(NULL);
    ratimos_screen_load(s);
}

static const char * s_settings_path = "assets/save/settings.bin";

static int count_timers(void)
{
    int n = 0;
    for (lv_timer_t * t = lv_timer_get_next(NULL); t != NULL; t = lv_timer_get_next(t)) {
        n++;
    }
    return n;
}

/* Estado capturado no instante do on_done de uma previa. */
static struct {
    bool called;
    char pct_text[32];
    bool step_hidden;
    bool step_shadow_hidden;
    uint32_t children;
    int visible_pronto;
} s_done_info;

static void done_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    s_done_info.called = true;
    s_done_info.children = lv_obj_get_child_count(scr);
    snprintf(s_done_info.pct_text, sizeof(s_done_info.pct_text), "%s",
             lv_label_get_text(lv_obj_get_child(scr, 2)));
    s_done_info.step_shadow_hidden = lv_obj_has_flag(lv_obj_get_child(scr, 3), LV_OBJ_FLAG_HIDDEN);
    s_done_info.step_hidden = lv_obj_has_flag(lv_obj_get_child(scr, 4), LV_OBJ_FLAG_HIDDEN);
    s_done_info.visible_pronto = 0;
    for (uint32_t i = 1; i < lv_obj_get_child_count(scr); i += 2) { /* so' os textos, nao as sombras */
        lv_obj_t * l = lv_obj_get_child(scr, (int32_t) i + 1);
        if (l && !lv_obj_has_flag(l, LV_OBJ_FLAG_HIDDEN) && strcmp(lv_label_get_text(l), "pronto!") == 0) {
            s_done_info.visible_pronto++;
        }
    }
    leave_splash();
}

static void solid_green_bg(lv_layer_t * layer, const lv_area_t * area)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = lv_color_hex(0x00ff00);
    d.bg_opa = LV_OPA_COVER;
    d.border_width = 0;
    lv_draw_rect(layer, &d, area);
}


static void solid_black_bg(lv_layer_t * layer, const lv_area_t * area)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = lv_color_hex(0x000000);
    d.bg_opa = LV_OPA_COVER;
    d.border_width = 0;
    lv_draw_rect(layer, &d, area);
}

/* Bombeia ate o progresso exibido alcancar `units` (ou passar dele). */
static void pump_until_units(int units)
{
    int guard = 0;
    while (ratimos_splash_units_shown() < units && guard++ < 800) {
        pump_ms(10);
    }
}

static void rgb8_of(int32_t x, int32_t y, int rgb[3])
{
    rgb565_to_rgb8(px565(x, y), rgb);
}

static bool px_is(int32_t x, int32_t y, uint32_t hex, int tol)
{
    int got[3], want[3] = { (hex >> 16) & 0xff, (hex >> 8) & 0xff, hex & 0xff };
    rgb8_of(x, y, got);
    return near_rgb(got, want, tol);
}

static int count_non_color(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t hex, int tol)
{
    int n = 0;
    for (int32_t y = y1; y <= y2; y++) {
        for (int32_t x = x1; x <= x2; x++) {
            if (!px_is(x, y, hex, tol)) {
                n++;
            }
        }
    }
    return n;
}

static uint8_t desc_alpha(const lv_image_dsc_t * d, int32_t x, int32_t y)
{
    const uint8_t * a8 = d->data + (size_t) d->header.w * d->header.h * 2;
    return a8[y * d->header.w + x];
}

/* O primeiro pixel (varredura por linha) do sprite que e' transparente. */
static bool find_alpha0(const lv_image_dsc_t * d, int32_t * ox, int32_t * oy)
{
    for (uint32_t y = 0; y < d->header.h; y++) {
        for (uint32_t x = 0; x < d->header.w; x++) {
            if (desc_alpha(d, (int32_t) x, (int32_t) y) == 0) {
                *ox = (int32_t) x;
                *oy = (int32_t) y;
                return true;
            }
        }
    }
    return false;
}

/* Cor 8-bit do sprite (RGB565 expandido). */
static void desc_rgb8(const lv_image_dsc_t * d, int32_t x, int32_t y, int rgb[3])
{
    rgb565_to_rgb8(desc_px565(d, x, y), rgb);
}

void setUp(void)
{
    remove(s_settings_path);
    memset(&s_done_info, 0, sizeof(s_done_info));
    ratimos_splash_set_background(NULL);
    ratimos_splash_set_fade_ms_for_test(0); /* quadro 1 ja com opa cheia (deterministico) */
    ratimos_theme_catalog_override_for_test(NULL, 0);
}

void tearDown(void)
{
    leave_splash(); /* nunca deixa splash/timer vivo para o proximo teste */
    ratimos_splash_set_background(NULL);
}

/* ------------------------------------------------------------------ *
 * Testes
 * ------------------------------------------------------------------ */

void test_without_settings_the_boot_is_classico_and_bandeira(void)
{
    ratimos_splash_show();
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_splash_active_theme());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_splash_active_style());
}

void test_boot_reads_the_two_settings_independently_before_the_first_step(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_XADREZ));
    TEST_ASSERT_TRUE(ratimos_storage_set_boot_style(RATIMOS_BOOT_STYLE_TORRES));
    ratimos_splash_show(); /* nenhum passo rodou ainda */
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_splash_units_shown());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_splash_active_theme());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_TORRES, ratimos_splash_active_style());
}

void test_progress_model_invariants_hold_every_tick_and_home_waits_for_pronto(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, done_cb);

    uint8_t prev = 0;
    bool saw_partial = false;
    int guard = 0;
    while (!s_done_info.called && guard++ < 600) {
        pump_ms(10);
        if (s_done_info.called) {
            break;
        }
        uint8_t shown = ratimos_splash_units_shown();
        uint8_t target = ratimos_splash_units_target();
        TEST_ASSERT_TRUE_MESSAGE(shown <= target, "shown ran ahead of the real-step target");
        TEST_ASSERT_TRUE_MESSAGE(shown - prev <= 1, "shown advanced more than 1 unit in a tick");
        int32_t rs = ratimos_boot_flag_rise_px(shown);
        TEST_ASSERT_TRUE(rs <= ratimos_boot_flag_rise_px(target));
        TEST_ASSERT_TRUE(rs % 2 == 0 && rs >= 0 && rs <= 64);
        if (rs == 64) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(RATIMOS_SPLASH_UNITS, target,
                                            "flag reached the top before the last real step");
        }
        if (shown > 0 && shown < RATIMOS_SPLASH_UNITS) {
            saw_partial = true;
        }
        prev = shown;
    }
    TEST_ASSERT_TRUE(saw_partial);
    TEST_ASSERT_TRUE_MESSAGE(s_done_info.called, "on_done never called");
    /* o on_done so' acontece depois do "pronto!" */
    TEST_ASSERT_EQUAL_STRING("pronto!", s_done_info.pct_text);
}

void test_flag_rise_is_a_pure_even_function_of_progress(void)
{
    TEST_ASSERT_EQUAL_INT32(0, ratimos_boot_flag_rise_px(0));
    TEST_ASSERT_EQUAL_INT32(64, ratimos_boot_flag_rise_px(RATIMOS_SPLASH_UNITS));
    TEST_ASSERT_EQUAL_INT32(32, ratimos_boot_flag_rise_px(RATIMOS_SPLASH_UNITS / 2));
    for (int u = 0; u <= RATIMOS_SPLASH_UNITS; u++) {
        TEST_ASSERT_TRUE(ratimos_boot_flag_rise_px(u) % 2 == 0);
        if (u > 0) {
            TEST_ASSERT_TRUE(ratimos_boot_flag_rise_px(u) >= ratimos_boot_flag_rise_px(u - 1));
        }
    }
}

void test_percent_format_rounds_units_and_carries_the_style_prefix(void)
{
    char buf[32];
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_BANDEIRA, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("000%", buf);
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_BANDEIRA, RATIMOS_SPLASH_UNITS, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("100%", buf);
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_BANDEIRA, RATIMOS_SPLASH_UNITS / 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("050%", buf);
    /* valor invalido nunca estoura */
    ratimos_boot_style_format_pct((ratimos_boot_style_t) 77, 500, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("100%", buf);
}

void test_classico_boot_corner_is_the_dither_dimmed_55_percent(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    refresh("classico_bandeira_000");

    int got[3], want[3];
    rgb565_to_rgb8(px565(5, 5), got);
    expected_classic_bg(5, 5, 55, want);
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(got, want, 17), "corner (5,5) is not bg_dither under 55% black");
}

void test_bandeira_flag_starts_low_and_ends_high(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, done_cb);
    refresh("bandeira_inicio");
    TEST_ASSERT_TRUE_MESSAGE(count_non_bg(161, 181, 190, 196) > 80, "no flag at y 181..196 with 0 units");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_bg(161, 117, 190, 132), "flag already at the top with 0 units");

    int guard = 0;
    while (ratimos_splash_units_shown() < RATIMOS_SPLASH_UNITS && guard++ < 600) {
        pump_ms(10);
    }
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_SPLASH_UNITS, ratimos_splash_units_shown());
    TEST_ASSERT_FALSE(s_done_info.called); /* ainda segurando o "pronto!" */
    refresh("bandeira_fim");
    TEST_ASSERT_TRUE_MESSAGE(count_non_bg(161, 117, 191, 132) > 80, "no flag at y 117..132 at the end");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_bg(161, 183, 190, 196), "flag left something behind");
}

void test_bandeira_mast_is_seamless_and_has_nothing_beside_it(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    int guard = 0;
    while (ratimos_splash_units_shown() < RATIMOS_SPLASH_UNITS && guard++ < 600) {
        pump_ms(10);
    }
    refresh(NULL);

    /* o mastro esticado (x 157..159, y 119..196) tem a cor da linha 40 do logo */
    for (int c = 0; c < 3; c++) {
        uint16_t want = desc_px565(&ratimos_logo_desc, 125 + c, 40);
        for (int32_t y = 119; y <= 196; y++) {
            TEST_ASSERT_EQUAL_HEX16_MESSAGE(want, px565(157 + c, y), "mast column is not the logo's row 40 color");
        }
    }
    /* nada ao lado do mastro (x 140..155, y 119..196 = so' o fundo) */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_bg(140, 119, 155, 196), "marks beside the mast");
}

void test_logo_uses_alpha_so_the_background_shows_through(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    refresh(NULL);

    /* um pixel do retangulo do logo (linhas 0..30, que ficam em y = 86 + linha)
     * onde o logo e' PRETO (alfa 0) mostra o fundo, nao uma caixa preta */
    int32_t fx = -1, fy = -1;
    const uint8_t * alpha = ratimos_logo_alpha_desc.data + (size_t) ratimos_logo_alpha_desc.header.w *
                                                               ratimos_logo_alpha_desc.header.h * 2;
    for (int32_t y = 0; y < 30 && fx < 0; y++) {
        for (int32_t x = 0; x < 256; x++) {
            if (alpha[y * 256 + x] == 0) {
                fx = x;
                fy = y;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(fx >= 0, "test fixture: no transparent logo pixel found");
    TEST_ASSERT_TRUE_MESSAGE(is_classic_bg(32 + fx, 86 + fy), "logo pixel with alpha 0 does not show the background");
}

void test_background_is_a_separate_swappable_step(void)
{
    ratimos_splash_set_background(solid_green_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    refresh("fundo_verde");

    int green[3] = { 0, 255, 0 }, got[3];
    int32_t pts[][2] = { { 5, 5 }, { 200, 150 }, { 300, 470 }, { 165, 135 } }; /* o trilho acima da bandeira inclusive */
    for (size_t i = 0; i < sizeof(pts) / sizeof(pts[0]); i++) {
        rgb565_to_rgb8(px565(pts[i][0], pts[i][1]), got);
        TEST_ASSERT_TRUE_MESSAGE(near_rgb(got, green, 8), "the test background was not drawn under the loading");
    }
}

void test_texts_use_the_boot_fonts_colors_and_a_1px_shadow(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    lv_obj_t * scr = lv_screen_active();
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(scr));
    lv_obj_update_layout(scr);

    lv_obj_t * pct_sh = lv_obj_get_child(scr, 1);
    lv_obj_t * pct = lv_obj_get_child(scr, 2);
    lv_obj_t * step_sh = lv_obj_get_child(scr, 3);
    lv_obj_t * step = lv_obj_get_child(scr, 4);

    TEST_ASSERT_EQUAL_PTR(&ratimos_font_title_8, lv_obj_get_style_text_font(pct, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(lv_color_hex(0xd8cce6)),
                              lv_color_to_u32(lv_obj_get_style_text_color(pct, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_STRING("000%", lv_label_get_text(pct));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_10, lv_obj_get_style_text_font(step, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_TEXT_MUTED),
                              lv_color_to_u32(lv_obj_get_style_text_color(step, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_STRING("montando cartão sd...", lv_label_get_text(step));

    lv_obj_t * pairs[2][2] = { { pct_sh, pct }, { step_sh, step } };
    for (int i = 0; i < 2; i++) {
        lv_obj_t * sh = pairs[i][0];
        lv_obj_t * tx = pairs[i][1];
        TEST_ASSERT_EQUAL_STRING(lv_label_get_text(tx), lv_label_get_text(sh));
        TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(lv_color_black()),
                                  lv_color_to_u32(lv_obj_get_style_text_color(sh, LV_PART_MAIN)));
        int opa = lv_obj_get_style_text_opa(sh, LV_PART_MAIN);
        TEST_ASSERT_TRUE_MESSAGE(opa >= 186 && opa <= 196, "shadow opacity is not ~75%");
        TEST_ASSERT_EQUAL_INT(lv_obj_get_x(tx) + 1, lv_obj_get_x(sh));
        TEST_ASSERT_EQUAL_INT(lv_obj_get_y(tx) + 1, lv_obj_get_y(sh));
    }
    TEST_ASSERT_EQUAL_INT(ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_BANDEIRA), lv_obj_get_y(pct));
    TEST_ASSERT_EQUAL_INT(ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_BANDEIRA) + 16, lv_obj_get_y(step));
}

void test_step_label_follows_the_real_steps_and_pronto_appears_exactly_once(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, done_cb);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_t * step = lv_obj_get_child(scr, 4);

    bool saw_second = false;
    int guard = 0;
    while (!s_done_info.called && guard++ < 600) {
        pump_ms(10);
        if (!s_done_info.called && strcmp(lv_label_get_text(step), "verificando cartas novas...") == 0) {
            saw_second = true;
        }
    }
    TEST_ASSERT_TRUE(saw_second);
    TEST_ASSERT_TRUE(s_done_info.called);
    TEST_ASSERT_EQUAL_STRING("pronto!", s_done_info.pct_text);
    TEST_ASSERT_TRUE_MESSAGE(s_done_info.step_hidden, "step line must disappear at the end");
    TEST_ASSERT_TRUE(s_done_info.step_shadow_hidden);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, s_done_info.visible_pronto, "'pronto!' must appear exactly once");
}

void test_screen_child_count_is_constant_during_the_boot(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, done_cb);
    lv_obj_t * scr = lv_screen_active();
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(scr));
    pump_ms(1000);
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(scr));
    int guard = 0;
    while (!s_done_info.called && guard++ < 600) {
        pump_ms(10);
    }
    TEST_ASSERT_EQUAL_UINT32(5, s_done_info.children);
}

void test_fade_in_takes_about_400ms_and_never_fades_the_background(void)
{
    ratimos_splash_set_fade_ms_for_test(400);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_t * pct = lv_obj_get_child(scr, 2);

    refresh(NULL);
    TEST_ASSERT_EQUAL_UINT8(0, lv_obj_get_style_text_opa(pct, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_bg(32, 197, 287, 330), "logo visible before the fade-in started");
    /* o fundo ja esta la desde o primeiro quadro */
    TEST_ASSERT_TRUE(is_classic_bg(5, 5));

    pump_ms(500);
    refresh(NULL);
    TEST_ASSERT_EQUAL_UINT8(255, lv_obj_get_style_text_opa(pct, LV_PART_MAIN));
    TEST_ASSERT_TRUE_MESSAGE(count_non_bg(32, 197, 287, 330) > 500, "logo not visible after the fade-in");
}

void test_preview_runs_no_real_step_and_leaves_no_timer_behind(void)
{
    int base = count_timers();
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_NOITE));
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, done_cb);
    int guard = 0;
    while (!s_done_info.called && guard++ < 600) {
        pump_ms(10);
    }
    TEST_ASSERT_TRUE(s_done_info.called);
    /* a previa nao grava: o que estava salvo continua */
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_NOITE, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
    pump_ms(50);
    TEST_ASSERT_EQUAL_INT_MESSAGE(base, count_timers(), "orphan lv_timer after a preview");
}

void test_unknown_style_and_theme_ids_fall_back_instead_of_aborting(void)
{
    ratimos_splash_preview((ratimos_theme_id_t) 99, (ratimos_boot_style_t) 99, NULL);
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_DEFAULT, ratimos_splash_active_theme());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_DEFAULT, ratimos_splash_active_style());
    refresh(NULL);
    TEST_ASSERT_EQUAL_PTR(ratimos_theme_get(RATIMOS_THEME_CLASSICO), ratimos_theme_get((ratimos_theme_id_t) 99));
}


/* ------------------------------------------------------------------ *
 * Task 2: carregamentos A (ameias), C (letreiro) e D (torres)
 * ------------------------------------------------------------------ */

void test_style_names_prefixes_and_text_rows(void)
{
    TEST_ASSERT_EQUAL_STRING("hastear a bandeira", ratimos_boot_style_name(RATIMOS_BOOT_STYLE_BANDEIRA));
    TEST_ASSERT_EQUAL_STRING("ameias do castelo", ratimos_boot_style_name(RATIMOS_BOOT_STYLE_AMEIAS));
    TEST_ASSERT_EQUAL_STRING("letreiro que acende", ratimos_boot_style_name(RATIMOS_BOOT_STYLE_LETREIRO));
    TEST_ASSERT_EQUAL_STRING("fileira de torres", ratimos_boot_style_name(RATIMOS_BOOT_STYLE_TORRES));

    char buf[32];
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_AMEIAS, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("construindo 000%", buf);
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_LETREIRO, 0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("carregando 000%", buf);
    ratimos_boot_style_format_pct(RATIMOS_BOOT_STYLE_TORRES, RATIMOS_SPLASH_UNITS, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("carregando 100%", buf);

    TEST_ASSERT_EQUAL_INT(339, ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_BANDEIRA));
    TEST_ASSERT_EQUAL_INT(339, ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_AMEIAS));
    TEST_ASSERT_EQUAL_INT(303, ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_LETREIRO));
    TEST_ASSERT_EQUAL_INT(353, ratimos_boot_style_text_y(RATIMOS_BOOT_STYLE_TORRES));
}

void test_ameias_middle_has_lit_gradient_brick_unlit_last_brick_and_dark_outline(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_AMEIAS, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS / 2);
    refresh("ameias_meio");

    /* 1o tijolo (x 66..73): aceso, degrade de cima (y0+7) para baixo (y0+18): mais azul
     * em cima, mais vermelho embaixo */
    int top[3], bottom[3];
    rgb8_of(68, 303 + 7, top);
    rgb8_of(68, 303 + 18, bottom);
    TEST_ASSERT_TRUE_MESSAGE(bottom[0] > top[0] + 40, "gradient: red must grow downwards");
    TEST_ASSERT_TRUE_MESSAGE(top[2] > bottom[2] + 30, "gradient: blue must be stronger at the top");

    /* ultimo tijolo (coluna 23, x 250..257): apagado (#1a0d29) */
    TEST_ASSERT_TRUE_MESSAGE(px_is(252, 303 + 8, 0x1a0d29, 10), "last brick should be unlit");
    /* contorno #0d0515 (010), nao o #3a2252 do 008 */
    TEST_ASSERT_TRUE_MESSAGE(px_is(65, 303 + 10, 0x0d0515, 8), "wall outline color");
    /* nada de bandeira ainda */
    TEST_ASSERT_EQUAL_INT(0, count_non_color(240, 285, 290, 300, 0x000000, 8));
}

void test_ameias_end_puts_the_flag_to_the_right_of_the_mast(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_AMEIAS, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS);
    refresh("ameias_fim");

    TEST_ASSERT_TRUE_MESSAGE(px_is(250, 290, RATIMOS_BOOT_MAST_HEX, 10), "mast (#d9d0e6) missing at (250,285)");
    TEST_ASSERT_TRUE(px_is(251, 300, RATIMOS_BOOT_MAST_HEX, 10));
    TEST_ASSERT_TRUE_MESSAGE(count_non_color(254, 285, 281, 300, 0x000000, 8) > 60, "no flag to the RIGHT of the mast");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_color(236, 285, 249, 300, 0x000000, 8), "something LEFT of the mast");
}

void test_letreiro_middle_lights_left_of_the_edge_and_leaves_the_rest_at_17_percent(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_LETREIRO, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS / 2);
    refresh("letreiro_meio");

    /* edge = 34 + floor(units*193/UNITS/4)*4; achamos um pixel opaco do letreiro
     * a esquerda (col 60..90) e outro a direita (col 190..220) da borda. */
    int32_t edge = 34 + ((ratimos_splash_units_shown() * 193) / (RATIMOS_SPLASH_UNITS * 4)) * 4;
    int32_t lx = -1, ly = -1, rx = -1, ry = -1;
    for (int32_t y = 100; y < 135; y++) {
        for (int32_t x = 60; x < 90 && lx < 0; x++) {
            if (desc_alpha(&ratimos_logo_alpha_desc, x, y) == 255 && x + 2 < edge) { lx = x; ly = y; }
        }
        for (int32_t x = 190; x < 220 && rx < 0; x++) {
            if (desc_alpha(&ratimos_logo_alpha_desc, x, y) == 255 && x > edge + 4) { rx = x; ry = y; }
        }
    }
    TEST_ASSERT_TRUE(lx >= 0 && rx >= 0);
    int want_l[3], want_r[3], got_l[3], got_r[3];
    desc_rgb8(&ratimos_logo_alpha_desc, lx, ly, want_l);
    desc_rgb8(&ratimos_logo_alpha_desc, rx, ry, want_r);
    rgb8_of(32 + lx, 110 + ly, got_l);
    rgb8_of(32 + rx, 110 + ry, got_r);
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(got_l, want_l, 10), "letter left of the edge must be fully lit");
    for (int k = 0; k < 3; k++) {
        want_r[k] = want_r[k] * 43 / 255; /* 17% sobre preto */
    }
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(got_r, want_r, 12), "letter right of the edge must be at 17%");
    TEST_ASSERT_TRUE(got_l[0] + got_l[1] + got_l[2] > 2 * (got_r[0] + got_r[1] + got_r[2]));

    /* a area da bandeira (cols 135..157 do logo, a direita da borda) tem a bandeira
     * APAGADA (17%), nao o fundo preto */
    TEST_ASSERT_TRUE_MESSAGE(count_non_color(32 + 135, 110 + 31, 32 + 157, 110 + 46, 0x000000, 3) > 30,
                             "the dim flag must be drawn at 17%, not erased");
}

void test_letreiro_end_is_fully_lit_still_and_frieze_free(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_LETREIRO, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS);
    refresh("letreiro_fim");

    /* todo pixel opaco do logo aparece com a cor cheia */
    int bad = 0;
    for (int32_t y = 0; y < 171; y += 3) {
        for (int32_t x = 0; x < 256; x += 3) {
            if (desc_alpha(&ratimos_logo_alpha_desc, x, y) == 255) {
                int want[3], got[3];
                desc_rgb8(&ratimos_logo_alpha_desc, x, y, want);
                rgb8_of(32 + x, 110 + y, got);
                if (!near_rgb(got, want, 10)) {
                    bad++;
                }
            }
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, bad, "logo not fully lit at the end");

    /* parado: uma onda de 260ms depois, o quadro (acima do texto) e' identico */
    static uint8_t before[sizeof(s_disp_buf)];
    memcpy(before, s_disp_buf, sizeof(before));
    pump_ms(290);
    refresh(NULL);
    size_t rows_bytes = (size_t) 300 * RATIMOS_SCREEN_W * 2u; /* y < 300: acima do texto */
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(before, s_disp_buf, rows_bytes, "the finished letreiro must not wave");
}

void test_torres_middle_tints_the_first_and_leaves_the_last_empty(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_TORRES, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS / 2);
    refresh("torres_meio");

    /* um pixel opaco perto da base da torre (linha 36 do sprite de 40) */
    int32_t bx = -1;
    for (int32_t x = 0; x < 28; x++) {
        if (desc_alpha(&ratimos_boot_rook_empty_desc, x, 36) == 255) { bx = x; break; }
    }
    TEST_ASSERT_TRUE(bx >= 0);
    /* torre 0 (x0 58): tingida */
    TEST_ASSERT_EQUAL_HEX16(desc_px565(&ratimos_boot_rook_full_0_desc, bx, 36), px565(58 + bx, 299 + 36));
    /* torre 5 (x0 58 + 5*35 = 233): vazia (#2e1a46) */
    TEST_ASSERT_EQUAL_HEX16(desc_px565(&ratimos_boot_rook_empty_desc, bx, 36), px565(233 + bx, 299 + 36));
    TEST_ASSERT_TRUE(px_is(233 + bx, 299 + 36, 0x2e1a46, 8));
}

void test_torres_jump_only_for_the_one_that_just_filled_and_none_at_the_end(void)
{
    ratimos_splash_set_background(solid_black_bg);
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_TORRES, NULL);

    int32_t r0 = -1; /* primeira linha opaca do sprite */
    for (int32_t y = 0; y < 40 && r0 < 0; y++) {
        for (int32_t x = 0; x < 28; x++) {
            if (desc_alpha(&ratimos_boot_rook_empty_desc, x, y)) { r0 = y; break; }
        }
    }
    TEST_ASSERT_TRUE(r0 >= 0);

    pump_until_units(RATIMOS_SPLASH_UNITS / 2); /* rows = 60: torre 2 acabou de encher */
    int rows = ratimos_splash_units_shown() * 120 / RATIMOS_SPLASH_UNITS;
    TEST_ASSERT_TRUE(rows >= 60 && rows - 60 < 4);
    refresh(NULL);
    /* torre 2 (x 128..155) 2px acima: ha pixel na linha 297 + r0; torre 1 (x 93..120) nao */
    TEST_ASSERT_TRUE_MESSAGE(count_non_color(128, 297 + r0, 155, 297 + r0, 0x000000, 8) > 0, "tower 2 did not jump");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_color(93, 297 + r0, 120, 297 + r0, 0x000000, 8),
                                  "tower 1 must not jump");

    pump_until_units(RATIMOS_SPLASH_UNITS);
    refresh("torres_fim");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, count_non_color(58, 297, 260, 297 + r0 - 1 + (r0 == 0 ? 2 : 0), 0x000000, 8),
                                  "a tower is above its rest position at the end");
    /* todas cheias no fim: a torre 5 deixou de ser #2e1a46 */
    TEST_ASSERT_FALSE(px_is(233 + 14, 299 + 36, 0x2e1a46, 8));
}

void test_every_style_finishes_the_preview_without_aborting(void)
{
    for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
        memset(&s_done_info, 0, sizeof(s_done_info));
        ratimos_splash_preview(RATIMOS_THEME_CLASSICO, (ratimos_boot_style_t) st, done_cb);
        int guard = 0;
        while (!s_done_info.called && guard++ < 700) {
            pump_ms(10);
        }
        TEST_ASSERT_TRUE_MESSAGE(s_done_info.called, "a style never finished its preview");
        TEST_ASSERT_EQUAL_STRING("pronto!", s_done_info.pct_text);
        TEST_ASSERT_EQUAL_UINT32(5, s_done_info.children);
        TEST_ASSERT_EQUAL_INT(1, s_done_info.visible_pronto);
    }
}

void test_transparent_sprite_pixels_show_the_swapped_background(void)
{
    ratimos_splash_set_background(solid_green_bg);
    int32_t tx, ty;

    /* A: canto da muralha */
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_AMEIAS, NULL);
    pump_until_units(RATIMOS_SPLASH_UNITS);
    refresh(NULL);
    TEST_ASSERT_TRUE(find_alpha0(&ratimos_boot_wall_lit_desc, &tx, &ty));
    TEST_ASSERT_TRUE_MESSAGE(px_is(65 + tx, 302 + ty, 0x00ff00, 8), "A: transparent wall pixel is not the test background");

    /* B: o logo (linhas 0..30 em y = 86 + linha) e a bandeira (sprite proprio, linha a
     * linha, fase 0 no 1o quadro) mostram o fundo nos pixels transparentes */
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    refresh(NULL);
    TEST_ASSERT_TRUE(find_alpha0(&ratimos_logo_alpha_desc, &tx, &ty));
    TEST_ASSERT_TRUE(ty < 31);
    TEST_ASSERT_TRUE_MESSAGE(px_is(32 + tx, 86 + ty, 0x00ff00, 8), "B: transparent logo pixel");
    TEST_ASSERT_TRUE(find_alpha0(&ratimos_boot_flag_desc, &tx, &ty));
    TEST_ASSERT_TRUE_MESSAGE(px_is(161 + ((ty / 3) % 2) + tx, 181 + ty, 0x00ff00, 8), "B: transparent flag pixel");

    /* C: pixel transparente do friso */
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_LETREIRO, NULL);
    pump_until_units(8); /* edge ainda perto do comeco, friso visivel */
    refresh(NULL);
    int32_t edge = 34 + ((ratimos_splash_units_shown() * 193) / (RATIMOS_SPLASH_UNITS * 4)) * 4;
    TEST_ASSERT_TRUE(edge < 226);
    int32_t gx = edge - RATIMOS_BOOT_GLOW_X0;
    int found = 0;
    for (uint32_t y = 0; y < ratimos_boot_wordmark_glow_desc.header.h; y++) {
        if (desc_alpha(&ratimos_boot_wordmark_glow_desc, gx, (int32_t) y) == 0) {
            /* o brilho so' tem alfa onde o logo tem: logo transparente tambem aqui */
            TEST_ASSERT_TRUE_MESSAGE(px_is(32 + edge, 110 + RATIMOS_BOOT_GLOW_Y0 + (int32_t) y, 0x00ff00, 8),
                                     "C: transparent frieze pixel");
            found = 1;
            break;
        }
    }
    TEST_ASSERT_TRUE(found);

    /* D: canto da torre */
    ratimos_splash_preview(RATIMOS_THEME_CLASSICO, RATIMOS_BOOT_STYLE_TORRES, NULL);
    refresh(NULL);
    TEST_ASSERT_TRUE(find_alpha0(&ratimos_boot_rook_empty_desc, &tx, &ty));
    TEST_ASSERT_TRUE_MESSAGE(px_is(58 + tx, 299 + ty, 0x00ff00, 8), "D: transparent tower pixel");
}

void test_each_style_has_a_40x60_thumbnail_with_alpha(void)
{
    for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
        const lv_image_dsc_t * t = ratimos_boot_style_thumb((ratimos_boot_style_t) st);
        TEST_ASSERT_NOT_NULL(t);
        TEST_ASSERT_EQUAL_UINT32(40, t->header.w);
        TEST_ASSERT_EQUAL_UINT32(60, t->header.h);
        TEST_ASSERT_EQUAL_UINT32(LV_COLOR_FORMAT_RGB565A8, t->header.cf);
        int opaque = 0, transparent = 0;
        for (int32_t y = 0; y < 60; y++) {
            for (int32_t x = 0; x < 40; x++) {
                uint8_t a = desc_alpha(t, x, y);
                if (a > 200) opaque++;
                if (a == 0) transparent++;
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(opaque > 20, "thumbnail has nothing drawn");
        TEST_ASSERT_TRUE_MESSAGE(transparent > 500, "thumbnail must keep alpha so the theme background shows");
    }
}

/*
 * Invariante que torna a invalidacao parcial SEGURA (T-02.1-41/42): entre dois
 * quadros consecutivos (1 tick), todo pixel que muda acima das linhas de texto
 * esta dentro da area de dirty_area OU de wave_area do estilo -- se o LVGL
 * redesenhasse so' essas areas (modo parcial do dispositivo), o resultado seria
 * o mesmo que redesenhar tudo.
 */
void test_dirty_and_wave_areas_cover_every_pixel_that_changes_between_ticks(void)
{
    static uint8_t prev_buf[sizeof(s_disp_buf)];
    ratimos_splash_set_background(solid_black_bg);

    for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
        ratimos_splash_preview(RATIMOS_THEME_CLASSICO, (ratimos_boot_style_t) st, NULL);
        int32_t limit_y = ratimos_boot_style_text_y((ratimos_boot_style_t) st) - 1;
        refresh(NULL);
        memcpy(prev_buf, s_disp_buf, sizeof(prev_buf));
        int prev_units = 0;
        bool prev_done = false;
        int checked = 0;

        for (int guard = 0; guard < 700 && !prev_done; guard++) {
            pump_ms(10);
            refresh(NULL);
            int units = ratimos_splash_units_shown();
            lv_area_t dirty, wave;
            bool has_dirty = false, has_wave = false;
            if (units != prev_units) {
                has_dirty = ratimos_boot_style_dirty_area((ratimos_boot_style_t) st, prev_units, units, &dirty);
            }
            has_wave = ratimos_boot_style_wave_area((ratimos_boot_style_t) st, units >= RATIMOS_SPLASH_UNITS, &wave);
            for (int32_t y = 0; y < limit_y; y++) {
                for (int32_t x = 0; x < RATIMOS_SCREEN_W; x++) {
                    size_t idx = ((size_t) y * RATIMOS_SCREEN_W + (size_t) x) * 2u;
                    if (s_disp_buf[idx] != prev_buf[idx] || s_disp_buf[idx + 1] != prev_buf[idx + 1]) {
                        bool in_dirty = has_dirty && x >= dirty.x1 && x <= dirty.x2 && y >= dirty.y1 && y <= dirty.y2;
                        bool in_wave = has_wave && x >= wave.x1 && x <= wave.x2 && y >= wave.y1 && y <= wave.y2;
                        char msg[96];
                        snprintf(msg, sizeof(msg), "style %d: pixel (%d,%d) changed outside dirty/wave areas (units %d->%d)",
                                 st, (int) x, (int) y, prev_units, units);
                        TEST_ASSERT_TRUE_MESSAGE(in_dirty || in_wave, msg);
                    }
                }
            }
            memcpy(prev_buf, s_disp_buf, sizeof(prev_buf));
            prev_units = units;
            prev_done = units >= RATIMOS_SPLASH_UNITS;
            checked++;
        }
        TEST_ASSERT_TRUE(checked > 50);
        leave_splash();
    }
}

/*
 * 3 voltas de previas dos 4 carregamentos: o heap do LVGL da volta 3 nao passa
 * da volta 2 + 2%, e nenhum lv_timer sobra (T-02.1-41).
 */
static size_t run_styles_lap(void)
{
    for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
        memset(&s_done_info, 0, sizeof(s_done_info));
        ratimos_splash_preview(RATIMOS_THEME_CLASSICO, (ratimos_boot_style_t) st, done_cb);
        int guard = 0;
        while (!s_done_info.called && guard++ < 700) {
            pump_ms(10);
        }
        TEST_ASSERT_TRUE(s_done_info.called);
    }
    pump_ms(50);
    return ratimos_heap_used();
}

void test_three_laps_of_style_previews_keep_the_heap_and_timers_stable(void)
{
    ratimos_splash_set_fade_ms_for_test(400); /* o caminho real, com a animacao de fade */
    int base_timers = count_timers();
    run_styles_lap();
    size_t lap2 = run_styles_lap();
    size_t lap3 = run_styles_lap();
    TEST_ASSERT_TRUE_MESSAGE(lap3 <= lap2 + lap2 / 50, "LVGL heap grew from lap 2 to lap 3");
    TEST_ASSERT_EQUAL_INT_MESSAGE(base_timers, count_timers(), "orphan lv_timer after the laps");
}


/* ------------------------------------------------------------------ *
 * Task 3: Noite estrelada, Castelo ao entardecer e Xadrez
 * ------------------------------------------------------------------ */

static void snap(uint8_t * dst)
{
    memcpy(dst, s_disp_buf, sizeof(s_disp_buf));
}

static bool snap_px_differs(const uint8_t * a, const uint8_t * b, int32_t x, int32_t y)
{
    size_t idx = ((size_t) y * RATIMOS_SCREEN_W + (size_t) x) * 2u;
    return a[idx] != b[idx] || a[idx + 1] != b[idx + 1];
}

static bool near_a_star(int32_t x, int32_t y)
{
    for (int i = 0; i < RATIMOS_NOITE_STAR_COUNT; i++) {
        if (abs_i(ratimos_noite_stars[i].x - (int) x) <= 1 && abs_i(ratimos_noite_stars[i].y - (int) y) <= 1) {
            return true;
        }
    }
    return false;
}

void test_the_catalog_has_the_4_themes_with_the_sketch_dims_and_images(void)
{
    const ratimos_theme_t * c = ratimos_theme_get(RATIMOS_THEME_CLASSICO);
    const ratimos_theme_t * n = ratimos_theme_get(RATIMOS_THEME_NOITE);
    const ratimos_theme_t * k = ratimos_theme_get(RATIMOS_THEME_CASTELO);
    const ratimos_theme_t * x = ratimos_theme_get(RATIMOS_THEME_XADREZ);

    TEST_ASSERT_EQUAL_INT(55, c->boot_bg.dim_pct);
    TEST_ASSERT_EQUAL_PTR(&ratimos_bg_dither_desc, c->system_bg.image);
    TEST_ASSERT_EQUAL_INT(0, c->system_bg.dim_pct);

    TEST_ASSERT_EQUAL_INT(0, n->boot_bg.dim_pct);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_noite_desc, n->system_bg.image);

    TEST_ASSERT_EQUAL_INT(15, k->boot_bg.dim_pct);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_castelo_desc, k->system_bg.image);
    TEST_ASSERT_EQUAL_INT(20, k->system_bg.dim_pct);

    TEST_ASSERT_EQUAL_INT(10, x->boot_bg.dim_pct);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_xadrez_desc, x->system_bg.image);
    TEST_ASSERT_EQUAL_INT(0, x->system_bg.dim_pct);

    const ratimos_theme_t * all[4] = { c, n, k, x };
    for (int i = 0; i < 4; i++) {
        /* fundos 320x480 sao RGB565 CRU: indexado passaria pelo decoder e alocaria heap */
        TEST_ASSERT_EQUAL_UINT32(LV_COLOR_FORMAT_RGB565, all[i]->system_bg.image->header.cf);
        TEST_ASSERT_EQUAL_UINT32(320, all[i]->system_bg.image->header.w);
        TEST_ASSERT_EQUAL_UINT32(480, all[i]->system_bg.image->header.h);
        TEST_ASSERT_NOT_NULL(all[i]->home_thumb);
        TEST_ASSERT_NOT_NULL(all[i]->boot_bg_thumb);
        TEST_ASSERT_EQUAL_UINT32(40, all[i]->home_thumb->header.w);
        TEST_ASSERT_EQUAL_UINT32(60, all[i]->boot_bg_thumb->header.h);
    }
    TEST_ASSERT_EQUAL_STRING("noite estrelada", n->name);
    TEST_ASSERT_EQUAL_STRING("castelo ao entardecer", k->name);
}

void test_noite_boot_has_the_sky_band_and_the_moon(void)
{
    ratimos_splash_preview(RATIMOS_THEME_NOITE, RATIMOS_BOOT_STYLE_TORRES, NULL);
    refresh("noite_boot");
    TEST_ASSERT_TRUE_MESSAGE(px_is(5, 5, 0x07030f, 8), "Noite: sky band 0 (#07030f) at (5,5)");
    TEST_ASSERT_TRUE_MESSAGE(px_is(5, 200, 0x0b0518, 8), "Noite: sky band 1");
    TEST_ASSERT_TRUE_MESSAGE(px_is(5, 470, 0x110822, 8), "Noite: sky band 2");
    TEST_ASSERT_TRUE_MESSAGE(px_is(262, 56, 0xd9cfe6, 8), "Noite: moon at (262,56)");
}

void test_noite_boot_stars_twinkle_and_nothing_else_moves(void)
{
    static uint8_t a[sizeof(s_disp_buf)], b[sizeof(s_disp_buf)];
    ratimos_splash_preview(RATIMOS_THEME_NOITE, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
    refresh(NULL);
    pump_ms(50);
    refresh(NULL);
    snap(a);
    pump_ms(1700);
    refresh("noite_boot_depois");
    snap(b);

    int changed = 0;
    /* acima do logo (y < 80) e abaixo do texto (y >= 380): so' as estrelas podem mudar */
    for (int32_t y = 0; y < 480; y++) {
        if (y >= 80 && y < 380) {
            continue;
        }
        for (int32_t x = 0; x < RATIMOS_SCREEN_W; x++) {
            if (snap_px_differs(a, b, x, y)) {
                changed++;
                TEST_ASSERT_TRUE_MESSAGE(near_a_star(x, y), "a non-star pixel changed in the Noite boot background");
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(changed > 0, "no star twinkled in ~1.7s");
}

void test_noite_home_background_is_the_generated_static_image(void)
{
    const ratimos_theme_t * n = ratimos_theme_get(RATIMOS_THEME_NOITE);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_noite_desc, n->system_bg.image);
    /* a lua esta no fundo da home (parada) */
    int rgb[3];
    desc_rgb8(&ratimos_home_bg_noite_desc, 262, 56, rgb);
    int moon[3] = { 0xd9, 0xcf, 0xe6 };
    TEST_ASSERT_TRUE(near_rgb(rgb, moon, 8));
}

void test_castelo_boot_sky_roof_and_blinking_windows(void)
{
    ratimos_splash_preview(RATIMOS_THEME_CASTELO, RATIMOS_BOOT_STYLE_TORRES, NULL);
    refresh("castelo_boot");

    /* faixa de ceu y=10: #0c0420 (a imagem e' RGB565) com 15% de escurecimento */
    int got[3], want[3], band[3] = { 0x0c, 0x04, 0x20 };
    desc_rgb8(&ratimos_boot_bg_castelo_desc, 5, 10, want);
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(want, band, 8), "Castelo boot image: band 0 is #0c0420 at y=10");
    for (int k = 0; k < 3; k++) {
        want[k] = want[k] * 85 / 100;
    }
    rgb8_of(5, 10, got);
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(got, want, 8), "Castelo boot: sky band at y=10 under 15% dim");

    /* telhado vermelho a direita (x > 150): ha pixels do vermelho do telhado na imagem do boot */
    int roof_px = 0;
    for (int32_t y = 330; y < 460; y++) {
        for (int32_t x = 151; x < 320; x++) {
            int c[3];
            desc_rgb8(&ratimos_boot_bg_castelo_desc, x, y, c);
            int roof[3] = { 0x8a, 0x0a, 0x18 };
            if (near_rgb(c, roof, 8)) {
                roof_px++;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(roof_px > 100, "Castelo boot image: no red roof on the right (x > 150)");

    /* as janelas alternam: em cada amostra o pixel bate com o estado esperado pelo relogio */
    int saw_on[RATIMOS_CASTELO_WINDOW_COUNT] = { 0 }, saw_off[RATIMOS_CASTELO_WINDOW_COUNT] = { 0 };
    for (int step = 0; step < 24; step++) {
        pump_ms(100);
        refresh(NULL);
        for (int i = 0; i < RATIMOS_CASTELO_WINDOW_COUNT; i++) {
            const ratimos_window_t * w = &ratimos_castelo_windows[i];
            int32_t cx = w->x + 1, cy = w->y + 3; /* dentro da janela, abaixo do friso */
            /* o estado vem do instante do ULTIMO tick do fundo (50ms), nao do relogio exato */
            uint32_t now = lv_tick_get();
            bool off_now = ((now / 800u) + w->i) % 5u == 0u;
            bool off_prev = (((now - 50u) / 800u) + w->i) % 5u == 0u; /* tolerancia de 1 tick */
            int c_on[3] = { 0xff * 85 / 100, 0xc0 * 85 / 100, 0x4a * 85 / 100 };
            int c_off[3] = { 0x7a * 85 / 100, 0x4a * 85 / 100, 0x10 * 85 / 100 };
            int g[3];
            rgb8_of(cx, cy, g);
            bool is_on = near_rgb(g, c_on, 14), is_off = near_rgb(g, c_off, 14);
            TEST_ASSERT_TRUE_MESSAGE(is_on || is_off, "window pixel is neither on nor off color");
            TEST_ASSERT_TRUE_MESSAGE(is_off == off_now || is_off == off_prev, "window state disagrees with the 800ms clock");
            if (is_on) saw_on[i] = 1;
            if (is_off) saw_off[i] = 1;
        }
    }
    int toggled = 0;
    for (int i = 0; i < RATIMOS_CASTELO_WINDOW_COUNT; i++) {
        if (saw_on[i] && saw_off[i]) toggled++;
    }
    /* 24 amostras de 100ms = 2.4s: 3 valores de floor(now/800); pelo menos 2 das janelas
     * (indices 0,1,2,3 mod 5) apagam e acendem nesse intervalo */
    TEST_ASSERT_TRUE_MESSAGE(toggled >= 1, "no castle window toggled between lit and unlit");
}

void test_castelo_home_background_is_the_static_image_with_horizon_450(void)
{
    const ratimos_theme_t * k = ratimos_theme_get(RATIMOS_THEME_CASTELO);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_castelo_desc, k->system_bg.image);
    TEST_ASSERT_EQUAL_INT(20, k->system_bg.dim_pct);
    /* horizonte em 450 (boot: 466): a faixa de ceu mais baixa (#ee6a12) comeca em y 405 na home
     * (10 faixas de 45) e em y 423 no boot (10 faixas de 47) -- y=410 e' laranja so' na home */
    int home[3], boot[3], orange[3] = { 0xee, 0x6a, 0x12 };
    desc_rgb8(&ratimos_home_bg_castelo_desc, 5, 410, home);
    desc_rgb8(&ratimos_boot_bg_castelo_desc, 5, 410, boot);
    TEST_ASSERT_TRUE_MESSAGE(near_rgb(home, orange, 12), "home: last sky band (#ee6a12) starts at y=405");
    TEST_ASSERT_FALSE_MESSAGE(near_rgb(boot, orange, 12), "boot horizon is lower than the home one");
}

void test_xadrez_boot_line_board_scrolls_and_sky_stays(void)
{
    static uint8_t a[sizeof(s_disp_buf)], b[sizeof(s_disp_buf)];
    ratimos_splash_preview(RATIMOS_THEME_XADREZ, RATIMOS_BOOT_STYLE_LETREIRO, NULL);
    refresh("xadrez_boot");
    /* a linha do horizonte (y=140) #4e2277 com 10% de escurecimento */
    uint32_t line_dimmed = (uint32_t) (((0x4e * 90 / 100) << 16) | ((0x22 * 90 / 100) << 8) | (0x77 * 90 / 100));
    TEST_ASSERT_TRUE_MESSAGE(px_is(5, 140, line_dimmed, 8), "Xadrez boot: horizon line (#4e2277 under 10% dim)");
    snap(a);
    pump_ms(1000);
    refresh(NULL);
    pump_ms(1000);
    refresh("xadrez_boot_2s");
    snap(b);

    int sky = 0, board_near_base = 0;
    for (int32_t y = 0; y < 80; y++) {
        for (int32_t x = 0; x < RATIMOS_SCREEN_W; x++) {
            sky += snap_px_differs(a, b, x, y);
        }
    }
    for (int32_t y = 400; y < 480; y++) {
        for (int32_t x = 0; x < RATIMOS_SCREEN_W; x++) {
            board_near_base += snap_px_differs(a, b, x, y);
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, sky, "the sky (y < 80) must not change");
    TEST_ASSERT_TRUE_MESSAGE(board_near_base > 100, "the board did not scroll in ~2s");
}

void test_xadrez_home_has_horizon_70_and_brightness_085(void)
{
    const ratimos_theme_t * x = ratimos_theme_get(RATIMOS_THEME_XADREZ);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_xadrez_desc, x->system_bg.image);
    TEST_ASSERT_EQUAL_INT(0, x->system_bg.dim_pct);
    /* sem linha de horizonte na home (bgChessHome so' copia a imagem): a linha 70 e' so' tabuleiro/nevoa */
    int line[3] = { 0x4e, 0x22, 0x77 }, got[3];
    desc_rgb8(&ratimos_home_bg_xadrez_desc, 5, 70, got);
    TEST_ASSERT_FALSE(near_rgb(got, line, 20));
    /* brilho 0.85: nenhum canal passa de 70 * 0.85 = 60 (a casa mais clara) */
    int maxc = 0;
    for (int32_t y = 0; y < 480; y += 7) {
        for (int32_t xx = 0; xx < 320; xx += 5) {
            int c[3];
            desc_rgb8(&ratimos_home_bg_xadrez_desc, xx, y, c);
            for (int k = 0; k < 3; k++) if (c[k] > maxc) maxc = c[k];
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(maxc <= 70 * 85 / 100 + 8, "xadrez home must be at 0.85 brightness");
}

void test_all_16_theme_style_combinations_finish_the_preview(void)
{
    for (int t = 0; t < RATIMOS_THEME_COUNT; t++) {
        for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
            memset(&s_done_info, 0, sizeof(s_done_info));
            ratimos_splash_preview((ratimos_theme_id_t) t, (ratimos_boot_style_t) st, done_cb);
            int guard = 0;
            while (!s_done_info.called && guard++ < 700) {
                pump_ms(10);
            }
            TEST_ASSERT_TRUE_MESSAGE(s_done_info.called, "a theme x style combination never finished");
            TEST_ASSERT_EQUAL_STRING("pronto!", s_done_info.pct_text);
            TEST_ASSERT_EQUAL_UINT32(5, s_done_info.children);
        }
    }
}

static size_t run_themes_lap(void)
{
    for (int t = 0; t < RATIMOS_THEME_COUNT; t++) {
        memset(&s_done_info, 0, sizeof(s_done_info));
        ratimos_splash_preview((ratimos_theme_id_t) t, RATIMOS_BOOT_STYLE_LETREIRO, done_cb);
        int guard = 0;
        while (!s_done_info.called && guard++ < 700) {
            pump_ms(10);
        }
        TEST_ASSERT_TRUE(s_done_info.called);
    }
    pump_ms(50);
    return ratimos_heap_used();
}

void test_three_laps_of_theme_previews_keep_the_heap_and_timers_stable(void)
{
    ratimos_splash_set_fade_ms_for_test(400);
    int base_timers = count_timers();
    run_themes_lap();
    size_t lap2 = run_themes_lap();
    size_t lap3 = run_themes_lap();
    TEST_ASSERT_TRUE_MESSAGE(lap3 <= lap2 + lap2 / 50, "LVGL heap grew from lap 2 to lap 3 across the 4 themes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(base_timers, count_timers(), "orphan lv_timer after the theme laps");
}

void test_animated_backgrounds_create_no_lvgl_object_per_frame(void)
{
    for (int t = 0; t < RATIMOS_THEME_COUNT; t++) {
        ratimos_splash_preview((ratimos_theme_id_t) t, RATIMOS_BOOT_STYLE_BANDEIRA, NULL);
        lv_obj_t * scr = lv_screen_active();
        uint32_t children = lv_obj_get_child_count(scr);
        size_t heap0 = ratimos_heap_used();
        pump_ms(600);
        refresh(NULL);
        pump_ms(600);
        refresh(NULL);
        TEST_ASSERT_EQUAL_UINT32(children, lv_obj_get_child_count(scr));
        /* o heap oscila so' pelos textos (3 labels) e pelas draw tasks transitorias; sem objetos */
        TEST_ASSERT_TRUE_MESSAGE(ratimos_heap_used() <= heap0 + 4096, "animated background grew the LVGL heap");
    }
}


/*
 * O que torna a invalidacao parcial SEGURA no dispositivo (T-02.1-41/42): um
 * display em modo PARTIAL (como o da placa) renderizando so' as areas que
 * splash/boot_styles/theme_catalog invalidam tem que produzir EXATAMENTE o
 * mesmo quadro que um redesenho completo -- nenhum pixel mudou sem ser
 * invalidado. Vale para as 16 combinacoes tema x carregamento.
 */
static uint8_t s_part_buf[RATIMOS_SCREEN_W * 48 * 2];
static uint8_t s_part_fb[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2];

static void part_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    int32_t w = lv_area_get_width(area), h = lv_area_get_height(area);
    uint32_t stride = lv_draw_buf_width_to_stride((uint32_t) w, LV_COLOR_FORMAT_RGB565);
    for (int32_t y = 0; y < h; y++) {
        memcpy(&s_part_fb[(((size_t) area->y1 + (size_t) y) * RATIMOS_SCREEN_W + (size_t) area->x1) * 2u],
               px_map + (size_t) y * stride, (size_t) w * 2u);
    }
    lv_display_flush_ready(disp);
}

void test_partial_rendering_equals_a_full_redraw_for_the_16_combinations(void)
{
    static uint8_t reference[sizeof(s_part_fb)];
    lv_display_t * prev_default = lv_display_get_default();
    lv_display_t * part = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(part, s_part_buf, NULL, sizeof(s_part_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(part, part_flush_cb);
    lv_display_set_default(part);

    int compares = 0;
    for (int t = 0; t < RATIMOS_THEME_COUNT; t++) {
        for (int st = 0; st < RATIMOS_BOOT_STYLE_COUNT; st++) {
            ratimos_splash_preview((ratimos_theme_id_t) t, (ratimos_boot_style_t) st, NULL);
            lv_refr_now(part);
            for (int n = 1; n <= 260 && ratimos_splash_units_shown() < RATIMOS_SPLASH_UNITS; n++) {
                pump_ms(10);
                lv_refr_now(part);
                if (n % 9 != 0) {
                    continue;
                }
                memcpy(reference, s_part_fb, sizeof(reference)); /* o que o modo parcial acumulou */
                lv_obj_invalidate(lv_screen_active());
                lv_refr_now(part); /* redesenho completo no MESMO instante */
                compares++;
                if (memcmp(reference, s_part_fb, sizeof(reference)) != 0) {
                    size_t i = 0;
                    while (reference[i] == s_part_fb[i]) {
                        i++;
                    }
                    char msg[120];
                    snprintf(msg, sizeof(msg), "theme %d style %d: partial render != full redraw at pixel (%d,%d), units %d",
                             t, st, (int) ((i / 2) % RATIMOS_SCREEN_W), (int) ((i / 2) / RATIMOS_SCREEN_W),
                             (int) ratimos_splash_units_shown());
                    lv_display_set_default(prev_default);
                    TEST_FAIL_MESSAGE(msg);
                }
            }
            leave_splash();
        }
    }
    lv_display_set_default(prev_default);
    lv_display_delete(part);
    TEST_ASSERT_TRUE(compares > 100);
}

int main(void)
{
    char tmpl[] = "/tmp/ratimos-bootscreen-XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    lv_init();
    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    UNITY_BEGIN();
    RUN_TEST(test_without_settings_the_boot_is_classico_and_bandeira);
    RUN_TEST(test_boot_reads_the_two_settings_independently_before_the_first_step);
    RUN_TEST(test_progress_model_invariants_hold_every_tick_and_home_waits_for_pronto);
    RUN_TEST(test_flag_rise_is_a_pure_even_function_of_progress);
    RUN_TEST(test_percent_format_rounds_units_and_carries_the_style_prefix);
    RUN_TEST(test_classico_boot_corner_is_the_dither_dimmed_55_percent);
    RUN_TEST(test_bandeira_flag_starts_low_and_ends_high);
    RUN_TEST(test_bandeira_mast_is_seamless_and_has_nothing_beside_it);
    RUN_TEST(test_logo_uses_alpha_so_the_background_shows_through);
    RUN_TEST(test_background_is_a_separate_swappable_step);
    RUN_TEST(test_texts_use_the_boot_fonts_colors_and_a_1px_shadow);
    RUN_TEST(test_step_label_follows_the_real_steps_and_pronto_appears_exactly_once);
    RUN_TEST(test_screen_child_count_is_constant_during_the_boot);
    RUN_TEST(test_fade_in_takes_about_400ms_and_never_fades_the_background);
    RUN_TEST(test_preview_runs_no_real_step_and_leaves_no_timer_behind);
    RUN_TEST(test_unknown_style_and_theme_ids_fall_back_instead_of_aborting);
    RUN_TEST(test_style_names_prefixes_and_text_rows);
    RUN_TEST(test_ameias_middle_has_lit_gradient_brick_unlit_last_brick_and_dark_outline);
    RUN_TEST(test_ameias_end_puts_the_flag_to_the_right_of_the_mast);
    RUN_TEST(test_letreiro_middle_lights_left_of_the_edge_and_leaves_the_rest_at_17_percent);
    RUN_TEST(test_letreiro_end_is_fully_lit_still_and_frieze_free);
    RUN_TEST(test_torres_middle_tints_the_first_and_leaves_the_last_empty);
    RUN_TEST(test_torres_jump_only_for_the_one_that_just_filled_and_none_at_the_end);
    RUN_TEST(test_every_style_finishes_the_preview_without_aborting);
    RUN_TEST(test_transparent_sprite_pixels_show_the_swapped_background);
    RUN_TEST(test_each_style_has_a_40x60_thumbnail_with_alpha);
    RUN_TEST(test_dirty_and_wave_areas_cover_every_pixel_that_changes_between_ticks);
    RUN_TEST(test_three_laps_of_style_previews_keep_the_heap_and_timers_stable);
    RUN_TEST(test_the_catalog_has_the_4_themes_with_the_sketch_dims_and_images);
    RUN_TEST(test_noite_boot_has_the_sky_band_and_the_moon);
    RUN_TEST(test_noite_boot_stars_twinkle_and_nothing_else_moves);
    RUN_TEST(test_noite_home_background_is_the_generated_static_image);
    RUN_TEST(test_castelo_boot_sky_roof_and_blinking_windows);
    RUN_TEST(test_castelo_home_background_is_the_static_image_with_horizon_450);
    RUN_TEST(test_xadrez_boot_line_board_scrolls_and_sky_stays);
    RUN_TEST(test_xadrez_home_has_horizon_70_and_brightness_085);
    RUN_TEST(test_all_16_theme_style_combinations_finish_the_preview);
    RUN_TEST(test_three_laps_of_theme_previews_keep_the_heap_and_timers_stable);
    RUN_TEST(test_animated_backgrounds_create_no_lvgl_object_per_frame);
    RUN_TEST(test_partial_rendering_equals_a_full_redraw_for_the_16_combinations);
    return UNITY_END();
}
