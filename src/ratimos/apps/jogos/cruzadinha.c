/*
 * Cruzadinha (JOGOS-04) -- tela.
 *
 * Persistencia (JOGOS-02): toda entrada de letra, troca de orientacao e
 * troca de selecao grava na hora via ratimos_storage_save_game_state() --
 * mesma disciplina de sudoku.c/termo.c/conexo.c. Este arquivo NAO abre
 * arquivo nenhum: todo byte que chega ao disco passa pela Storage/Content
 * API (D-10).
 *
 * Grid de tamanho variavel (9x9 na maioria dos quebra-cabecas, 11x11 quando
 * a lista de palavras genuinamente precisa): as celulas sao construidas UMA
 * vez no tamanho maximo (RATIMOS_CRUZADINHA_MAX_DIM x MAX_DIM) e cada visita
 * so mostra/esconde/redimensiona -- nunca reconstroi (mesma disciplina de
 * cache-once de todo screen deste projeto).
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cruzadinha.h"
#include "cruzadinha_engine.h"

#include "../../app_shell.h"
#include "../../theme.h"
#include "../../fonts/ratimos_fonts.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

#define CRUZ_MAX_DIM        RATIMOS_CRUZADINHA_MAX_DIM
#define CRUZ_BOARD_BUDGET_PX 300 /* largura util do conteudo (UI-SPEC screen budget) */
#define CRUZ_CLUE_STRIP_H   40
#define CRUZ_PILL_H         28

/* Teclado compartilhado A-Z + apagar -- mesma convencao visual do teclado do
 * termo (lv_buttonmatrix neutro, sem cor por tecla). */
static const char * const CRUZ_KEYBOARD_MAP[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "z", "x", "c", "v", "b", "n", "m", "apagar", NULL
};

/* ------------------------------------------------------------------------
 * Tela (LVGL) -- cache-once.
 * ------------------------------------------------------------------------ */

static lv_obj_t * s_cruzadinha_screen = NULL;

