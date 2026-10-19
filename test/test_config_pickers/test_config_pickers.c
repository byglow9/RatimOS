/*
 * Suite da tela de Configuracoes e dos seletores de tema e carregamento
 * (plano 02.1-16, Task 4): ./home/config, ./home/config/tema,
 * ./home/config/carregamento -- estrutura, hitbox, selecao, previa, aplicar
 * (persistencia independente), toast, interludio com a entrada de toque
 * desligada e religada, e os caminhos de falha.
 *
 * LVGL headless (display 320x480 FULL, mesmo harness do test_game_screens) com
 * um indev de ponteiro de mentira, para observar lv_indev_enable(). main() faz
 * mkdtemp + chdir ANTES de qualquer chamada: o settings.bin (substituto do NVS)
 * e' gravado no diretorio temporario, nunca no assets/save/ do desenvolvedor.
 */
#include <unity.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lvgl.h"
#include "src/indev/lv_indev_private.h"
#include "ratimos/app_shell.h"
#include "ratimos/apps/config_app.h"
#include "ratimos/apps/config_picker.h"
#include "ratimos/boot_styles.h"
#include "ratimos/fonts/ratimos_fonts.h"
#include "ratimos/heap_probe.h"
#include "ratimos/home_screen.h"
#include "ratimos/icons.h"
#include "ratimos/splash.h"
#include "ratimos/theme.h"
#include "ratimos/theme_art.h"
#include "ratimos/theme_catalog.h"
#include "storage/content_api.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */
static lv_indev_t * s_indev;

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

static void dummy_indev_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void) indev;
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = 0;
    data->point.y = 0;
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

static int count_timers(void)
{
    int n = 0;
    for (lv_timer_t * t = lv_timer_get_next(NULL); t != NULL; t = lv_timer_get_next(t)) {
        n++;
    }
    return n;
}

static bool input_enabled(void)
{
    return s_indev->enabled != 0;
}

static const char * active_path(void)
{
    lv_obj_t * sec = lv_obj_get_child(lv_screen_active(), 2);
    return lv_label_get_text(lv_obj_get_child(sec, 0));
}

static void press_back(lv_obj_t * scr)
{
    lv_obj_t * bottombar = lv_obj_get_child(scr, 4);
    lv_obj_t * back = lv_obj_get_child(bottombar, 0);
    TEST_ASSERT_TRUE(lv_obj_has_flag(back, LV_OBJ_FLAG_CLICKABLE));
    lv_obj_send_event(back, LV_EVENT_CLICKED, NULL);
}

static lv_obj_t * content_of(lv_obj_t * scr)
{
    return lv_obj_get_child(scr, 3);
}

/* Procura (em profundidade) um label visivel com exatamente `text`. */
static lv_obj_t * find_label(lv_obj_t * root, const char * text)
{
    uint32_t n = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t * c = lv_obj_get_child(root, (int32_t) i);
        if (lv_obj_get_class(c) == &lv_label_class && strcmp(lv_label_get_text(c), text) == 0) {
            return c;
        }
        lv_obj_t * r = find_label(c, text);
        if (r) {
            return r;
        }
    }
    return NULL;
}

static bool label_visible(lv_obj_t * label)
{
    for (lv_obj_t * o = label; o != NULL; o = lv_obj_get_parent(o)) {
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
            return false;
        }
    }
    return true;
}

static uint16_t px565(int32_t x, int32_t y)
{
    size_t idx = ((size_t) y * RATIMOS_SCREEN_W + (size_t) x) * 2u;
    return (uint16_t) (s_disp_buf[idx] | (uint16_t) (s_disp_buf[idx + 1] << 8));
}

