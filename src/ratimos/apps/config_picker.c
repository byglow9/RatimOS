#include "config_picker.h"

#include "config_app.h"
#include "../app_shell.h"
#include "../boot_styles.h"
#include "../fonts/ratimos_fonts.h"
#include "../home_screen.h"
#include "../splash.h"
#include "../theme.h"
#include "../theme_catalog.h"
#include "../../storage/content_api.h"

/*
 * Medidas do `drawPicker` do sketch 010: linhas de 72px com gap de 10px,
 * miniatura 40x60 na moldura 42x62, botoes 136x36. A conta cabe sem rolar:
 * 4*72 + 3*10 + 10 + 4 + 36 = 368 <= RATIMOS_CONTENT_INNER_H. A linha INTEIRA
 * e' clicavel (alvo de toque grande, UAT G-02.1-1).
 */
#define PICKER_ROW_COUNT 4
_Static_assert(PICKER_ROW_COUNT == RATIMOS_THEME_COUNT && PICKER_ROW_COUNT == RATIMOS_BOOT_STYLE_COUNT,
               "o seletor mostra uma linha por tema e por carregamento");
#define PICKER_ROW_H     72
#define PICKER_ROW_GAP   10
#define PICKER_THUMB_W   40
#define PICKER_THUMB_H   60
#define PICKER_BTN_W     136
#define PICKER_BTN_H     36

/* Tempos do `startPreview` do 010. */
#define PICKER_PREVIEW_HOME_MS 2600 /* previa: home com o tema, sem salvar */
#define PICKER_APPLY_HOME_MS   1500 /* aplicar: home com o tema novo */
#define PICKER_TOAST_MS        1600

/* Linha selecionada (a seta e' desenhada so' nela). */
#define PICKER_ROW_SELECTED LV_OBJ_FLAG_USER_2

typedef enum { PICKER_TEMA = 0, PICKER_CARREGAMENTO } picker_kind_t;

/* ---- estado da tela do seletor (existe so' enquanto a tela existe) ---- */
static picker_kind_t s_kind;
static int s_sel;
static lv_obj_t * s_screen;
static lv_obj_t * s_rows[PICKER_ROW_COUNT];
static lv_obj_t * s_apply_btn;
static lv_obj_t * s_toast;
static lv_obj_t * s_error_label;
static lv_timer_t * s_toast_timer;

/* ---- estado do FLUXO previa/aplicar (sobrevive a tela do seletor ser deletada) ---- */
static picker_kind_t s_flow_kind;
static bool s_flow_save;           /* aplicar (true) ou previa (false) */
static bool s_flow_theme_preview;  /* ratimos_theme_preview_begin() pendente de preview_end() */
static lv_timer_t * s_interlude_timer;
static lv_obj_t * s_guard_screen;  /* tela ativa durante o interludio (entrada desligada) */
static bool s_input_off;

static void picker_show(picker_kind_t kind, bool with_toast);

/* ---------------------------------------------------------------------- *
 * Seta
 * ---------------------------------------------------------------------- */

void ratimos_config_draw_arrow(lv_layer_t * layer, const lv_area_t * row_area)
{
    int32_t tip = row_area->x2 - 13;
    int32_t cy = (row_area->y1 + row_area->y2) / 2;
    for (int32_t k = 0; k < 5; k++) {
        lv_draw_rect_dsc_t d;
        lv_draw_rect_dsc_init(&d);
        d.radius = 0;
        d.bg_color = RATIMOS_COLOR_TEXT;
        d.bg_opa = LV_OPA_COVER;
        d.border_width = 0;
        lv_area_t a = { tip - k, cy - k, tip - k, cy + k };
        lv_draw_rect(layer, &d, &a);
    }
}

static void row_arrow_draw_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    if (!lv_obj_has_flag(row, PICKER_ROW_SELECTED)) {
        return;
    }
    lv_area_t a;
    lv_obj_get_coords(row, &a);
    ratimos_config_draw_arrow(lv_event_get_layer(e), &a);
}

/* ---------------------------------------------------------------------- *
 * Dados das linhas
 * ---------------------------------------------------------------------- */

