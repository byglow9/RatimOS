/*
 * Storage/Content API — dominio `games` (jogos).
 *
 * NAO e file-backed (D-09/RESEARCH.md Task 1 action): jogos vem compilados
 * no firmware, entao nao ha nada para indexar do disco. A lista e um array
 * const compilado; index_games() e um no-op documentado.
 */
#include <stdio.h>
#include <string.h>
#include "content_api.h"

#define RATIMOS_GAME_COUNT 3

static const char * const s_game_titles[RATIMOS_GAME_COUNT] = {
    "sudoku",
    "car jam",
    "paciencia",
};

void ratimos_storage_index_games(void)
{
    /* No-op: a lista de jogos ja esta compilada em s_game_titles, nao ha
     * I/O de disco para fazer nesta fase. */
}

size_t ratimos_storage_list_games(ratimos_game_t * out, size_t max_count)
{
    size_t n = RATIMOS_GAME_COUNT < max_count ? RATIMOS_GAME_COUNT : max_count;

    for (size_t i = 0; i < n; i++) {
        snprintf(out[i].id, sizeof(out[i].id), "game%zu", i + 1);
        snprintf(out[i].title, sizeof(out[i].title), "%s", s_game_titles[i]);
    }

    return n;
}
