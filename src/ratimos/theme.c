#include "theme.h"
#include "icons.h"

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

/*
 * Selo redondo de 28x28px usado pelos launchers/linhas de lista
 * (row_list.c, home_screen.c). `icon_id` e' resolvido primeiro via
 * ratimos_icon_by_id() -- numa correspondencia, monta a arte de pixel
 * art project-authored (VISUAL-01/D-07) recortada em circulo; em NULL ou
 * sem correspondencia, cai para o selo original de circulo+letra (o
 * comportamento pre-D-07, preservado para nunca quebrar um chamador
 * existente e para nunca desreferenciar um icon_id NULL).
 */
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * icon_id)
{
    lv_obj_t * badge = lv_obj_create(parent);
    lv_obj_set_size(badge, 28, 28);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_set_style_clip_corner(badge, true, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    const lv_image_dsc_t * icon = ratimos_icon_by_id(icon_id);
    if (icon) {
        lv_obj_t * img = lv_image_create(badge);
        lv_image_set_src(img, icon);
        lv_obj_center(img);
        return badge;
    }

    lv_obj_t * label = lv_label_create(badge);
    lv_label_set_text(label, icon_id ? icon_id : "?");
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_BG, 0);
    lv_obj_center(label);
    return badge;
}