/* RATIMOS_DUMP_DIR=<dir>: grava o quadro atual como PPM (conferencia visual). */
static void maybe_dump(const char * name)
{
    const char * dir = getenv("RATIMOS_DUMP_DIR");
    if (dir == NULL) {
        return;
    }
    lv_refr_now(NULL);
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    FILE * f = fopen(path, "wb");
    if (!f) {
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    for (int y = 0; y < RATIMOS_SCREEN_H; y++) {
        for (int x = 0; x < RATIMOS_SCREEN_W; x++) {
            uint16_t v = px565(x, y);
            int r = (v >> 11) & 0x1f, g = (v >> 5) & 0x3f, b = v & 0x1f;
            fputc((r << 3) | (r >> 2), f);
            fputc((g << 2) | (g >> 4), f);
            fputc((b << 3) | (b >> 2), f);
        }
    }
    fclose(f);
}

static void tap_row(lv_obj_t * row)
{
    TEST_ASSERT_TRUE(lv_obj_has_flag(row, LV_OBJ_FLAG_CLICKABLE));
    lv_obj_send_event(row, LV_EVENT_CLICKED, NULL);
}

/* Abre o seletor por ./home/config -> toque na linha (o caminho real). */
static lv_obj_t * open_picker(int config_row_index)
{
    ratimos_config_show(NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_update_layout(scr);
    lv_obj_t * row = lv_obj_get_child(content_of(scr), config_row_index);
    tap_row(row);
    lv_obj_t * picker = lv_screen_active();
    lv_obj_update_layout(picker);
    return picker;
}
#define OPEN_TEMA()         open_picker(2)
#define OPEN_CARREGAMENTO() open_picker(3)

static lv_obj_t * picker_row(lv_obj_t * picker, int i)
{
    return lv_obj_get_child(content_of(picker), i);
}

static lv_obj_t * picker_button(lv_obj_t * picker, int which) /* 0 previa, 1 aplicar */
{
    lv_obj_t * btn_row = lv_obj_get_child(content_of(picker), 4);
    return lv_obj_get_child(btn_row, which);
}

static void reset_storage(void)
{
    remove("assets/save/settings.bin");
    ratimos_storage_index_settings();
    ratimos_theme_apply(RATIMOS_THEME_CLASSICO);
}

void setUp(void)
{
    ratimos_theme_preview_end();
    reset_storage();
    /* Home em cache de testes anteriores usa o tema antigo: a geracao do tema a refaz. */
    lv_indev_enable(NULL, true);
}

void tearDown(void)
{
    ratimos_theme_preview_end();
    lv_indev_enable(NULL, true);
    rmdir("assets/save/settings.bin.tmp");
    /* uma tela qualquer: deleta a do teste e solta seus timers */
    lv_obj_t * s = lv_obj_create(NULL);
    ratimos_screen_load(s);
    pump_ms(50);
}

/* ------------------------------------------------------------------ *
 * ./home/config
 * ------------------------------------------------------------------ */

static const char * row_title(lv_obj_t * row)
{
    lv_obj_t * text_col = lv_obj_get_child(row, 1);
    return lv_label_get_text(lv_obj_get_child(text_col, 0));
}

static const char * row_subtitle(lv_obj_t * row)
{
    lv_obj_t * text_col = lv_obj_get_child(row, 1);
    return lv_label_get_text(lv_obj_get_child(text_col, 1));
}

void test_config_rows_are_in_the_sketch_order_with_tema_and_carregamento_tappable(void)
{
    ratimos_config_show(NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_update_layout(scr);
    maybe_dump("config");
    TEST_ASSERT_EQUAL_STRING("./home/config", active_path());

    lv_obj_t * content = content_of(scr);
    TEST_ASSERT_EQUAL_UINT32(6, lv_obj_get_child_count(content));
    const char * order[] = { "brilho", "volume", "tema", "carregamento", "firmware", "armazenamento" };
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL_STRING(order[i], row_title(lv_obj_get_child(content, i)));
    }

    lv_obj_t * tema = lv_obj_get_child(content, 2);
    lv_obj_t * carr = lv_obj_get_child(content, 3);
    TEST_ASSERT_EQUAL_STRING("clássico", row_subtitle(tema));
    TEST_ASSERT_EQUAL_STRING("hastear a bandeira", row_subtitle(carr));
    for (int i = 0; i < 6; i++) {
        lv_obj_t * row = lv_obj_get_child(content, i);
        TEST_ASSERT_EQUAL_INT(RATIMOS_LIST_ROW_H, lv_obj_get_height(row));
        /* contrato de indice de filho: badge=0, text_col=1, nada mais (a seta e' desenhada) */
        TEST_ASSERT_EQUAL_UINT32(2, lv_obj_get_child_count(row));
        bool tappable = i == 2 || i == 3;
        uint32_t bg = lv_color_to_u32(lv_obj_get_style_bg_color(row, LV_PART_MAIN));
        if (tappable) {
            TEST_ASSERT_TRUE(lv_obj_has_flag(row, LV_OBJ_FLAG_CLICKABLE));
            TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL_TAPPABLE), bg);
        } else {
            /* as informativas continuam como eram (fundo PANEL, sem o tom de "toque aqui") */
            TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL), bg);
        }
    }
    /* os icones sao os pixel-art novos, 26x26 */
    TEST_ASSERT_EQUAL_PTR(ratimos_icon_by_id("cfg_tema"), lv_image_get_src(lv_obj_get_child(tema, 0)));
    TEST_ASSERT_EQUAL_PTR(ratimos_icon_by_id("cfg_carregamento"), lv_image_get_src(lv_obj_get_child(carr, 0)));
}

