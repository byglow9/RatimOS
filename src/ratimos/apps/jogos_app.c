#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"

void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("jogos", "toque para abrir");

    /* TODO (Fase 3): rows viram cliques reais que abrem cada jogo */
    ratimos_row_create(shell.content, "S", "sudoku", "abrir", NULL);
    ratimos_row_create(shell.content, "C", "car jam", "abrir", NULL);
    ratimos_row_create(shell.content, "P", "paciencia", "abrir", NULL);

    lv_screen_load(shell.screen);
}