static int applied_index(picker_kind_t kind)
{
    return kind == PICKER_TEMA ? (int) ratimos_storage_get_theme_id() : (int) ratimos_storage_get_boot_style();
}

static const char * row_name(picker_kind_t kind, int i)
{
    return kind == PICKER_TEMA ? ratimos_theme_get((ratimos_theme_id_t) i)->name
                               : ratimos_boot_style_name((ratimos_boot_style_t) i);
}

/* ---------------------------------------------------------------------- *
 * Construcao
 * ---------------------------------------------------------------------- */

static void style_row(lv_obj_t * row, bool selected)
{
    lv_obj_set_style_bg_color(row, selected ? RATIMOS_COLOR_PANEL_ACTIVE : RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(row, selected ? LV_OPA_90 : (lv_opa_t) 191 /* ~75% */, 0);
    if (selected) {
        lv_obj_add_flag(row, PICKER_ROW_SELECTED);
    } else {
        lv_obj_remove_flag(row, PICKER_ROW_SELECTED);
    }
    lv_obj_invalidate(row);
}

static void refresh_apply_button(void)
{
    if (s_apply_btn == NULL) {
        return;
    }
    bool hot = s_sel != applied_index(s_kind);
    lv_obj_set_style_bg_color(s_apply_btn, hot ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(s_apply_btn, hot ? LV_OPA_90 : LV_OPA_70, 0);
}

static void row_clicked_cb(lv_event_t * e)
{
    /* So' destaca: nunca grava (salvar e' o "aplicar"). */
    s_sel = (int) (intptr_t) lv_event_get_user_data(e);
    for (int i = 0; i < PICKER_ROW_COUNT; i++) {
        if (s_rows[i]) {
            style_row(s_rows[i], i == s_sel);
        }
    }
    refresh_apply_button();
}

static lv_obj_t * thumb_image(lv_obj_t * frame, const lv_image_dsc_t * dsc)
{
    lv_obj_t * img = lv_image_create(frame);
    lv_image_set_src(img, dsc);
    lv_obj_set_pos(img, 1, 1);
    return img;
}

static lv_obj_t * text_label(lv_obj_t * parent, const lv_font_t * font, lv_color_t color, const char * text,
                             int32_t y)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, 0, y);
    return l;
}

static lv_obj_t * picker_row_create(lv_obj_t * parent, picker_kind_t kind, int i)
{
    lv_obj_t * row = ratimos_panel_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, PICKER_ROW_H);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_left(row, 6, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, row_clicked_cb, LV_EVENT_CLICKED, (void *) (intptr_t) i);
    lv_obj_add_event_cb(row, row_arrow_draw_cb, LV_EVENT_DRAW_POST, NULL);

    /* Moldura 42x62 com a miniatura 40x60. Nenhum filho clicavel por cima da
     * linha (licao de hitbox do 02.1-09): lv_obj_remove_style_all() nao limpa flags. */
    lv_obj_t * frame = lv_obj_create(row);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, PICKER_THUMB_W + 2, PICKER_THUMB_H + 2);
    lv_obj_set_style_bg_color(frame, RATIMOS_COLOR_BEVEL_DARK, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    if (kind == PICKER_TEMA) {
        thumb_image(frame, ratimos_theme_get((ratimos_theme_id_t) i)->home_thumb);
    } else {
        /* a miniatura do carregamento fica por cima do fundo de boot do tema EM USO */
        thumb_image(frame, ratimos_theme_applied()->boot_bg_thumb);
        thumb_image(frame, ratimos_boot_style_thumb((ratimos_boot_style_t) i));
    }

    lv_obj_t * col = lv_obj_create(row);
    lv_obj_remove_style_all(col);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, PICKER_THUMB_H + 2);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    text_label(col, &ratimos_font_mono_12, RATIMOS_COLOR_TEXT, row_name(kind, i), 10);
    if (i == 0) {
        text_label(col, &ratimos_font_mono_10, RATIMOS_COLOR_TEXT_MUTED, "padrão", 30);
    }
    if (i == applied_index(kind)) {
        text_label(col, &ratimos_font_mono_10, RATIMOS_COLOR_IN_USE, "em uso", 46);
    }

    style_row(row, i == s_sel);
    return row;
}