void test_config_tappable_rows_draw_the_pixel_arrow_and_informative_rows_do_not(void)
{
    ratimos_config_show(NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_update_layout(scr);
    lv_refr_now(NULL);

    lv_obj_t * content = content_of(scr);
    lv_area_t a;
    lv_obj_get_coords(lv_obj_get_child(content, 2), &a);
    int32_t tip = a.x2 - 13, cy = (a.y1 + a.y2) / 2;
    uint16_t want = lv_color_to_u16(RATIMOS_COLOR_TEXT);
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(want, px565(tip, cy), "arrow tip");
    TEST_ASSERT_EQUAL_HEX16(want, px565(tip - 4, cy - 4)); /* base da seta: 9px de altura */
    TEST_ASSERT_EQUAL_HEX16(want, px565(tip - 4, cy + 4));

    /* a linha informativa (volume) nao tem seta */
    lv_obj_get_coords(lv_obj_get_child(content, 1), &a);
    tip = a.x2 - 13;
    cy = (a.y1 + a.y2) / 2;
    TEST_ASSERT_TRUE(px565(tip, cy) != want);
}

void test_config_subtitles_follow_the_applied_values_when_coming_back(void)
{
    lv_obj_t * picker = OPEN_TEMA();
    tap_row(picker_row(picker, RATIMOS_THEME_NOITE));
    lv_obj_send_event(picker_button(picker, 1), LV_EVENT_CLICKED, NULL); /* aplicar */
    pump_ms(6000);                                                       /* boot + home + volta */
    TEST_ASSERT_EQUAL_STRING("./home/config/tema", active_path());

    press_back(lv_screen_active());
    TEST_ASSERT_EQUAL_STRING("./home/config", active_path());
    lv_obj_t * content = content_of(lv_screen_active());
    TEST_ASSERT_EQUAL_STRING("noite estrelada", row_subtitle(lv_obj_get_child(content, 2)));
    TEST_ASSERT_EQUAL_STRING("hastear a bandeira", row_subtitle(lv_obj_get_child(content, 3)));
}

/* ------------------------------------------------------------------ *
 * ./home/config/tema
 * ------------------------------------------------------------------ */

void test_tema_picker_has_4_big_rows_with_thumbnails_default_and_in_use_marks(void)
{
    lv_obj_t * picker = OPEN_TEMA();
    maybe_dump("picker_tema");
    TEST_ASSERT_EQUAL_STRING("./home/config/tema", active_path());

    lv_obj_t * content = content_of(picker);
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(content)); /* 4 linhas + a fileira de botoes */

    for (int i = 0; i < 4; i++) {
        lv_obj_t * row = picker_row(picker, i);
        TEST_ASSERT_EQUAL_INT(72, lv_obj_get_height(row));
        TEST_ASSERT_TRUE_MESSAGE(lv_obj_has_flag(row, LV_OBJ_FLAG_CLICKABLE), "the whole row is the tap target");

        lv_obj_t * frame = lv_obj_get_child(row, 0);
        TEST_ASSERT_EQUAL_INT(42, lv_obj_get_width(frame));
        TEST_ASSERT_EQUAL_INT(62, lv_obj_get_height(frame));
        TEST_ASSERT_FALSE(lv_obj_has_flag(frame, LV_OBJ_FLAG_CLICKABLE));
        lv_obj_t * img = lv_obj_get_child(frame, 0);
        TEST_ASSERT_EQUAL_PTR(&lv_image_class, lv_obj_get_class(img));
        TEST_ASSERT_EQUAL_PTR(ratimos_theme_get((ratimos_theme_id_t) i)->home_thumb, lv_image_get_src(img));
        TEST_ASSERT_EQUAL_INT(40, lv_obj_get_width(img));
        TEST_ASSERT_EQUAL_INT(60, lv_obj_get_height(img));

        lv_obj_t * col = lv_obj_get_child(row, 1);
        TEST_ASSERT_FALSE(lv_obj_has_flag(col, LV_OBJ_FLAG_CLICKABLE));
        TEST_ASSERT_NOT_NULL(find_label(row, ratimos_theme_get((ratimos_theme_id_t) i)->name));
        /* "padrao" so' no Classico; "em uso" so' no aplicado (Classico) */
        TEST_ASSERT_EQUAL_INT(i == 0, find_label(row, "padrão") != NULL);
        lv_obj_t * in_use = find_label(row, "em uso");
        TEST_ASSERT_EQUAL_INT(i == 0, in_use != NULL);
        if (in_use) {
            TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_IN_USE),
                                      lv_color_to_u32(lv_obj_get_style_text_color(in_use, LV_PART_MAIN)));
        }
    }

    /* tudo cabe sem rolar */
    TEST_ASSERT_TRUE_MESSAGE(lv_obj_get_scroll_bottom(content) <= 0, "the picker content overflows the screen");
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_scroll_top(content));

    lv_obj_t * prev = picker_button(picker, 0);
    lv_obj_t * apply = picker_button(picker, 1);
    TEST_ASSERT_EQUAL_INT(136, lv_obj_get_width(prev));
    TEST_ASSERT_EQUAL_INT(36, lv_obj_get_height(prev));
    TEST_ASSERT_EQUAL_INT(136, lv_obj_get_width(apply));
    TEST_ASSERT_EQUAL_STRING("prévia", lv_label_get_text(lv_obj_get_child(prev, 0)));
    TEST_ASSERT_EQUAL_STRING("aplicar", lv_label_get_text(lv_obj_get_child(apply, 0)));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_title_8, lv_obj_get_style_text_font(lv_obj_get_child(apply, 0), LV_PART_MAIN));
    /* aplicar so' fica em destaque quando a selecao difere do aplicado */
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL),
                              lv_color_to_u32(lv_obj_get_style_bg_color(apply, LV_PART_MAIN)));
}

