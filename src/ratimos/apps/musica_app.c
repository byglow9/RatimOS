#include <stdio.h>
#include "musica_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Construida a cada visita e deletada ao sair (ratimos_screen_load, plano
 * 02.1-15): o ponteiro so' vale enquanto a tela existe -- o LV_EVENT_DELETE
 * zera ele pro proximo ratimos_musica_show() reconstruir.
 */
static lv_obj_t * s_musica_screen = NULL;

static void musica_screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_musica_screen = NULL;
}

static lv_obj_t * build_musica_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("./home/musica", NULL);
    lv_obj_set_style_pad_row(shell.content, RATIMOS_ROW_LIST_GAP, 0);

    ratimos_track_t tracks[4];
    size_t n = ratimos_storage_list_tracks(tracks, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "row_empty", "nenhuma musica ainda", "adicione via SD ou sync", NULL);
    } else {
        char subtitle[32];
        if (n == 1) {
            snprintf(subtitle, sizeof(subtitle), "1 faixa");
        } else {
            snprintf(subtitle, sizeof(subtitle), "%zu faixas", n);
        }
        ratimos_row_create(shell.content, "row_playlist", "playlist", subtitle, NULL);

        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "row_track", tracks[i].title, "tocar", NULL);
        }
    }

    return shell.screen;
}

void ratimos_musica_show(lv_event_t * e)
{
    (void) e;
    if (!s_musica_screen) {
        s_musica_screen = build_musica_screen();
        lv_obj_add_event_cb(s_musica_screen, musica_screen_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    ratimos_screen_load(s_musica_screen);
}
