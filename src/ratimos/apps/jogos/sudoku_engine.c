/*
 * Motor do sudoku (JOGOS-01) -- validacao, solver contador, gerador com
 * garantia de solucao unica, faixas de dificuldade por contagem de pistas,
 * modo diario determinista.
 *
 * IMPORTANTE (Pitfall 4 do RESEARCH): a contagem de pistas e um PROXY de
 * dificuldade, nao uma graduacao validada por tecnica de resolucao -- um
 * quebra-cabeca de 30 pistas soluvel por eliminacao pura pode parecer mais
 * facil que um de 35 pistas que exige tecnica avancada. Para o escopo v1
 * deste presente pessoal isso e uma simplificacao honesta e aceita; os
 * rotulos de dificuldade no UI-SPEC nunca devem ser lidos como "grau de
 * dificuldade cientificamente medido".
 */
#include "sudoku_engine.h"

#include <string.h>

#define SUDOKU_SIZE 9

static bool find_empty_cell(const uint8_t board[9][9], int * out_row, int * out_col)
{
    for (int r = 0; r < SUDOKU_SIZE; r++) {
        for (int c = 0; c < SUDOKU_SIZE; c++) {
            if (board[r][c] == 0) {
                *out_row = r;
                *out_col = c;
                return true;
            }
        }
    }
    return false;
}

bool ratimos_sudoku_is_valid_placement(const uint8_t board[9][9], int row, int col, uint8_t digit)
{
    if (row < 0 || row >= SUDOKU_SIZE || col < 0 || col >= SUDOKU_SIZE) {
        return false;
    }
    if (digit < 1 || digit > 9) {
        return false;
    }

    for (int c = 0; c < SUDOKU_SIZE; c++) {
        if (c != col && board[row][c] == digit) {
            return false;
        }
    }
    for (int r = 0; r < SUDOKU_SIZE; r++) {
        if (r != row && board[r][col] == digit) {
            return false;
        }
    }

    int box_row = (row / 3) * 3;
    int box_col = (col / 3) * 3;
    for (int r = box_row; r < box_row + 3; r++) {
        for (int c = box_col; c < box_col + 3; c++) {
            if ((r != row || c != col) && board[r][c] == digit) {
                return false;
            }
        }
    }

    return true;
}

/* Solver contador padrao (RESEARCH "Sudoku uniqueness check"): acha a
 * primeira celula vazia, tenta digitos 1-9 validos, recursa com um limite
 * decrescente, e para assim que o total acumulado atinge `limit`. */
int ratimos_sudoku_count_solutions(uint8_t board[9][9], int limit)
{
    int row, col;
    if (!find_empty_cell(board, &row, &col)) {
        return 1;
    }

    int total = 0;
    for (uint8_t digit = 1; digit <= 9 && total < limit; digit++) {
        if (ratimos_sudoku_is_valid_placement(board, row, col, digit)) {
            board[row][col] = digit;
            total += ratimos_sudoku_count_solutions(board, limit - total);
            board[row][col] = 0;
        }
    }
    return total;
}

/* PRNG local (xorshift32) -- gerar e embaralhar NUNCA usam o gerador
 * pseudoaleatorio global da libc, para que os testes sejam deterministas e
 * independentes de ordem de chamada. Semente 0 travaria o xorshift em 0 para
 * sempre; forcada para 1. */