void test_tapping_a_row_only_highlights_it_and_never_saves(void)
{
    lv_obj_t * picker = OPEN_TEMA();
    lv_obj_t * apply = picker_button(picker, 1);

    tap_row(picker_row(picker, RATIMOS_THEME_XADREZ));
    maybe_dump("picker_tema_selecionado");
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL_ACTIVE),
                              lv_color_to_u32(lv_obj_get_style_bg_color(picker_row(picker, 3), LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_90, lv_obj_get_style_bg_opa(picker_row(picker, 3), LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL),
                              lv_color_to_u32(lv_obj_get_style_bg_color(picker_row(picker, 0), LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_ACCENT),
                              lv_color_to_u32(lv_obj_get_style_bg_color(apply, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());

    /* a seta aparece na linha selecionada */
    lv_refr_now(NULL);
    lv_area_t a;
    lv_obj_get_coords(picker_row(picker, 3), &a);
    TEST_ASSERT_EQUAL_HEX16(lv_color_to_u16(RATIMOS_COLOR_TEXT), px565(a.x2 - 13, (a.y1 + a.y2) / 2));
    lv_obj_get_coords(picker_row(picker, 0), &a);
    TEST_ASSERT_TRUE(px565(a.x2 - 13, (a.y1 + a.y2) / 2) != lv_color_to_u16(RATIMOS_COLOR_TEXT));

    /* voltar a selecionar o aplicado apaga o destaque do aplicar */
    tap_row(picker_row(picker, 0));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL),
                              lv_color_to_u32(lv_obj_get_style_bg_color(apply, LV_PART_MAIN)));
}

void test_tema_preview_plays_the_boot_then_the_home_with_the_selected_theme_and_saves_nothing(void)
{
    int base_timers = count_timers();
    lv_obj_t * picker = OPEN_TEMA();
    tap_row(picker_row(picker, RATIMOS_THEME_XADREZ));
    lv_obj_send_event(picker_button(picker, 0), LV_EVENT_CLICKED, NULL); /* previa */

    /* o boot toca com o tema SELECIONADO e o carregamento APLICADO */
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(lv_screen_active()));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_splash_active_theme());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_splash_active_style());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_theme_displayed()->id);

    pump_ms(3200); /* ~2s de boot + 0.4 de "pronto!" */
    lv_obj_t * home = lv_screen_active();
    TEST_ASSERT_EQUAL_STRING("./home", active_path());
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_xadrez_desc, lv_image_get_src(lv_obj_get_child(home, 0)));
    TEST_ASSERT_FALSE_MESSAGE(input_enabled(), "input must be off during the home interlude");

    pump_ms(2800); /* ~2,6s de home */
    TEST_ASSERT_EQUAL_STRING("./home/config/tema", active_path());
    TEST_ASSERT_TRUE_MESSAGE(input_enabled(), "input must be back on");
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_displayed()->id);
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_applied()->id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id(), "a preview must not save");
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
    TEST_ASSERT_FALSE_MESSAGE(label_visible(find_label(lv_screen_active(), "aplicado!")), "no toast after a preview");
    pump_ms(100);
    TEST_ASSERT_EQUAL_INT(base_timers, count_timers());
}