static lv_obj_t * s_unavailable_label = NULL;
static lv_obj_t * s_error_label = NULL;
static lv_obj_t * s_board = NULL;
static lv_obj_t * s_cell[CRUZ_MAX_DIM][CRUZ_MAX_DIM];
static lv_obj_t * s_cell_letter_label[CRUZ_MAX_DIM][CRUZ_MAX_DIM];
static lv_obj_t * s_cell_number_label[CRUZ_MAX_DIM][CRUZ_MAX_DIM];
static lv_obj_t * s_clue_strip = NULL;
static lv_obj_t * s_clue_strip_label = NULL;
static lv_obj_t * s_next_word_btn = NULL;
static lv_obj_t * s_keyboard = NULL;
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_confirm_overlay = NULL;
static lv_obj_t * s_clue_list_overlay = NULL;
static lv_obj_t * s_clue_list_across_header = NULL;
static lv_obj_t * s_clue_list_down_header = NULL;
static lv_obj_t * s_clue_list_across_row[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_down_row[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_across_label[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_down_label[RATIMOS_CRUZADINHA_MAX_WORDS];

static ratimos_cruzadinha_state_t s_state;
static ratimos_cruzadinha_puzzle_t s_puzzle;
static ratimos_cruzadinha_grid_t s_grid;
static bool s_puzzle_ready = false;

static void render_all(void);
static void persist_state(void);
static void start_new_puzzle(void);

/* ------------------------------------------------------------------------
 * Helpers puros de layout/estado -- sem efeito colateral em LVGL.
 * ------------------------------------------------------------------------ */

static lv_coord_t cell_px_for_dim(uint8_t dim)
{
    if (dim == 0) {
        return CRUZ_BOARD_BUDGET_PX;
    }
    return (lv_coord_t) (CRUZ_BOARD_BUDGET_PX / dim);
}

static void select_cell(uint8_t row, uint8_t col)
{
    uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
    uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
    if (!ratimos_cruzadinha_entry_at(&s_puzzle, row, col, &across, &down)) {
        return; /* fora dos limites do puzzle */
    }
    if (across == RATIMOS_CRUZADINHA_NO_ENTRY && down == RATIMOS_CRUZADINHA_NO_ENTRY) {
        return; /* nao e celula de letra */
    }

    s_state.cursor_row = row;
    s_state.cursor_col = col;

    /* Preserva a orientacao ativa quando a celula nova ainda tem uma
     * entrada naquela direcao; senao cai para a unica direcao disponivel. */
    if (s_state.active_is_across && across != RATIMOS_CRUZADINHA_NO_ENTRY) {
        s_state.active_entry = across;
        s_state.active_is_across = 1;
    } else if (!s_state.active_is_across && down != RATIMOS_CRUZADINHA_NO_ENTRY) {
        s_state.active_entry = down;
        s_state.active_is_across = 0;
    } else if (across != RATIMOS_CRUZADINHA_NO_ENTRY) {
        s_state.active_entry = across;
        s_state.active_is_across = 1;
    } else {
        s_state.active_entry = down;
        s_state.active_is_across = 0;
    }
}

static void jump_to_entry(uint8_t entry_index)
{
    if (entry_index >= s_puzzle.word_count) {
        return;
    }
    const ratimos_cruzadinha_word_t * w = &s_puzzle.words[entry_index];
    s_state.active_entry = entry_index;
    s_state.active_is_across = w->is_across ? 1 : 0;
    s_state.cursor_row = w->row;
    s_state.cursor_col = w->col;
}

/* Avanca o cursor uma celula dentro da entrada ativa (sem passar do fim).
 * Usado depois de uma letra digitada com sucesso. */
static void advance_cursor(void)
{
    const ratimos_cruzadinha_word_t * w = &s_puzzle.words[s_state.active_entry];
    uint8_t offset = w->is_across ? (uint8_t) (s_state.cursor_col - w->col)
                                   : (uint8_t) (s_state.cursor_row - w->row);
    if (offset + 1 >= w->length) {
        return; /* ja esta na ultima celula da entrada */
    }
    if (w->is_across) {
        s_state.cursor_col++;
    } else {
        s_state.cursor_row++;
    }
}

/* Recua o cursor uma celula dentro da entrada ativa (sem passar do inicio).
 * Usado por "apagar". */
static void retreat_cursor(void)
{
    const ratimos_cruzadinha_word_t * w = &s_puzzle.words[s_state.active_entry];
    uint8_t offset = w->is_across ? (uint8_t) (s_state.cursor_col - w->col)
                                   : (uint8_t) (s_state.cursor_row - w->row);
    if (offset == 0) {
        return; /* ja esta na primeira celula da entrada */
    }
    if (w->is_across) {
        s_state.cursor_col--;
    } else {
        s_state.cursor_row--;
    }
}

static bool cell_is_in_active_entry(uint8_t row, uint8_t col)
{
    if (s_state.active_entry >= s_puzzle.word_count) {
        return false;
    }
    const ratimos_cruzadinha_word_t * w = &s_puzzle.words[s_state.active_entry];
    if (w->is_across) {
        return row == w->row && col >= w->col && col < (uint8_t) (w->col + w->length);
    }
    return col == w->col && row >= w->row && row < (uint8_t) (w->row + w->length);
}

/* ------------------------------------------------------------------------
 * Persistencia + carregamento.
 * ------------------------------------------------------------------------ */

static void persist_state(void)
{
    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &s_state, sizeof(s_state));
    blob.used = sizeof(s_state);

    ratimos_storage_save_game_state(RATIMOS_GAME_CRUZADINHA, &blob);
}

/* Sorteia o proximo quebra-cabeca evitando o historico recente (D-11),
 * preservando o historico atraves do reset (mesmo padrao de conexo.c). */
static void start_new_puzzle(void)
{
    size_t index = ratimos_puzzle_pick(&s_state.history, ratimos_cruzadinha_puzzle_count(),
                                       ratimos_daily_seed(RATIMOS_GAME_CRUZADINHA));

    ratimos_puzzle_history_t history = s_state.history;
    memset(&s_state, 0, sizeof(s_state));
    s_state.history = history;
    s_state.puzzle_index = (uint16_t) index;

    ratimos_cruzadinha_get_puzzle(index, &s_puzzle);
    ratimos_cruzadinha_number_grid(&s_puzzle, &s_grid);
    ratimos_puzzle_history_push(&s_state.history, (uint16_t) index);

    if (s_puzzle.word_count > 0) {
        jump_to_entry(0);
    }

    persist_state();
}

/* Decide o estado inicial: restaura um save valido, comeca um quebra-cabeca
 * novo quando nao ha save, ou comeca limpo COM aviso quando o save esta
 * corrompido. Mitigacao T-02.1-02: puzzle_index e re-resolvido contra o
 * banco (nunca confiado sozinho), e cursor/entrada ativa sao checados
 * contra as dimensoes do proprio puzzle antes de qualquer uso. */
static bool load_or_start_state(void)
{
    ratimos_game_state_t blob;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CRUZADINHA, &blob);

    if (status == RATIMOS_GAME_STATE_OK && blob.used == sizeof(s_state)) {
        ratimos_cruzadinha_state_t restored;
        memcpy(&restored, blob.bytes, sizeof(restored));

        bool valid = ratimos_cruzadinha_get_puzzle((size_t) restored.puzzle_index, &s_puzzle) &&
                     restored.active_entry < s_puzzle.word_count &&
                     restored.cursor_row < s_puzzle.grid_h &&
                     restored.cursor_col < s_puzzle.grid_w &&
                     restored.active_is_across <= 1 &&
                     restored.complete <= 1 &&
                     restored.daily_win_recorded <= 1;

        if (valid) {
            s_state = restored;
            ratimos_cruzadinha_number_grid(&s_puzzle, &s_grid);
            return false; /* restaurado, sem aviso de erro */
        }
        status = RATIMOS_GAME_STATE_INVALID;
    } else if (status == RATIMOS_GAME_STATE_OK) {
        status = RATIMOS_GAME_STATE_INVALID; /* tamanho do blob nao bate com esta versao */
    }

    start_new_puzzle();
    return status == RATIMOS_GAME_STATE_INVALID;
}

/* ------------------------------------------------------------------------
 * Renderizacao.
 * ------------------------------------------------------------------------ */

static void render_grid(void)
{
    lv_coord_t cell_px = cell_px_for_dim(s_puzzle.grid_w);
    lv_coord_t board_px = (lv_coord_t) (cell_px * s_puzzle.grid_w);
    lv_obj_set_size(s_board, board_px, board_px);

    for (uint8_t r = 0; r < CRUZ_MAX_DIM; r++) {
        for (uint8_t c = 0; c < CRUZ_MAX_DIM; c++) {
            lv_obj_t * cell = s_cell[r][c];

            if (r >= s_puzzle.grid_h || c >= s_puzzle.grid_w) {
                lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(cell, cell_px, cell_px);
            lv_obj_set_pos(cell, (lv_coord_t) (c * cell_px), (lv_coord_t) (r * cell_px));

            if (!s_grid.is_cell[r][c]) {
                /* Celula fora do desenho: bloco solido sem borda (UI-SPEC). */
                lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_BG, 0);
                lv_obj_set_style_border_width(cell, 0, 0);
                lv_label_set_text(s_cell_letter_label[r][c], "");
                lv_label_set_text(s_cell_number_label[r][c], "");
                continue;
            }

            lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);

            bool is_selected = (r == s_state.cursor_row && c == s_state.cursor_col);
            bool in_active_entry = cell_is_in_active_entry(r, c);

            if (is_selected) {
                lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_border_color(cell, RATIMOS_COLOR_ACCENT, 0);
                lv_obj_set_style_border_width(cell, 2, 0);
            } else if (in_active_entry) {
                lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_border_color(cell, RATIMOS_COLOR_PANEL, 0);
                lv_obj_set_style_border_width(cell, 1, 0);
            } else {
                lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_PANEL, 0);
                lv_obj_set_style_border_color(cell, RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_border_width(cell, 1, 0);
            }

            char letter_buf[2] = { 0, 0 };
            if (s_state.entered[r][c] != 0) {
                letter_buf[0] = s_state.entered[r][c];
            }
            lv_label_set_text(s_cell_letter_label[r][c], letter_buf);
            lv_obj_set_style_text_color(s_cell_letter_label[r][c], RATIMOS_COLOR_TEXT, 0);

            if (s_grid.numbers[r][c] != 0) {
                lv_label_set_text_fmt(s_cell_number_label[r][c], "%u", (unsigned) s_grid.numbers[r][c]);
            } else {
                lv_label_set_text(s_cell_number_label[r][c], "");
            }
        }
    }
}

