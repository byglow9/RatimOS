#include "theme.h"

void ratimos_theme_apply_screen(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
}

lv_obj_t * ratimos_panel_create(lv_obj_t * parent)
{
    lv_obj_t * panel = lv_obj_create(parent);
    lv_obj_set_style_bg_color(panel, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 6, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_style_text_color(panel, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_shadow_width(panel, 0, 0);
    return panel;
}

/* selo redondo com uma letra — placeholder de ícone até termos arte pixel própria */
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * letter)
{
    lv_obj_t * badge = lv_obj_create(parent);
    lv_obj_set_size(badge, 28, 28);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(badge);
    lv_label_set_text(label, letter);
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_BG, 0);
    lv_obj_center(label);
    return badge;
}
