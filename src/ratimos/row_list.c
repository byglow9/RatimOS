#include "row_list.h"
#include "theme.h"

lv_obj_t * ratimos_row_create(lv_obj_t * parent,
                               const char * letter,
                               const char * title,
                               const char * subtitle,
                               lv_event_cb_t click_cb)
{
    lv_obj_t * row = ratimos_panel_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    if (letter) {
        ratimos_badge_create(row, letter);
    }

    lv_obj_t * text_col = lv_obj_create(row);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_flex_grow(text_col, 1);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title_lbl = lv_label_create(text_col);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, RATIMOS_COLOR_TEXT, 0);

    if (subtitle) {
        lv_obj_t * sub_lbl = lv_label_create(text_col);
        lv_label_set_text(sub_lbl, subtitle);
        lv_obj_set_style_text_color(sub_lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    }

    if (click_cb) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, click_cb, LV_EVENT_CLICKED, NULL);
    }

    return row;
}
