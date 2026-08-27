#include "cartas_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Cache-once, like ratimos_home_screen_show() (home_screen.c): the letters
 * domain is only ever indexed once, synchronously, during the splash's
 * staged init (D-10 — no live/async content updates in Phase 1), so building
 * this screen's row list once and reusing it forever is both correct (data
 * never changes mid-session) and required — without caching, every visit to
 * "cartas" builds a brand-new, never-freed lv_obj_t screen (no delete-on-
 * navigate-away exists anywhere in this codebase yet), which exhausts
 * LVGL's small builtin heap after only ~3-4 visits (`lv_realloc: couldn't
 * reallocate memory` / `lv_array_resize` assert — reproduced and confirmed
 * during this task's checkpoint review).
 */
static lv_obj_t * s_cartas_screen = NULL;

static lv_obj_t * build_cartas_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("cartas", "toque para abrir");

    ratimos_letter_t letters[4];
    size_t n = ratimos_storage_list_letters(letters, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhuma carta ainda", "chegam aqui quando sincronizadas", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "L", letters[i].title, "abrir", NULL);
        }
    }

    return shell.screen;
}

void ratimos_cartas_show(lv_event_t * e)
{
    (void) e;
    if (!s_cartas_screen) {
        s_cartas_screen = build_cartas_screen();
    }
    lv_screen_load(s_cartas_screen);
}
