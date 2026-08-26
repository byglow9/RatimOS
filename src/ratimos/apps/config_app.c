#include "config_app.h"
#include "../app_shell.h"
#include "../row_list.h"

void ratimos_config_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("config", "fase 0 local");

    /* TODO: cada linha vira uma tela/toggle real nas Fases 2-5 */
    ratimos_row_create(shell.content, "W", "wifi", "desligado nesta fase", NULL);
    ratimos_row_create(shell.content, "P", "perfil", "nao iniciado", NULL);
    ratimos_row_create(shell.content, "A", "audio", "nao iniciado", NULL);
    ratimos_row_create(shell.content, "R", "render", "direto / sem param", NULL);

    lv_screen_load(shell.screen);
}
