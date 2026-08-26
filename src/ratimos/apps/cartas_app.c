#include "cartas_app.h"
#include "../app_shell.h"
#include "../row_list.h"

void ratimos_cartas_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("cartas", "toque pra ler");

    /* TODO (Fase 5): puxar cartas novas do backend via sync remoto */
    ratimos_row_create(shell.content, "!", "nenhuma carta ainda", "chegam aqui quando sincronizadas", NULL);

    lv_screen_load(shell.screen);
}
