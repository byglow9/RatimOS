#include "home_screen.h"
#include "theme.h"
#include "status_bar.h"
#include "apps/jogos_app.h"
#include "apps/musica_app.h"
#include "apps/album_app.h"
#include "apps/cartas_app.h"
#include "apps/config_app.h"

static lv_obj_t * s_home_screen = NULL;

static lv_obj_t * tile_create(lv_obj_t * parent, const char * letter, const char * title,
                               lv_coord_t w, lv_coord_t h, lv_event_cb_t click_cb)
{
    lv_obj_t * tile = ratimos_panel_create(parent);
    lv_obj_set_size(tile, w, h);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(tile, 4, 0);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tile, click_cb, LV_EVENT_CLICKED, NULL);

    ratimos_badge_create(tile, letter);

    lv_obj_t * lbl = lv_label_create(tile);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT, 0);

    return tile;
}

static lv_obj_t * build_home_screen(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    ratimos_topbar_create(scr);
    ratimos_sectionbar_create(scr, "home.mem");

    lv_obj_t * content = lv_obj_create(scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_width(content, RATIMOS_SCREEN_W);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);

    /* tile largo: jogos */
    tile_create(content, "J", "jogos", RATIMOS_SCREEN_W - 20, 70, ratimos_jogos_show);

    /* grid 2x2: musica / album / cartas / config */
    lv_obj_t * grid = lv_obj_create(content);
    lv_obj_remove_style_all(grid);
    lv_obj_set_width(grid, RATIMOS_SCREEN_W - 20);
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    lv_coord_t tile_w = (RATIMOS_SCREEN_W - 20 - 10) / 2;
    tile_create(grid, "M", "musica", tile_w, 80, ratimos_musica_show);
    tile_create(grid, "A", "album", tile_w, 80, ratimos_album_show);
    tile_create(grid, "C", "cartas", tile_w, 80, ratimos_cartas_show);
    tile_create(grid, "#", "config", tile_w, 80, ratimos_config_show);

    ratimos_bottombar_create(scr, "local", NULL, "sem wifi/audio");

    return scr;
}

void ratimos_home_screen_show(lv_event_t * e)
{
    (void) e;
    if (!s_home_screen) {
        s_home_screen = build_home_screen();
    }
    lv_screen_load(s_home_screen);
}
