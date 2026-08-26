#include "status_bar.h"
#include "theme.h"

static lv_obj_t * bar_row_create(lv_obj_t * parent, lv_coord_t height)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, RATIMOS_SCREEN_W, height);
    lv_obj_set_style_bg_color(row, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

void ratimos_topbar_create(lv_obj_t * parent)
{
    lv_obj_t * row = bar_row_create(parent, 26);

    lv_obj_t * brand = lv_label_create(row);
    lv_label_set_text(brand, LV_SYMBOL_HOME " RatimOS");
    lv_obj_set_style_text_color(brand, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);

    lv_obj_t * right = lv_obj_create(row);
    lv_obj_remove_style_all(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 6, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * clock = lv_label_create(right);
    lv_label_set_text(clock, "--:--");
    lv_obj_set_style_text_color(clock, RATIMOS_COLOR_TEXT, 0);

    lv_obj_t * batt = lv_label_create(right);
    lv_label_set_text(batt, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(batt, RATIMOS_COLOR_TEXT, 0);
}

void ratimos_sectionbar_create(lv_obj_t * parent, const char * label)
{
    lv_obj_t * row = bar_row_create(parent, 24);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * dots = lv_label_create(row);
    lv_label_set_text(dots, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(dots, RATIMOS_COLOR_ACCENT, 0);

    lv_obj_t * title = lv_label_create(row);
    lv_label_set_text(title, label);
    lv_obj_set_style_text_color(title, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_pad_left(title, 6, 0);
}

void ratimos_bottombar_create(lv_obj_t * parent, const char * left_text, lv_event_cb_t left_cb,
                               const char * right_text)
{
    lv_obj_t * row = bar_row_create(parent, 22);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    if (left_cb) {
        lv_obj_t * btn = lv_button_create(row);
        lv_obj_remove_style_all(btn);
        lv_obj_add_event_cb(btn, left_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t * lbl = lv_label_create(btn);
        char buf[48];
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT " %s", left_text);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_ACCENT, 0);
    }
    else {
        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, left_text);
        lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    }

    lv_obj_t * right = lv_label_create(row);
    lv_label_set_text(right, right_text);
    lv_obj_set_style_text_color(right, RATIMOS_COLOR_TEXT_MUTED, 0);
}
