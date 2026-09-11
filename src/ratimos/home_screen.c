#include <string.h>

#include "home_screen.h"
#include "theme.h"
#include "status_bar.h"
#include "progression.h"
#include "apps/jogos_app.h"
#include "apps/musica_app.h"
#include "apps/album_app.h"
#include "apps/cartas_app.h"
#include "apps/config_app.h"
#include "apps/castelo_app.h"
#include "../storage/content_api.h"

static lv_obj_t * s_home_screen = NULL;
static lv_obj_t * s_castelo_status_label = NULL;

static lv_obj_t * tile_create(lv_obj_t * parent, const char * icon_id, const char * title,
                               lv_coord_t w, lv_coord_t h, lv_event_cb_t click_cb)
{
    lv_obj_t * tile = ratimos_panel_create(parent);
    lv_obj_set_size(tile, w, h);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(tile, 4, 0);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tile, click_cb, LV_EVENT_CLICKED, NULL);

    ratimos_badge_create(tile, icon_id);

    lv_obj_t * lbl = lv_label_create(tile);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT, 0);

    return tile;
}

/*
 * Mesma tabela de copia por estagio da tela dedicada (castelo_app.c) --
 * sempre celebratoria, nunca de risco/perda (UI-SPEC Copywriting Contract).
 * Duplicada intencionalmente (um label pequeno de uma linha, nao vale a
 * pena extrair um helper compartilhado por duas linhas de codigo).
 */
static void set_castelo_status_text(lv_obj_t * label)
{
    ratimos_progression_state_t state;
    if (!ratimos_storage_get_progression(&state)) {
        memset(&state, 0, sizeof(state));
    }

    if (ratimos_progression_is_complete(state.shared_completions)) {
        lv_label_set_text(label, "castelo completo! parabens");
    } else if (state.shared_completions == 0) {
        lv_label_set_text(label, "o castelo esta apenas comecando");
    } else {
        lv_label_set_text_fmt(label, "dia %d de %d",
                              (int) state.shared_completions,
                              (int) ratimos_progression_target_completions());
    }
}

/* Tile largo (row: selo + coluna titulo/status) abaixo da grade 2x2 --
 * folga vertical ja prevista no UI-SPEC's Screen Budget, nenhum tile
 * existente encolhe ou reflui. */
static lv_obj_t * castelo_tile_create(lv_obj_t * parent)
{
    lv_obj_t * tile = ratimos_panel_create(parent);
    lv_obj_set_size(tile, RATIMOS_SCREEN_W - 20, 100);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tile, 10, 0);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tile, ratimos_castelo_show, LV_EVENT_CLICKED, NULL);

    ratimos_badge_create(tile, "home_castelo");

    lv_obj_t * col = lv_obj_create(tile);
    lv_obj_remove_style_all(col);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 4, 0);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    /* Same hitbox bug class as row_list.c's text_col: lv_obj_remove_style_all()
     * never clears flags, so this plain lv_obj_create() kept LVGL's default
     * LV_OBJ_FLAG_CLICKABLE with no handler, swallowing taps over the
     * title/status text (most of the tile's width) before they reach
     * `tile`'s own click_cb (ratimos_castelo_show) above. */
    lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * title = lv_label_create(col);
    lv_label_set_text(title, "castelo");
    lv_obj_set_style_text_color(title, RATIMOS_COLOR_TEXT, 0);

    s_castelo_status_label = lv_label_create(col);
    lv_obj_set_width(s_castelo_status_label, lv_pct(100));
    lv_label_set_long_mode(s_castelo_status_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_color(s_castelo_status_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    set_castelo_status_text(s_castelo_status_label);

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
    tile_create(content, "home_jogos", "jogos", RATIMOS_SCREEN_W - 20, 70, ratimos_jogos_show);

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
    tile_create(grid, "home_musica", "musica", tile_w, 80, ratimos_musica_show);
    tile_create(grid, "home_album", "album", tile_w, 80, ratimos_album_show);
    tile_create(grid, "home_cartas", "cartas", tile_w, 80, ratimos_cartas_show);
    tile_create(grid, "home_config", "config", tile_w, 80, ratimos_config_show);

    /* tile largo abaixo da grade 2x2: progressao castelo/jardim (PROGRESSAO-01) */
    castelo_tile_create(content);

    ratimos_bottombar_create(scr, "local", NULL, "sem wifi/audio");

    return scr;
}

void ratimos_home_screen_show(lv_event_t * e)
{
    (void) e;
    if (!s_home_screen) {
        s_home_screen = build_home_screen();
    }
    /* Reler o status a cada visita (nao so na primeira construcao) -- ao
     * voltar de um jogo recem-vencido, o dia atualizado deve aparecer sem
     * precisar reconstruir a tela inteira. */
    if (s_castelo_status_label) {
        set_castelo_status_text(s_castelo_status_label);
    }
    lv_screen_load(s_home_screen);
}