/* ---------------------------------------------------------------------- *
 * Fluxos previa / aplicar (`startPreview` do 010)
 * ---------------------------------------------------------------------- */

static void delete_timer(lv_timer_t ** t)
{
    if (*t != NULL) {
        lv_timer_delete(*t);
        *t = NULL;
    }
}

static void guard_deleted_cb(lv_event_t * e);

/* Religa a entrada de toque e solta o guarda. Chamado em TODO caminho de saida. */
static void input_restore(void)
{
    if (s_guard_screen != NULL) {
        lv_obj_remove_event_cb(s_guard_screen, guard_deleted_cb);
        s_guard_screen = NULL;
    }
    if (s_input_off) {
        lv_indev_enable(NULL, true);
        s_input_off = false;
    }
}

static void flow_abort(void)
{
    delete_timer(&s_interlude_timer);
    input_restore();
    if (s_flow_theme_preview) {
        ratimos_theme_preview_end();
        s_flow_theme_preview = false;
    }
}

/* A tela do interludio foi deletada no meio dele: nunca deixa a entrada desligada. */
static void guard_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_guard_screen = NULL; /* ja esta morrendo: nada de remove_event_cb nela */
    flow_abort();
}

static void interlude_done_cb(lv_timer_t * t)
{
    lv_timer_delete(t);
    s_interlude_timer = NULL;

    picker_kind_t kind = s_flow_kind;
    bool toast = s_flow_save;
    input_restore();
    if (s_flow_theme_preview) {
        ratimos_theme_preview_end();
        s_flow_theme_preview = false;
    }
    picker_show(kind, toast);
}

/* O boot (previa) terminou: mostra a home com o tema, com a entrada desligada. */
static void flow_boot_done_cb(void)
{
    ratimos_home_screen_show(NULL); /* reconstruida pela geracao do tema */

    s_guard_screen = lv_screen_active();
    lv_obj_add_event_cb(s_guard_screen, guard_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_indev_enable(NULL, false);
    s_input_off = true;

    s_interlude_timer = lv_timer_create(interlude_done_cb,
                                        s_flow_save ? PICKER_APPLY_HOME_MS : PICKER_PREVIEW_HOME_MS, NULL);
}

static void show_save_error(void)
{
    if (s_screen == NULL) {
        return;
    }
    if (s_error_label == NULL) {
        s_error_label = lv_label_create(s_screen);
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_FLOATING);
        lv_obj_set_style_text_font(s_error_label, &ratimos_font_mono_10, 0);
        lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
        lv_label_set_text(s_error_label, "não foi possível salvar");
        lv_obj_align(s_error_label, LV_ALIGN_TOP_MID, 0, 428);
    }
}

static void start_flow(bool save)
{
    flow_abort(); /* defensivo: um fluxo por vez */

    picker_kind_t kind = s_kind;
    ratimos_theme_id_t theme;
    ratimos_boot_style_t style;

    s_flow_kind = kind;
    s_flow_save = save;
    s_flow_theme_preview = false;

    if (kind == PICKER_TEMA) {
        theme = (ratimos_theme_id_t) s_sel;
        style = ratimos_storage_get_boot_style(); /* o carregamento APLICADO */
        if (save) {
            if (!ratimos_storage_set_theme_id(theme)) {
                show_save_error();
                return;
            }
            ratimos_theme_apply(theme);
        } else {
            ratimos_theme_preview_begin(theme);
            s_flow_theme_preview = true;
        }
    } else {
        theme = ratimos_theme_applied()->id; /* o tema APLICADO */
        style = (ratimos_boot_style_t) s_sel;
        if (save && !ratimos_storage_set_boot_style(style)) {
            show_save_error();
            return;
        }
    }

    /* O boot toca na previa e no aplicar: o carregamento novo so' aparece no
     * proximo liga (nota do 010). */
    ratimos_splash_preview(theme, style, flow_boot_done_cb);
}

static void preview_clicked_cb(lv_event_t * e)
{
    (void) e;
    start_flow(false);
}

static void apply_clicked_cb(lv_event_t * e)
{
    (void) e;
    start_flow(true);
}

