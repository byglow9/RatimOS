#include <stdio.h>
#include "musica_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Cache-once (same rationale as jogos_app.c / cartas_app.c / 01-01) -- avoids
 * rebuilding a new lv_obj_t screen on every visit, which exhausts LVGL's
 * builtin heap after a handful of visits.
 */
static lv_obj_t * s_musica_screen = NULL;

static lv_obj_t * build_musica_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("musica", "toque para abrir");

    ratimos_track_t tracks[4];
    size_t n = ratimos_storage_list_tracks(tracks, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhuma musica ainda", "adicione via SD ou sync", NULL);
    } else {
        char subtitle[32];
        if (n == 1) {
            snprintf(subtitle, sizeof(subtitle), "1 faixa");
        } else {
            snprintf(subtitle, sizeof(subtitle), "%zu faixas", n);
        }
        ratimos_row_create(shell.content, "P", "playlist", subtitle, NULL);

        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "T", tracks[i].title, "tocar", NULL);
        }
    }

    return shell.screen;
}

void ratimos_musica_show(lv_event_t * e)
{
    (void) e;
    if (!s_musica_screen) {
        s_musica_screen = build_musica_screen();
    }
    lv_screen_load(s_musica_screen);
}
