#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Cache-once, like ratimos_cartas_show() (01-01) / ratimos_home_screen_show():
 * games are only ever indexed once, synchronously, during the splash's staged
 * init (D-10), so building this screen's row list once and reusing it forever
 * is both correct and required -- without caching, every visit builds a
 * brand-new, never-freed lv_obj_t screen, which exhausts LVGL's builtin heap
 * after only a handful of visits (same leak already fixed in cartas_app.c
 * during 01-01; deferred here for 01-03 per deferred-items.md).
 */
static lv_obj_t * s_jogos_screen = NULL;

static lv_obj_t * build_jogos_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("jogos", "toque para abrir");

    ratimos_game_t games[4];
    size_t n = ratimos_storage_list_games(games, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhum jogo disponivel", "verifique a instalacao do RatimOS", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "J", games[i].title, "abrir", NULL);
        }
    }

    return shell.screen;
}

void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    if (!s_jogos_screen) {
        s_jogos_screen = build_jogos_screen();
    }
    lv_screen_load(s_jogos_screen);
}