void test_tema_apply_saves_only_the_theme_shows_the_new_home_and_the_toast(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_boot_style(RATIMOS_BOOT_STYLE_AMEIAS)); /* o outro campo nao pode mudar */
    int base_timers = count_timers();
    lv_obj_t * picker = OPEN_TEMA();
    tap_row(picker_row(picker, RATIMOS_THEME_CASTELO));
    lv_obj_send_event(picker_button(picker, 1), LV_EVENT_CLICKED, NULL); /* aplicar */

    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CASTELO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_AMEIAS, ratimos_storage_get_boot_style());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_AMEIAS, ratimos_splash_active_style());

    pump_ms(3200);
    lv_obj_t * home = lv_screen_active();
    TEST_ASSERT_EQUAL_STRING("./home", active_path());
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_castelo_desc, lv_image_get_src(lv_obj_get_child(home, 0)));
    TEST_ASSERT_FALSE(input_enabled());

    pump_ms(1700); /* ~1,5s de home */
    TEST_ASSERT_EQUAL_STRING("./home/config/tema", active_path());
    TEST_ASSERT_TRUE(input_enabled());
    lv_obj_t * toast = find_label(lv_screen_active(), "aplicado!");
    TEST_ASSERT_NOT_NULL(toast);
    maybe_dump("picker_aplicado_toast");
    TEST_ASSERT_TRUE_MESSAGE(label_visible(toast), "toast 'aplicado!' must show after applying");
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_IN_USE),
                              lv_color_to_u32(lv_obj_get_style_text_color(toast, LV_PART_MAIN)));
    /* "em uso" na linha nova (e so' nela) */
    TEST_ASSERT_NOT_NULL(find_label(picker_row(lv_screen_active(), RATIMOS_THEME_CASTELO), "em uso"));
    TEST_ASSERT_NULL(find_label(picker_row(lv_screen_active(), RATIMOS_THEME_CLASSICO), "em uso"));

    pump_ms(1800); /* o toast some em ~1,6s */
    TEST_ASSERT_FALSE(label_visible(find_label(lv_screen_active(), "aplicado!")));
    TEST_ASSERT_EQUAL_INT(base_timers, count_timers());
}

/* ------------------------------------------------------------------ *
 * ./home/config/carregamento
 * ------------------------------------------------------------------ */

void test_carregamento_picker_thumbnails_stack_the_style_over_the_applied_theme_boot_bg(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_NOITE));
    ratimos_theme_apply(RATIMOS_THEME_NOITE);
    lv_obj_t * picker = OPEN_CARREGAMENTO();
    maybe_dump("picker_carregamento_noite");
    TEST_ASSERT_EQUAL_STRING("./home/config/carregamento", active_path());

    for (int i = 0; i < 4; i++) {
        lv_obj_t * row = picker_row(picker, i);
        TEST_ASSERT_EQUAL_INT(72, lv_obj_get_height(row));
        lv_obj_t * frame = lv_obj_get_child(row, 0);
        TEST_ASSERT_EQUAL_UINT32(2, lv_obj_get_child_count(frame));
        TEST_ASSERT_EQUAL_PTR(ratimos_theme_get(RATIMOS_THEME_NOITE)->boot_bg_thumb,
                              lv_image_get_src(lv_obj_get_child(frame, 0)));
        TEST_ASSERT_EQUAL_PTR(ratimos_boot_style_thumb((ratimos_boot_style_t) i),
                              lv_image_get_src(lv_obj_get_child(frame, 1)));
        TEST_ASSERT_NOT_NULL(find_label(row, ratimos_boot_style_name((ratimos_boot_style_t) i)));
        TEST_ASSERT_EQUAL_INT(i == 0, find_label(row, "padrão") != NULL);
        TEST_ASSERT_EQUAL_INT(i == 0, find_label(row, "em uso") != NULL);
    }
    TEST_ASSERT_TRUE(lv_obj_get_scroll_bottom(content_of(picker)) <= 0);
}