static uint32_t sudoku_xorshift32(uint32_t * state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static bool fill_complete_grid(uint8_t board[9][9], uint32_t * rng)
{
    int row, col;
    if (!find_empty_cell(board, &row, &col)) {
        return true;
    }

    uint8_t digits[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    for (int i = 8; i > 0; i--) {
        int j = (int) (sudoku_xorshift32(rng) % (uint32_t) (i + 1));
        uint8_t tmp = digits[i];
        digits[i] = digits[j];
        digits[j] = tmp;
    }

    for (int i = 0; i < 9; i++) {
        uint8_t digit = digits[i];
        if (ratimos_sudoku_is_valid_placement(board, row, col, digit)) {
            board[row][col] = digit;
            if (fill_complete_grid(board, rng)) {
                return true;
            }
            board[row][col] = 0;
        }
    }
    return false;
}

/* Bandas de pistas: facil 40-45, medio 30-35, dificil 22-27; diario usa a
 * banda do medio (mesma faixa de trabalho do solver, so o dia muda). */
static void clue_band(ratimos_sudoku_mode_t mode, int * out_min, int * out_max)
{
    switch (mode) {
        case RATIMOS_SUDOKU_FACIL:
            *out_min = 40;
            *out_max = 45;
            break;
        case RATIMOS_SUDOKU_DIFICIL:
            *out_min = 22;
            *out_max = 27;
            break;
        case RATIMOS_SUDOKU_MEDIO:
        case RATIMOS_SUDOKU_DIARIO:
        default:
            *out_min = 30;
            *out_max = 35;
            break;
    }
}

bool ratimos_sudoku_generate(ratimos_sudoku_state_t * out, ratimos_sudoku_mode_t mode,
                              uint32_t seed, int max_attempts)
{
    if (!out || mode < 0 || mode >= RATIMOS_SUDOKU_MODE_COUNT || max_attempts <= 0) {
        return false;
    }

    int min_clues, max_clues;
    clue_band(mode, &min_clues, &max_clues);

    uint32_t rng = seed != 0 ? seed : 1u;

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        uint8_t board[9][9];
        memset(board, 0, sizeof(board));
        if (!fill_complete_grid(board, &rng)) {
            continue;
        }

        uint8_t solution[9][9];
        memcpy(solution, board, sizeof(board));

        int target = min_clues + (int) (sudoku_xorshift32(&rng) % (uint32_t) (max_clues - min_clues + 1));

        /* Ordem embaralhada das 81 celulas -- remocao de pistas segue essa
         * ordem, uma celula por vez, nunca em bloco. */
        uint8_t order[81];
        for (int i = 0; i < 81; i++) {
            order[i] = (uint8_t) i;
        }
        for (int i = 80; i > 0; i--) {
            int j = (int) (sudoku_xorshift32(&rng) % (uint32_t) (i + 1));
            uint8_t tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }

        int clue_count = 81;
        for (int i = 0; i < 81 && clue_count > target; i++) {
            int row = order[i] / 9;
            int col = order[i] % 9;
            if (board[row][col] == 0) {
                continue;
            }

            uint8_t backup = board[row][col];
            board[row][col] = 0;

            uint8_t probe[9][9];
            memcpy(probe, board, sizeof(board));
            if (ratimos_sudoku_count_solutions(probe, 2) == 1) {
                clue_count--;
            } else {
                /* Remover essa pista quebrou a unicidade -- devolve. */
                board[row][col] = backup;
            }
        }

        if (clue_count < min_clues || clue_count > max_clues) {
            continue;
        }

        /* Confirmacao final defensiva antes de entregar ao chamador. */
        uint8_t verify[9][9];
        memcpy(verify, board, sizeof(board));
        if (ratimos_sudoku_count_solutions(verify, 2) != 1) {
            continue;
        }

        memset(out, 0, sizeof(*out));
        memcpy(out->given, board, sizeof(board));
        memcpy(out->solution, solution, sizeof(solution));
        out->mode = mode;
        return true;
    }

    return false;
}

bool ratimos_sudoku_generate_daily(ratimos_sudoku_state_t * out, uint32_t day_seed)
{
    return ratimos_sudoku_generate(out, RATIMOS_SUDOKU_DIARIO, day_seed, 64);
}

bool ratimos_sudoku_set_cell(ratimos_sudoku_state_t * st, int row, int col, uint8_t digit)
{
    if (!st || row < 0 || row >= SUDOKU_SIZE || col < 0 || col >= SUDOKU_SIZE || digit > 9) {
        return false;
    }
    if (st->given[row][col] != 0) {
        return false;
    }
    st->filled[row][col] = digit;
    return true;
}

static void combined_board(const ratimos_sudoku_state_t * st, uint8_t out[9][9])
{
    for (int r = 0; r < SUDOKU_SIZE; r++) {
        for (int c = 0; c < SUDOKU_SIZE; c++) {
            out[r][c] = st->given[r][c] != 0 ? st->given[r][c] : st->filled[r][c];
        }
    }
}

bool ratimos_sudoku_cell_conflicts(const ratimos_sudoku_state_t * st, int row, int col)
{
    if (!st || row < 0 || row >= SUDOKU_SIZE || col < 0 || col >= SUDOKU_SIZE) {
        return false;
    }

    uint8_t digit = st->given[row][col] != 0 ? st->given[row][col] : st->filled[row][col];
    if (digit == 0) {
        return false;
    }

    uint8_t board[9][9];
    combined_board(st, board);
    return !ratimos_sudoku_is_valid_placement(board, row, col, digit);
}

bool ratimos_sudoku_is_solved(const ratimos_sudoku_state_t * st)
{
    if (!st) {
        return false;
    }

    for (int r = 0; r < SUDOKU_SIZE; r++) {
        for (int c = 0; c < SUDOKU_SIZE; c++) {
            uint8_t digit = st->given[r][c] != 0 ? st->given[r][c] : st->filled[r][c];
            if (digit == 0) {
                return false;
            }
            if (ratimos_sudoku_cell_conflicts(st, r, c)) {
                return false;
            }
        }
    }
    return true;
}