static void render_clue_strip(void)
{
    if (s_state.active_entry >= s_puzzle.word_count) {
        lv_label_set_text(s_clue_strip_label, "");
        return;
    }
    const ratimos_cruzadinha_word_t * w = &s_puzzle.words[s_state.active_entry];
    char buf[160];
    snprintf(buf, sizeof(buf), "%u %s: %s", (unsigned) w->clue_number,
             w->is_across ? "horizontal" : "vertical", w->clue_text);
    lv_label_set_text(s_clue_strip_label, buf);
}

static void render_clue_list(void)
{
    uint8_t across_count = 0;
    uint8_t down_count = 0;

    for (uint8_t i = 0; i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        lv_obj_add_flag(s_clue_list_across_row[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_clue_list_down_row[i], LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t i = 0; i < s_puzzle.word_count; i++) {
        const ratimos_cruzadinha_word_t * w = &s_puzzle.words[i];
        char buf[160];
        snprintf(buf, sizeof(buf), "%u: %s", (unsigned) w->clue_number, w->clue_text);

        if (w->is_across) {
            lv_label_set_text(s_clue_list_across_label[across_count], buf);
            lv_obj_set_user_data(s_clue_list_across_row[across_count], (void *) (uintptr_t) i);
            lv_obj_clear_flag(s_clue_list_across_row[across_count], LV_OBJ_FLAG_HIDDEN);
            across_count++;
        } else {
            lv_label_set_text(s_clue_list_down_label[down_count], buf);
            lv_obj_set_user_data(s_clue_list_down_row[down_count], (void *) (uintptr_t) i);
            lv_obj_clear_flag(s_clue_list_down_row[down_count], LV_OBJ_FLAG_HIDDEN);
            down_count++;
        }
    }
}

static void render_banner(void)
{
    if (!s_state.complete) {
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(s_banner_label, "cruzadinha completa!");
    lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);

    if (s_state.daily_win_recorded) {
        lv_obj_clear_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void render_all(void)
{
    if (!s_puzzle_ready) {
        return;
    }
    render_grid();
    render_clue_strip();
    render_clue_list();
    render_banner();
}

/* ------------------------------------------------------------------------
 * Callbacks.
 * ------------------------------------------------------------------------ */

static void check_completion(void)
{
    if (ratimos_cruzadinha_is_complete(&s_puzzle, &s_state)) {
        s_state.complete = 1;
        /* PROGRESSAO-01: credita o castelo EXATAMENTE uma vez -- o guard
         * `daily_win_recorded` fica dentro do proprio estado persistido. */
        if (!s_state.daily_win_recorded) {
            if (ratimos_storage_record_daily_win(RATIMOS_GAME_CRUZADINHA)) {
                s_state.daily_win_recorded = 1;
            }
        }
    } else {
        s_state.complete = 0;
    }
}

static void cell_clicked_cb(lv_event_t * e)
{
    lv_obj_t * cell = lv_event_get_target(e);
    uint32_t user_data = (uint32_t) (uintptr_t) lv_obj_get_user_data(cell);
    uint8_t row = (uint8_t) (user_data >> 8);
    uint8_t col = (uint8_t) (user_data & 0xFF);

    if (row == s_state.cursor_row && col == s_state.cursor_col) {
        /* Ja e a celula selecionada -- alterna a orientacao ativa quando
         * ambas as direcoes existem ali. */
        uint8_t across = RATIMOS_CRUZADINHA_NO_ENTRY;
        uint8_t down = RATIMOS_CRUZADINHA_NO_ENTRY;
        ratimos_cruzadinha_entry_at(&s_puzzle, row, col, &across, &down);
        if (across != RATIMOS_CRUZADINHA_NO_ENTRY && down != RATIMOS_CRUZADINHA_NO_ENTRY) {
            s_state.active_is_across = !s_state.active_is_across;
            s_state.active_entry = s_state.active_is_across ? across : down;
        }
    } else {
        select_cell(row, col);
    }

    persist_state();
    render_all();
}

static void next_word_clicked_cb(lv_event_t * e)
{
    (void) e;
    uint8_t next = ratimos_cruzadinha_next_unsolved(&s_puzzle, &s_state, s_state.active_entry);
    if (next != RATIMOS_CRUZADINHA_NO_ENTRY) {
        jump_to_entry(next);
        persist_state();
        render_all();
    }
}

static void keyboard_value_changed_cb(lv_event_t * e)
{
    if (s_state.active_entry >= s_puzzle.word_count) {
        return;
    }

    lv_obj_t * matrix = lv_event_get_target(e);
    uint16_t id = lv_buttonmatrix_get_selected_button(matrix);
    const char * text = lv_buttonmatrix_get_button_text(matrix, id);
    if (!text) {
        return;
    }

    if (strcmp(text, "apagar") == 0) {
        ratimos_cruzadinha_set_letter(&s_puzzle, &s_state, s_state.cursor_row, s_state.cursor_col, ' ');
        retreat_cursor();
        check_completion();
        persist_state();
        render_all();
        return;
    }

    if (ratimos_cruzadinha_set_letter(&s_puzzle, &s_state, s_state.cursor_row, s_state.cursor_col, text[0])) {
        advance_cursor();
        check_completion();
        persist_state();
        render_all();
    }
}

static void clue_list_row_clicked_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    uint8_t entry_index = (uint8_t) (uintptr_t) lv_obj_get_user_data(row);
    jump_to_entry(entry_index);
    persist_state();
    lv_obj_add_flag(s_clue_list_overlay, LV_OBJ_FLAG_HIDDEN);
    render_all();
}

static void ver_todas_clicked_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_clear_flag(s_clue_list_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_clue_list_overlay);
}

static void clue_list_close_clicked_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_add_flag(s_clue_list_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void confirm_cancel_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_add_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void confirm_restart_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_add_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
    /* So apaga o TABULEIRO salvo -- a progressao ja conquistada nunca
     * regride por causa de um reset manual (proibicao do plano). */
    ratimos_storage_clear_game_state(RATIMOS_GAME_CRUZADINHA);
    start_new_puzzle();
    render_all();
}

static void novo_jogo_clicked_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_confirm_overlay);
}