void test_carregamento_apply_saves_only_the_boot_style(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_CASTELO));
    ratimos_theme_apply(RATIMOS_THEME_CASTELO);
    lv_obj_t * picker = OPEN_CARREGAMENTO();
    tap_row(picker_row(picker, RATIMOS_BOOT_STYLE_TORRES));
    lv_obj_send_event(picker_button(picker, 1), LV_EVENT_CLICKED, NULL);

    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_TORRES, ratimos_storage_get_boot_style());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CASTELO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CASTELO, ratimos_splash_active_theme()); /* sobre o tema em uso */
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_TORRES, ratimos_splash_active_style());

    pump_ms(3200);
    TEST_ASSERT_EQUAL_STRING("./home", active_path());
    pump_ms(1700);
    TEST_ASSERT_EQUAL_STRING("./home/config/carregamento", active_path());
    TEST_ASSERT_TRUE(input_enabled());
    TEST_ASSERT_TRUE(label_visible(find_label(lv_screen_active(), "aplicado!")));
    TEST_ASSERT_NOT_NULL(find_label(picker_row(lv_screen_active(), RATIMOS_BOOT_STYLE_TORRES), "em uso"));
}

void test_carregamento_preview_plays_the_selected_style_on_the_applied_theme_and_saves_nothing(void)
{
    lv_obj_t * picker = OPEN_CARREGAMENTO();
    tap_row(picker_row(picker, RATIMOS_BOOT_STYLE_LETREIRO));
    lv_obj_send_event(picker_button(picker, 0), LV_EVENT_CLICKED, NULL);
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_splash_active_theme());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_LETREIRO, ratimos_splash_active_style());
    pump_ms(3200 + 2800);
    TEST_ASSERT_EQUAL_STRING("./home/config/carregamento", active_path());
    TEST_ASSERT_TRUE(input_enabled());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

/* ------------------------------------------------------------------ *
 * Caminhos de falha e seguranca
 * ------------------------------------------------------------------ */

void test_a_failed_save_shows_the_message_and_changes_nothing(void)
{
    ratimos_storage_set_theme_id(RATIMOS_THEME_CLASSICO); /* garante assets/save/ */
    TEST_ASSERT_EQUAL_INT(0, mkdir("assets/save/settings.bin.tmp", 0777)); /* a escrita atomica nao consegue abrir o .tmp */
    lv_obj_t * picker = OPEN_TEMA();
    tap_row(picker_row(picker, RATIMOS_THEME_NOITE));
    lv_obj_send_event(picker_button(picker, 1), LV_EVENT_CLICKED, NULL);

    TEST_ASSERT_EQUAL_PTR_MESSAGE(picker, lv_screen_active(), "a failed save must not leave the picker");
    TEST_ASSERT_NOT_NULL(find_label(picker, "não foi possível salvar"));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_applied()->id);
    TEST_ASSERT_TRUE(input_enabled());
    TEST_ASSERT_EQUAL_UINT32(5, lv_obj_get_child_count(content_of(picker)));
}

void test_deleting_the_home_during_the_interlude_never_leaves_the_input_off(void)
{
    lv_obj_t * picker = OPEN_TEMA();
    tap_row(picker_row(picker, RATIMOS_THEME_NOITE));
    lv_obj_send_event(picker_button(picker, 0), LV_EVENT_CLICKED, NULL);
    pump_ms(3200);
    TEST_ASSERT_FALSE(input_enabled());

    /* alguem troca/deleta a tela do interludio (ex.: a home em cache foi reconstruida) */
    lv_obj_t * home = lv_screen_active();
    lv_obj_t * other = lv_obj_create(NULL);
    ratimos_screen_clear_persistent(home);
    ratimos_screen_load(other); /* deleta a home */
    TEST_ASSERT_TRUE_MESSAGE(input_enabled(), "the DELETE of the interlude screen must re-enable the input");
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_displayed()->id);
    pump_ms(3000); /* nenhum timer disparando depois do abort */
    TEST_ASSERT_TRUE(input_enabled());
    ratimos_home_screen_show(NULL); /* reconstroi a home persistente para os proximos testes */
}

