#include "cartas_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Construida a cada visita e deletada ao sair (ratimos_screen_load, plano
 * 02.1-15): o ponteiro so' vale enquanto a tela existe -- o LV_EVENT_DELETE
 * zera ele pro proximo ratimos_cartas_show() reconstruir.
 */
static lv_obj_t * s_cartas_screen = NULL;

static void cartas_screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_cartas_screen = NULL;
}

static lv_obj_t * build_cartas_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("./home/cartas", NULL);
    lv_obj_set_style_pad_row(shell.content, RATIMOS_ROW_LIST_GAP, 0);

    ratimos_letter_t letters[4];
    size_t n = ratimos_storage_list_letters(letters, 4);

    if (n == 0) {
        ratimos_row_create(shell.content, "row_empty", "nenhuma carta ainda", "chegam aqui quando sincronizadas", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "row_carta", letters[i].title, "abrir", NULL);
        }
    }

    return shell.screen;
}

void ratimos_cartas_show(lv_event_t * e)
{
    (void) e;
    if (!s_cartas_screen) {
        s_cartas_screen = build_cartas_screen();
        lv_obj_add_event_cb(s_cartas_screen, cartas_screen_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    ratimos_screen_load(s_cartas_screen);
}
