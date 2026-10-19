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
#include "ratimos/boot_styles.h"
#include "ratimos/fonts/ratimos_fonts.h"
#include "ratimos/logo_image.h"
#include "ratimos/splash.h"
#include "ratimos/theme.h"
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
    return UNITY_END();
}
