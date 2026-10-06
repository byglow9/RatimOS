#include "row_list.h"
#include "theme.h"
#include "fonts/ratimos_fonts.h"

/*
 * Medidas do `.card-c` / `.row-inner` do sketch 003-C (plano 02.1-14):
 * padding 9px dentro da moldura, 10px entre icone e texto, 2px entre
 * titulo e descricao, icone 26px (os icones de linha ja sao gerados em
 * 26x26 por tools/generate_icon_art.py -- nada e' escalado aqui).
 */
#define RATIMOS_ROW_PAD          9
#define RATIMOS_ROW_ICON_GAP     10
#define RATIMOS_ROW_TEXT_GAP     2
#define RATIMOS_ROW_SHADOW_OFS   2

/*
 * Sombra em pixel do icone (`drop-shadow(2px 2px 0 rgba(0,0,0,0.7))` do
 * sketch, icones.md): a mesma imagem tingida de preto a 70%, desenhada 2px
 * abaixo/direita ANTES do icone, no LV_EVENT_DRAW_MAIN_BEGIN do proprio
 * lv_image -- nenhum objeto filho extra (o contrato badge=0/text_col=1 de
 * ratimos_row_create() continua valendo, ver jogos_app.c).
 */
static void row_icon_shadow_cb(lv_event_t * e)
{
    lv_obj_t * img = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_event_set_ext_draw_size(e, RATIMOS_ROW_SHADOW_OFS);
        return;
    }

    const void * src = lv_image_get_src(img);
    if (src == NULL) {
        return;
    }
    lv_area_t area;
    lv_obj_get_coords(img, &area);
    lv_area_move(&area, RATIMOS_ROW_SHADOW_OFS, RATIMOS_ROW_SHADOW_OFS);

    lv_draw_image_dsc_t dsc;
    lv_draw_image_dsc_init(&dsc);
    dsc.src = src;
    dsc.recolor = lv_color_black();
    dsc.recolor_opa = LV_OPA_COVER;
    dsc.opa = LV_OPA_70;
    lv_draw_image(lv_event_get_layer(e), &dsc, &area);
}

/*
 * Uma linha so', cortada com "..." (`white-space:nowrap; text-overflow:
 * ellipsis` do sketch). LV_LABEL_LONG_MODE_DOTS so' corta quando o label
 * tem altura fixa -- com LV_SIZE_CONTENT ele quebra a linha e a linha da
 * lista cresce. Altura = uma linha da fonte.
 */
static void single_line(lv_obj_t * label, const lv_font_t * font)
{
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_height(label, lv_font_get_line_height(font));
}

lv_obj_t * ratimos_row_create(lv_obj_t * parent,
                               const char * letter,
                               const char * title,
                               const char * subtitle,
                               lv_event_cb_t click_cb)
{
    lv_obj_t * row = ratimos_panel_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, RATIMOS_ROW_PAD, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, RATIMOS_ROW_ICON_GAP, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    if (letter) {
        lv_obj_t * icon = ratimos_badge_create(row, letter);
        if (lv_obj_get_class(icon) == &lv_image_class) {
            lv_obj_add_event_cb(icon, row_icon_shadow_cb, LV_EVENT_DRAW_MAIN_BEGIN, NULL);
            lv_obj_add_event_cb(icon, row_icon_shadow_cb, LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
            lv_obj_refresh_ext_draw_size(icon);
        }
    }

    lv_obj_t * text_col = lv_obj_create(row);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_flex_grow(text_col, 1);
    /* Altura = conteudo. Sem isto o text_col (lv_obj_create +
     * remove_style_all) herdava a altura default do lv_obj (~LV_DPI_DEF,
     * ~130px) e cada linha ficava com ~150px, com o icone centralizado
     * longe do texto (deferred-items #2, plano 02.1-14). */
    lv_obj_set_height(text_col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(text_col, RATIMOS_ROW_TEXT_GAP, 0);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);
    /* lv_obj_remove_style_all() only strips styles, never flags -- text_col
     * (a plain lv_obj_create()) keeps LVGL's default LV_OBJ_FLAG_CLICKABLE
     * with no handler of its own, which silently swallows taps over the
     * title/subtitle area (most of the row's width) before they can reach
     * `row`'s own click_cb below. Same bug class as the badge hitbox fix
     * in theme.c -- clear it explicitly. */
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_CLICKABLE);

    /* Titulo: Press Start 2P 8px (`.row-inner .t` do sketch). */
    lv_obj_t * title_lbl = lv_label_create(text_col);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title_lbl, &ratimos_font_title_8, 0);
    lv_obj_set_width(title_lbl, lv_pct(100));
    single_line(title_lbl, &ratimos_font_title_8);

    /* Descricao: JetBrains Mono 10px esmaecida (`.row-inner .d`). */
    if (subtitle) {
        lv_obj_t * sub_lbl = lv_label_create(text_col);
        lv_label_set_text(sub_lbl, subtitle);
        lv_obj_set_style_text_color(sub_lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(sub_lbl, &ratimos_font_mono_10, 0);
        lv_obj_set_width(sub_lbl, lv_pct(100));
        single_line(sub_lbl, &ratimos_font_mono_10);
    }

    if (click_cb) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, click_cb, LV_EVENT_CLICKED, NULL);
    }

    return row;
}