/* ---------------------------------------------------------------------- *
 * Tela
 * ---------------------------------------------------------------------- */

static void toast_hide_cb(lv_timer_t * t)
{
    lv_timer_delete(t);
    s_toast_timer = NULL;
    if (s_toast != NULL) {
        lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    }
}

static void picker_deleted_cb(lv_event_t * e)
{
    /* Um seletor antigo deletado por um NOVO nao pode zerar os ponteiros do novo. */
    if (lv_event_get_target(e) != s_screen) {
        return;
    }
    delete_timer(&s_toast_timer);
    s_screen = NULL;
    s_apply_btn = NULL;
    s_toast = NULL;
    s_error_label = NULL;
    for (int i = 0; i < PICKER_ROW_COUNT; i++) {
        s_rows[i] = NULL;
    }
}

static lv_obj_t * button_with_title_font(lv_obj_t * parent, const char * text, lv_event_cb_t cb)
{
    lv_obj_t * btn = ratimos_button_create(parent, text, cb, PICKER_BTN_W, PICKER_BTN_H);
    lv_obj_set_style_text_font(lv_obj_get_child(btn, 0), &ratimos_font_title_8, 0);
    return btn;
}

static lv_obj_t * build_picker(picker_kind_t kind)
{
    const char * path = kind == PICKER_TEMA
                            ? "./home/config/tema"
                            : "./home/config/carregamento";
    ratimos_app_shell_t shell = ratimos_app_shell_create_with_back(path, NULL, ratimos_config_show);
    s_screen = shell.screen;
    lv_obj_set_style_pad_row(shell.content, PICKER_ROW_GAP, 0);

    for (int i = 0; i < PICKER_ROW_COUNT; i++) {
        s_rows[i] = picker_row_create(shell.content, kind, i);
    }

    lv_obj_t * btn_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, lv_pct(100), PICKER_BTN_H);
    lv_obj_set_style_margin_top(btn_row, 4, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    button_with_title_font(btn_row, "prévia", preview_clicked_cb);
    s_apply_btn = button_with_title_font(btn_row, "aplicar", apply_clicked_cb);
    refresh_apply_button();

    /* Toast "aplicado!": criado uma vez, escondido; flutua por cima da lista. */
    s_toast = ratimos_panel_create(shell.screen);
    lv_obj_add_flag(s_toast, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_toast, 240, 32);
    /* y ~360 do sketch; um pouco acima para nao cobrir os botoes (que aqui comecam em y 384) */
    lv_obj_set_pos(s_toast, (RATIMOS_SCREEN_W - 240) / 2, 344);
    lv_obj_set_style_pad_all(s_toast, 0, 0);
    lv_obj_set_style_bg_color(s_toast, RATIMOS_COLOR_BEVEL_DARK, 0);
    lv_obj_set_style_bg_opa(s_toast, LV_OPA_COVER, 0);
    lv_obj_t * toast_label = lv_label_create(s_toast);
    lv_label_set_text(toast_label, "aplicado!");
    lv_obj_set_style_text_font(toast_label, &ratimos_font_title_8, 0);
    lv_obj_set_style_text_color(toast_label, RATIMOS_COLOR_IN_USE, 0);
    lv_obj_center(toast_label);

    return shell.screen;
}

static void picker_show(picker_kind_t kind, bool with_toast)
{
    s_kind = kind;
    s_sel = applied_index(kind);
    s_error_label = NULL;

    lv_obj_t * scr = build_picker(kind);
    lv_obj_add_event_cb(scr, picker_deleted_cb, LV_EVENT_DELETE, NULL);

    if (with_toast) {
        lv_obj_remove_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
        delete_timer(&s_toast_timer);
        s_toast_timer = lv_timer_create(toast_hide_cb, PICKER_TOAST_MS, NULL);
        lv_timer_set_repeat_count(s_toast_timer, 1);
    }
    ratimos_screen_load(scr);
}

void ratimos_config_tema_show(lv_event_t * e)
{
    (void) e;
    picker_show(PICKER_TEMA, false);
}

void ratimos_config_carregamento_show(lv_event_t * e)
{
    (void) e;
    picker_show(PICKER_CARREGAMENTO, false);
}