/* ------------------------------------------------------------------------
 * Construcao.
 * ------------------------------------------------------------------------ */

static lv_obj_t * make_pill_w(lv_obj_t * parent, const char * text, lv_color_t bg, lv_event_cb_t cb, lv_coord_t width)
{
    lv_obj_t * pill = ratimos_panel_create(parent);
    lv_obj_set_size(pill, width, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(pill, 6, 0);
    lv_obj_set_style_bg_color(pill, bg, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pill, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(pill);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    lv_obj_center(label);

    return pill;
}

static lv_obj_t * build_clue_list_row(lv_obj_t * parent, lv_obj_t ** out_label)
{
    lv_obj_t * row = ratimos_panel_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, clue_list_row_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * label = lv_label_create(row);
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    lv_label_set_text(label, "");
    *out_label = label;

    return row;
}

static lv_obj_t * build_cruzadinha_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("cruzadinha", "toque numa palavra");

    /* Banco vazio/quebrado: caminho defensivo, sem grid nenhum (UI-SPEC). */
    if (ratimos_cruzadinha_puzzle_count() == 0) {
        s_unavailable_label = lv_label_create(shell.content);
        lv_label_set_text(s_unavailable_label, "quebra-cabeca indisponivel - volte mais tarde");
        lv_obj_set_style_text_color(s_unavailable_label, RATIMOS_COLOR_TEXT_MUTED, 0);
        lv_obj_set_width(s_unavailable_label, lv_pct(100));
        lv_label_set_long_mode(s_unavailable_label, LV_LABEL_LONG_MODE_WRAP);
        s_puzzle_ready = false;
        return shell.screen;
    }

    bool show_load_error = load_or_start_state();
    s_puzzle_ready = true;

    s_error_label = lv_label_create(shell.content);
    lv_label_set_text(s_error_label,
                      "nao foi possivel carregar seu progresso salvo - comecando um jogo novo");
    lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(s_error_label, lv_pct(100));
    lv_label_set_long_mode(s_error_label, LV_LABEL_LONG_MODE_WRAP);
    if (!show_load_error) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Tira de dica contextual (~40px, UI-SPEC): so a dica da entrada ativa,
     * nunca a lista inteira competindo com o grid. */
    s_clue_strip = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_clue_strip);
    lv_obj_set_width(s_clue_strip, lv_pct(100));
    lv_obj_set_height(s_clue_strip, CRUZ_CLUE_STRIP_H);
    lv_obj_set_flex_flow(s_clue_strip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_clue_strip, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_clue_strip, LV_OBJ_FLAG_SCROLLABLE);

    s_clue_strip_label = lv_label_create(s_clue_strip);
    lv_obj_set_flex_grow(s_clue_strip_label, 1);
    lv_label_set_long_mode(s_clue_strip_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_color(s_clue_strip_label, RATIMOS_COLOR_TEXT, 0);
    lv_label_set_text(s_clue_strip_label, "");

    s_next_word_btn = make_pill_w(s_clue_strip, "proxima palavra", RATIMOS_COLOR_PANEL, next_word_clicked_cb, 90);

    /* Grid -- construido UMA vez no tamanho maximo (11x11); render_grid()
     * decide quantas celulas ficam visiveis e o tamanho de cada uma por
     * quebra-cabeca. */
    s_board = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_board);
    lv_obj_set_style_bg_color(s_board, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_board, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_board, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t r = 0; r < CRUZ_MAX_DIM; r++) {
        for (uint8_t c = 0; c < CRUZ_MAX_DIM; c++) {
            lv_obj_t * cell = lv_obj_create(s_board);
            lv_obj_remove_style_all(cell);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(cell, 0, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_user_data(cell, (void *) (uintptr_t) (((uint32_t) r << 8) | c));
            lv_obj_add_event_cb(cell, cell_clicked_cb, LV_EVENT_CLICKED, NULL);
            s_cell[r][c] = cell;

            lv_obj_t * number_label = lv_label_create(cell);
            lv_label_set_text(number_label, "");
            lv_obj_set_style_text_color(number_label, RATIMOS_COLOR_TEXT_MUTED, 0);
            lv_obj_align(number_label, LV_ALIGN_TOP_LEFT, 1, 0);
            s_cell_number_label[r][c] = number_label;

            lv_obj_t * letter_label = lv_label_create(cell);
            lv_label_set_text(letter_label, "");
            lv_obj_set_style_text_color(letter_label, RATIMOS_COLOR_TEXT, 0);
            lv_obj_center(letter_label);
            s_cell_letter_label[r][c] = letter_label;
        }
    }

    /* Pilula "ver todas as dicas" (UI-SPEC affordance secundaria). */
    make_pill_w(shell.content, "ver todas as dicas", RATIMOS_COLOR_PANEL, ver_todas_clicked_cb, lv_pct(100));

    /* Teclado A-Z compartilhado + apagar -- risco flagueado (UI-SPEC/
     * RESEARCH): celulas de ~30px ficam abaixo do alvo de toque ideal de
     * 44px porque o grid precisa caber num painel de 300px -- risco aceito
     * e explicitamente sinalizado; confirmacao real com dedo/stylus fica
     * pra verificacao de hardware da Fase 3. */
    s_keyboard = lv_buttonmatrix_create(shell.content);
    lv_buttonmatrix_set_map(s_keyboard, CRUZ_KEYBOARD_MAP);
    lv_obj_set_width(s_keyboard, lv_pct(100));
    lv_obj_set_height(s_keyboard, 110);
    lv_obj_set_style_pad_column(s_keyboard, 2, 0);
    lv_obj_set_style_pad_row(s_keyboard, 2, 0);
    lv_obj_set_style_bg_opa(s_keyboard, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_keyboard, 0, 0);
    lv_obj_set_style_bg_color(s_keyboard, RATIMOS_COLOR_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_keyboard, RATIMOS_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_keyboard, 4, LV_PART_ITEMS);
    lv_obj_add_event_cb(s_keyboard, keyboard_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Banner de vitoria (Display-tier, UI-SPEC). */
    s_banner_label = lv_label_create(shell.content);
    lv_label_set_text(s_banner_label, "");
    lv_obj_set_width(s_banner_label, lv_pct(100));
    lv_label_set_long_mode(s_banner_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(s_banner_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_banner_label, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_banner_label, &ratimos_font_title_20, 0);
    lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);

    s_castle_label = lv_label_create(shell.content);
    lv_label_set_text(s_castle_label, "+1 no castelo");
    lv_obj_set_width(s_castle_label, lv_pct(100));
    lv_obj_set_style_text_align(s_castle_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_castle_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);

    make_pill_w(shell.content, "novo jogo", RATIMOS_COLOR_PANEL, novo_jogo_clicked_cb, lv_pct(100));

    /* Overlay: lista completa de dicas (JOGOS-04's requisito literal de
     * dica numerada para quem quer navegar). Filho de shell.screen (nao
     * shell.content) para flutuar por cima do resto, igual aos dialogos de
     * confirmacao das outras telas. */
    s_clue_list_overlay = ratimos_panel_create(shell.screen);
    lv_obj_set_size(s_clue_list_overlay, 300, 400);
    lv_obj_align(s_clue_list_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(s_clue_list_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_clue_list_overlay, 6, 0);
    lv_obj_add_flag(s_clue_list_overlay, LV_OBJ_FLAG_HIDDEN);

    make_pill_w(s_clue_list_overlay, "fechar", RATIMOS_COLOR_ACCENT, clue_list_close_clicked_cb, lv_pct(100));

    s_clue_list_across_header = lv_label_create(s_clue_list_overlay);
    lv_label_set_text(s_clue_list_across_header, "horizontais");
    lv_obj_set_style_text_color(s_clue_list_across_header, RATIMOS_COLOR_ACCENT, 0);

    for (uint8_t i = 0; i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        s_clue_list_across_row[i] = build_clue_list_row(s_clue_list_overlay, &s_clue_list_across_label[i]);
    }

    s_clue_list_down_header = lv_label_create(s_clue_list_overlay);
    lv_label_set_text(s_clue_list_down_header, "verticais");
    lv_obj_set_style_text_color(s_clue_list_down_header, RATIMOS_COLOR_ACCENT, 0);

    for (uint8_t i = 0; i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        s_clue_list_down_row[i] = build_clue_list_row(s_clue_list_overlay, &s_clue_list_down_label[i]);
    }

    /* Dialogo de confirmacao destrutiva (Copywriting Contract) -- mesmo
     * padrao de sudoku.c/termo.c/conexo.c. */
    s_confirm_overlay = ratimos_panel_create(shell.screen);
    lv_obj_set_size(s_confirm_overlay, 260, 150);
    lv_obj_align(s_confirm_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(s_confirm_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_confirm_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_confirm_overlay, 12, 0);
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_confirm_overlay);

    lv_obj_t * confirm_msg = lv_label_create(s_confirm_overlay);
    lv_label_set_text(confirm_msg,
                      "comecar de novo? seu progresso atual nesse jogo sera perdido.");
    lv_obj_set_width(confirm_msg, lv_pct(100));
    lv_label_set_long_mode(confirm_msg, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(confirm_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(confirm_msg, RATIMOS_COLOR_TEXT, 0);

    lv_obj_t * confirm_actions = lv_obj_create(s_confirm_overlay);
    lv_obj_remove_style_all(confirm_actions);
    lv_obj_set_width(confirm_actions, lv_pct(100));
    lv_obj_set_height(confirm_actions, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(confirm_actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(confirm_actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(confirm_actions, 8, 0);
    lv_obj_clear_flag(confirm_actions, LV_OBJ_FLAG_SCROLLABLE);

    make_pill_w(confirm_actions, "cancelar", RATIMOS_COLOR_PANEL, confirm_cancel_cb, 100);
    make_pill_w(confirm_actions, "recomecar", RATIMOS_COLOR_ACCENT, confirm_restart_cb, 100);

    render_all();

    return shell.screen;
}

void ratimos_cruzadinha_show(lv_event_t * e)
{
    (void) e;
    if (!s_cruzadinha_screen) {
        s_cruzadinha_screen = build_cruzadinha_screen();
    }
    lv_screen_load(s_cruzadinha_screen);
}
