#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"
#include "jogos/conexo.h"
#include "jogos/cruzadinha.h"
#include "jogos/paciencia.h"
#include "jogos/sudoku.h"
#include "jogos/termo.h"

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

    ratimos_game_t games[RATIMOS_GAME_COUNT];
    size_t n = ratimos_storage_list_games(games, RATIMOS_GAME_COUNT);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhum jogo disponivel", "verifique a instalacao do RatimOS", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            /* Todos os 5 jogos (02.1-01/02.1-04/02.1-05/02.1-06/02.1-07) ja
             * estao prontos -- cada linha do launcher abre sua tela real. */
            lv_event_cb_t click_cb = NULL;
            if (i == (size_t) RATIMOS_GAME_SUDOKU) {
                click_cb = ratimos_sudoku_show;
            } else if (i == (size_t) RATIMOS_GAME_PACIENCIA) {
                click_cb = ratimos_paciencia_show;
            } else if (i == (size_t) RATIMOS_GAME_TERMO) {
                click_cb = ratimos_termo_show;
            } else if (i == (size_t) RATIMOS_GAME_CRUZADINHA) {
                click_cb = ratimos_cruzadinha_show;
            } else if (i == (size_t) RATIMOS_GAME_CONEXO) {
                click_cb = ratimos_conexo_show;
            }
            ratimos_row_create(shell.content, "J", games[i].title, "abrir", click_cb);
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
