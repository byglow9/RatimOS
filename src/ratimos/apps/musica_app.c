#include "musica_app.h"
#include "../app_shell.h"
#include "../row_list.h"

void ratimos_musica_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("musica", "toque na musica");

    /* TODO (Fase 3): listar faixas reais do SD via codec ES8311 */
    ratimos_row_create(shell.content, "P", "playlist", "0 faixas", NULL);
    ratimos_row_create(shell.content, "!", "nenhuma musica ainda", "adicione via SD ou sync", NULL);

    lv_screen_load(shell.screen);
}
