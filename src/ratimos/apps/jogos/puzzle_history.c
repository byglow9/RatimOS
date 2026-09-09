#include "puzzle_history.h"

#define PICK_MAX_ATTEMPTS 16


static uint32_t next_rand(uint32_t * s)
{
    /* LCG de Numerical Recipes: deterministico, minusculo, suficiente para
     * escolher entre um punhado de quebra-cabecas. */
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

static int history_contains(const ratimos_puzzle_history_t * history, size_t index)
{
    if (!history) {
        return 0;
    }
    for (size_t i = 0; i < RATIMOS_RECENT_PUZZLES_TRACKED; i++) {
        if (history->recent[i] == (uint16_t) (index + 1u)) {
            return 1;
        }
    }
    return 0;
}

size_t ratimos_puzzle_pick(const ratimos_puzzle_history_t * history, size_t pool_count, uint32_t seed)
{
    if (pool_count == 0) {
        return 0;
    }

    uint32_t s = seed;
    size_t candidate = 0;

    for (int attempt = 0; attempt < PICK_MAX_ATTEMPTS; attempt++) {
        candidate = (size_t) ((next_rand(&s) >> 16) % (uint32_t) pool_count);
        if (!history_contains(history, candidate)) {
            return candidate;
        }
    }

    /* Esgotou as tentativas: repete em vez de travar. Documentado no header
     * como limite de escala do banco pequeno. */
    return candidate;
}

void ratimos_puzzle_history_push(ratimos_puzzle_history_t * history, uint16_t index)
{
    if (!history) {
        return;
    }

    history->recent[history->write_pos % RATIMOS_RECENT_PUZZLES_TRACKED] = (uint16_t) (index + 1u);
    history->write_pos = (uint8_t) ((history->write_pos + 1u) % RATIMOS_RECENT_PUZZLES_TRACKED);
}