void test_voltar_goes_to_config_from_both_pickers(void)
{
    lv_obj_t * picker = OPEN_TEMA();
    press_back(picker);
    TEST_ASSERT_EQUAL_STRING("./home/config", active_path());
    picker = OPEN_CARREGAMENTO();
    press_back(picker);
    TEST_ASSERT_EQUAL_STRING("./home/config", active_path());
}

/* ------------------------------------------------------------------ *
 * Heap / timers: previas repetidas das combinacoes
 * ------------------------------------------------------------------ */

static void run_flow(int config_row, int sel)
{
    lv_obj_t * picker = open_picker(config_row);
    tap_row(picker_row(picker, sel));
    lv_obj_send_event(picker_button(picker, 0), LV_EVENT_CLICKED, NULL); /* previa */
    pump_ms(6100);
    TEST_ASSERT_TRUE(input_enabled());
}

static size_t run_flow_lap(void)
{
    for (int t = 0; t < 4; t++) {
        run_flow(2, t);
    }
    for (int s = 0; s < 4; s++) {
        run_flow(3, s);
    }
    pump_ms(2000); /* deixa o toast/timers assentarem */
    return ratimos_heap_used();
}

void test_three_laps_of_picker_previews_keep_heap_timers_and_input_stable(void)
{
    int base_timers = count_timers();
    run_flow_lap();
    size_t lap2 = run_flow_lap();
    size_t lap3 = run_flow_lap();
    fprintf(stderr, "heap picker previews x3 laps: lap2 %u, lap3 %u bytes used\n", (unsigned) lap2, (unsigned) lap3);
    TEST_ASSERT_TRUE_MESSAGE(lap3 <= lap2 + lap2 / 50, "LVGL heap grew from lap 2 to lap 3");
    TEST_ASSERT_EQUAL_INT_MESSAGE(base_timers, count_timers(), "orphan lv_timer after the picker laps");
    TEST_ASSERT_TRUE(input_enabled());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

int main(void)
{
    char tmpl[] = "/tmp/ratimos-pickers-XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    lv_init();
    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);
    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, dummy_indev_read_cb);

    ratimos_storage_mount();
    ratimos_storage_index_games();
    ratimos_storage_index_settings();
    ratimos_storage_index_game_state();

    UNITY_BEGIN();
    RUN_TEST(test_config_rows_are_in_the_sketch_order_with_tema_and_carregamento_tappable);
    RUN_TEST(test_config_tappable_rows_draw_the_pixel_arrow_and_informative_rows_do_not);
    RUN_TEST(test_tema_picker_has_4_big_rows_with_thumbnails_default_and_in_use_marks);
    RUN_TEST(test_tapping_a_row_only_highlights_it_and_never_saves);
    RUN_TEST(test_tema_preview_plays_the_boot_then_the_home_with_the_selected_theme_and_saves_nothing);
    RUN_TEST(test_tema_apply_saves_only_the_theme_shows_the_new_home_and_the_toast);
    RUN_TEST(test_config_subtitles_follow_the_applied_values_when_coming_back);
    RUN_TEST(test_carregamento_picker_thumbnails_stack_the_style_over_the_applied_theme_boot_bg);
    RUN_TEST(test_carregamento_apply_saves_only_the_boot_style);
    RUN_TEST(test_carregamento_preview_plays_the_selected_style_on_the_applied_theme_and_saves_nothing);
    RUN_TEST(test_a_failed_save_shows_the_message_and_changes_nothing);
    RUN_TEST(test_deleting_the_home_during_the_interlude_never_leaves_the_input_off);
    RUN_TEST(test_voltar_goes_to_config_from_both_pickers);
    RUN_TEST(test_three_laps_of_picker_previews_keep_heap_timers_and_input_stable);
    return UNITY_END();
}
