/*
 * Motor do cruzadinha (JOGOS-04) -- ver cruzadinha_engine.h.
 */
#include <ctype.h>
#include <string.h>

#include "cruzadinha_engine.h"

static bool word_covers(const ratimos_cruzadinha_word_t * w, uint8_t row, uint8_t col)
{
    if (w->is_across) {
        return row == w->row && col >= w->col && col < (uint8_t) (w->col + w->length);
    }
    return col == w->col && row >= w->row && row < (uint8_t) (w->row + w->length);
}

void ratimos_cruzadinha_number_grid(const ratimos_cruzadinha_puzzle_t * pz, ratimos_cruzadinha_grid_t * out)
{
    if (!pz || !out) {
        return;
    }

    memset(out, 0, sizeof(*out));

    for (uint8_t i = 0; i < pz->word_count && i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        const ratimos_cruzadinha_word_t * w = &pz->words[i];
        for (uint8_t k = 0; k < w->length; k++) {
            uint8_t r = w->is_across ? w->row : (uint8_t) (w->row + k);
            uint8_t c = w->is_across ? (uint8_t) (w->col + k) : w->col;
            if (r >= RATIMOS_CRUZADINHA_MAX_DIM || c >= RATIMOS_CRUZADINHA_MAX_DIM) {
                continue; /* geometria fora dos limites -- descartada, nunca escrita */
            }
            out->is_cell[r][c] = 1;
            out->letters[r][c] = w->answer[k];
        }
    }

    uint8_t next_num = 1;
    for (uint8_t r = 0; r < pz->grid_h && r < RATIMOS_CRUZADINHA_MAX_DIM; r++) {
        for (uint8_t c = 0; c < pz->grid_w && c < RATIMOS_CRUZADINHA_MAX_DIM; c++) {
            if (!out->is_cell[r][c]) {
                continue;
            }
            bool starts_across = (c == 0 || !out->is_cell[r][c - 1]) &&
                                  (c + 1 < RATIMOS_CRUZADINHA_MAX_DIM && out->is_cell[r][c + 1]);
            bool starts_down = (r == 0 || !out->is_cell[r - 1][c]) &&
                                (r + 1 < RATIMOS_CRUZADINHA_MAX_DIM && out->is_cell[r + 1][c]);
            if (starts_across || starts_down) {
                out->numbers[r][c] = next_num;
                next_num++;
            }
        }
    }
}

bool ratimos_cruzadinha_entry_at(const ratimos_cruzadinha_puzzle_t * pz, uint8_t row, uint8_t col,
                                  uint8_t * out_across, uint8_t * out_down)
{
    if (!pz || !out_across || !out_down) {
        return false;
    }
    if (row >= pz->grid_h || col >= pz->grid_w) {
        return false;
    }

    *out_across = RATIMOS_CRUZADINHA_NO_ENTRY;
    *out_down = RATIMOS_CRUZADINHA_NO_ENTRY;

    for (uint8_t i = 0; i < pz->word_count && i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        const ratimos_cruzadinha_word_t * w = &pz->words[i];
        if (!word_covers(w, row, col)) {
            continue;
        }
        if (w->is_across) {
            *out_across = i;
        } else {
            *out_down = i;
        }
    }

    return true;
}

bool ratimos_cruzadinha_set_letter(const ratimos_cruzadinha_puzzle_t * pz, ratimos_cruzadinha_state_t * st,
                                    uint8_t row, uint8_t col, char letter)
{
    if (!pz || !st) {
        return false;
    }
    if (row >= RATIMOS_CRUZADINHA_MAX_DIM || col >= RATIMOS_CRUZADINHA_MAX_DIM) {
        return false;
    }

    uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
    uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
    if (!ratimos_cruzadinha_entry_at(pz, row, col, &across, &down)) {
        return false; /* fora dos limites do proprio puzzle */
    }
    if (across == RATIMOS_CRUZADINHA_NO_ENTRY && down == RATIMOS_CRUZADINHA_NO_ENTRY) {
        return false; /* nao e celula de letra */
    }

    char normalized;
    if (letter == ' ' || letter == '\0') {
        normalized = '\0';
    } else if (isalpha((unsigned char) letter)) {
        normalized = (char) toupper((unsigned char) letter);
    } else {
        return false; /* caractere invalido -- nada mutado */
    }

    st->entered[row][col] = normalized;
    return true;
}

bool ratimos_cruzadinha_entry_is_solved(const ratimos_cruzadinha_puzzle_t * pz,
                                         const ratimos_cruzadinha_state_t * st, uint8_t entry_index)
{
    if (!pz || !st) {
        return false;
    }
    if (entry_index >= pz->word_count || entry_index >= RATIMOS_CRUZADINHA_MAX_WORDS) {
        return false;
    }

    const ratimos_cruzadinha_word_t * w = &pz->words[entry_index];
    for (uint8_t k = 0; k < w->length; k++) {
        uint8_t r = w->is_across ? w->row : (uint8_t) (w->row + k);
        uint8_t c = w->is_across ? (uint8_t) (w->col + k) : w->col;
        if (r >= RATIMOS_CRUZADINHA_MAX_DIM || c >= RATIMOS_CRUZADINHA_MAX_DIM) {
            return false;
        }
        if (st->entered[r][c] != w->answer[k]) {
            return false;
        }
    }
    return true;
}

bool ratimos_cruzadinha_is_complete(const ratimos_cruzadinha_puzzle_t * pz, const ratimos_cruzadinha_state_t * st)
{
    if (!pz || !st) {
        return false;
    }
    if (pz->word_count == 0) {
        return false; /* nenhum quebra-cabeca carregado nao pode estar "completo" */
    }

    for (uint8_t i = 0; i < pz->word_count && i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        if (!ratimos_cruzadinha_entry_is_solved(pz, st, i)) {
            return false;
        }
    }
    return true;
}

uint8_t ratimos_cruzadinha_next_unsolved(const ratimos_cruzadinha_puzzle_t * pz,
                                          const ratimos_cruzadinha_state_t * st, uint8_t from_entry)
{
    if (!pz || !st || pz->word_count == 0) {
        return RATIMOS_CRUZADINHA_NO_ENTRY;
    }

    uint8_t count = pz->word_count;
    uint8_t start = (from_entry < count) ? (uint8_t) ((from_entry + 1) % count) : 0;

    for (uint8_t i = 0; i < count; i++) {
        uint8_t idx = (uint8_t) ((start + i) % count);
        if (!ratimos_cruzadinha_entry_is_solved(pz, st, idx)) {
            return idx;
        }
    }

    return RATIMOS_CRUZADINHA_NO_ENTRY;
}
