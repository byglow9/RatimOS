#include "album_app.h"
#include "../app_shell.h"
#include "../theme.h"

static lv_obj_t * photo_tile_create(lv_obj_t * parent)
{
    lv_obj_t * tile = ratimos_panel_create(parent);
    lv_obj_set_size(tile, 130, 100);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * lbl = lv_label_create(tile);
    lv_label_set_text(lbl, "sem foto");
    lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    return tile;
}

void ratimos_album_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("galeria", "toque pra abrir");

    lv_obj_set_flex_flow(shell.content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(shell.content, 8, 0);

    /* TODO (Fase 3/6): grid real lendo miniaturas do SD (fotos sincronizadas + camera) */
    for (int i = 0; i < 4; i++) {
        photo_tile_create(shell.content);
    }

    lv_screen_load(shell.screen);
}
