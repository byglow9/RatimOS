#include "cartas_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

void ratimos_cartas_show(lv_event_t * e)
{
    (void) e;
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

    lv_screen_load(shell.screen);
}
