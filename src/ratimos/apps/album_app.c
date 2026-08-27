#include "album_app.h"
#include "../app_shell.h"
#include "../theme.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Cache-once (same rationale as jogos_app.c / musica_app.c / cartas_app.c /
 * 01-01) -- avoids rebuilding a new lv_obj_t screen on every visit, which
 * exhausts LVGL's builtin heap after a handful of visits.
 */
static lv_obj_t * s_album_screen = NULL;

static lv_obj_t * photo_tile_create(lv_obj_t * parent, const char * title)
{
    lv_obj_t * tile = ratimos_panel_create(parent);
    lv_obj_set_size(tile, 130, 100);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * lbl = lv_label_create(tile);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_DOTS);
    return tile;
}

static lv_obj_t * build_album_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("galeria", "toque para abrir");

    lv_obj_set_flex_flow(shell.content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(shell.content, 8, 0);

    ratimos_photo_t photos[4];
    size_t n = ratimos_storage_list_photos(photos, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhuma foto ainda", "tire uma foto ou aguarde a sincronizacao", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            photo_tile_create(shell.content, photos[i].title);
        }
    }

    return shell.screen;
}

void ratimos_album_show(lv_event_t * e)
{
    (void) e;
    if (!s_album_screen) {
        s_album_screen = build_album_screen();
    }
    lv_screen_load(s_album_screen);
}
