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
 * a lista de palavras genuinamente precisa).
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cruzadinha.h"
#include "cruzadinha_engine.h"

#include "../../app_shell.h"
#include "../jogos_app.h" /* voltar -> ./home/jogos (02.1-14) */
#include "../../theme.h"
#include "../../fonts/ratimos_fonts.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

#define CRUZ_MAX_DIM        RATIMOS_CRUZADINHA_MAX_DIM
/* Orcamento vertical (fix de checkpoint 02.1-14): dica + acoes + grade +
 * teclado + 3 gaps <= RATIMOS_CONTENT_INNER_H (386), sem rolar:
 * 30+26+208+98+24 = 386. A ALTURA e' o limite que manda (antes o grid
 * usava os 300px de largura e o teclado ficava abaixo da dobra). */
#define CRUZ_BOARD_BUDGET_PX 208
#define CRUZ_CLUE_STRIP_H   30
#define CRUZ_PILL_H         RATIMOS_PILL_H
#define CRUZ_KEY_H          30
#define CRUZ_KEY_GAP        4
#define CRUZ_KEYBOARD_H     (3 * CRUZ_KEY_H + 2 * CRUZ_KEY_GAP)

/* Teclado compartilhado A-Z + apagar -- mesma convencao visual do teclado do
 * termo (lv_buttonmatrix neutro, sem cor por tecla). */
static const char * const CRUZ_KEYBOARD_MAP[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "z", "x", "c", "v", "b", "n", "m", "apagar", NULL
};
/* "apagar" com 2 unidades de largura (plano 02.1-13): em largura igual o
 * texto estourava a tecla. */
static const lv_buttonmatrix_ctrl_t CRUZ_KEYBOARD_CTRL[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2
};

/* ------------------------------------------------------------------------
 * Tela (LVGL) -- construida a cada visita, deletada ao sair.
 *
 * ratimos_cruzadinha_show() constroi a tela a partir do save e
 * ratimos_screen_load() (app_shell.h) a deleta quando a jogadora navega pra
 * outra tela (plano 02.1-15, deferred-items #1: as telas em cache pra
 * sempre esgotavam o heap do LVGL). Enquanto a tela existe, cada
 * jogada so re-renderiza via render_all().
 * cruzadinha_screen_deleted_cb() zera os ponteiros de widget no LV_EVENT_DELETE.
 * ------------------------------------------------------------------------ */

static lv_obj_t * s_cruzadinha_screen = NULL;

static lv_obj_t * s_unavailable_label = NULL;
static lv_obj_t * s_error_label = NULL;
/* Grade inteira = UM objeto desenhado (board_draw_cb) + um clique por
 * coordenada (board_clicked_cb) -- plano 02.1-15 Task 3. Antes eram 121
 * lv_obj (celula) + 242 labels (letra/numero), cada um com estilos locais,
 * ~100KB de heap so' pra grade. */
static lv_obj_t * s_board = NULL;
static lv_obj_t * s_clue_strip = NULL;
static lv_obj_t * s_clue_strip_label = NULL;
static lv_obj_t * s_next_word_btn = NULL;
static lv_obj_t * s_keyboard = NULL;
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_confirm_overlay = NULL;
/* Lista de dicas: construida so' quando "dicas" e' tocado e deletada ao
 * fechar (02.1-15 Task 3) -- eram 32 linhas-painel montadas escondidas em
 * toda visita. NULL = lista fechada. */
static lv_obj_t * s_clue_list_overlay = NULL;
static lv_obj_t * s_clue_list_across_row[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_down_row[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_across_label[RATIMOS_CRUZADINHA_MAX_WORDS];
static lv_obj_t * s_clue_list_down_label[RATIMOS_CRUZADINHA_MAX_WORDS];

static ratimos_cruzadinha_state_t s_state;
static ratimos_cruzadinha_puzzle_t s_puzzle;
static ratimos_cruzadinha_grid_t s_grid;
static bool s_puzzle_ready = false;

/* Tela deletada ao sair (ratimos_screen_load, plano 02.1-15): zera todo
 * ponteiro de widget pro proximo ratimos_cruzadinha_show() reconstruir a partir
 * do save -- nenhum callback pode achar um handle de uma tela que ja sumiu. */
static void cruzadinha_screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_cruzadinha_screen = NULL;
    s_unavailable_label = NULL;
    s_error_label = NULL;
    s_board = NULL;
    s_clue_strip = NULL;
    s_clue_strip_label = NULL;
    s_next_word_btn = NULL;
    s_keyboard = NULL;
    s_banner_label = NULL;
    s_castle_label = NULL;
    s_confirm_overlay = NULL;
    s_clue_list_overlay = NULL;
    memset(s_clue_list_across_row, 0, sizeof(s_clue_list_across_row));
    memset(s_clue_list_down_row, 0, sizeof(s_clue_list_down_row));
    memset(s_clue_list_across_label, 0, sizeof(s_clue_list_across_label));
    memset(s_clue_list_down_label, 0, sizeof(s_clue_list_down_label));
    s_puzzle_ready = false;
}

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

/* Cores/borda de uma celula de letra -- mesmas 3 variantes de antes
 * (selecionada / na entrada ativa / normal). */
static void cell_style(uint8_t r, uint8_t c, lv_draw_rect_dsc_t * dsc)
{
    if (r == s_state.cursor_row && c == s_state.cursor_col) {
        dsc->bg_color = RATIMOS_COLOR_PANEL_ACTIVE;
        dsc->border_color = RATIMOS_COLOR_ACCENT;
        dsc->border_width = 2;
    } else if (cell_is_in_active_entry(r, c)) {
        dsc->bg_color = RATIMOS_COLOR_PANEL_ACTIVE;
        dsc->border_color = RATIMOS_COLOR_PANEL;
        dsc->border_width = 1;
    } else {
        dsc->bg_color = RATIMOS_COLOR_PANEL;
        dsc->border_color = RATIMOS_COLOR_PANEL_ACTIVE;
        dsc->border_width = 1;
    }
}

/*
 * Desenha a grade no LV_EVENT_DRAW_MAIN do board: por celula, o fundo +
 * borda, o numero da dica (mono 10, canto superior esquerdo, 1px pra
 * dentro da borda) e a letra digitada (fonte do tema, centralizada) --
 * pixel a pixel o mesmo layout dos antigos lv_obj/labels por celula.
 * Celula fora do desenho: bloco solido da cor do fundo, sem borda.
 */
static void board_draw_cb(lv_event_t * e)
{
    if (!s_puzzle_ready) {
        return;
    }
    lv_obj_t * board = lv_event_get_target(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t bc;
    lv_obj_get_coords(board, &bc);

    const lv_coord_t cell_px = cell_px_for_dim(s_puzzle.grid_w);
    const lv_font_t * letter_font = lv_obj_get_style_text_font(board, LV_PART_MAIN);
    const int32_t letter_h = lv_font_get_line_height(letter_font);

    for (uint8_t r = 0; r < s_puzzle.grid_h && r < CRUZ_MAX_DIM; r++) {
        for (uint8_t c = 0; c < s_puzzle.grid_w && c < CRUZ_MAX_DIM; c++) {
            lv_area_t a;
            a.x1 = bc.x1 + (int32_t) c * cell_px;
            a.y1 = bc.y1 + (int32_t) r * cell_px;
            a.x2 = a.x1 + cell_px - 1;
            a.y2 = a.y1 + cell_px - 1;

            lv_draw_rect_dsc_t rect;
            lv_draw_rect_dsc_init(&rect);
            rect.radius = 0;
            rect.bg_opa = LV_OPA_COVER;

            if (!s_grid.is_cell[r][c]) {
                rect.bg_color = RATIMOS_COLOR_BG;
                rect.border_width = 0;
                lv_draw_rect(layer, &rect, &a);
                continue;
            }

            cell_style(r, c, &rect);
            lv_draw_rect(layer, &rect, &a);

            if (s_grid.numbers[r][c] != 0) {
                char num[4];
                lv_snprintf(num, sizeof(num), "%u", (unsigned) s_grid.numbers[r][c]);
                lv_draw_label_dsc_t ld;
                lv_draw_label_dsc_init(&ld);
                ld.font = &ratimos_font_mono_10;
                ld.color = RATIMOS_COLOR_TEXT_MUTED;
                ld.text = num;
                ld.text_local = 1;
                lv_area_t na = { a.x1 + rect.border_width + 1, a.y1 + rect.border_width, a.x2, a.y2 };
                lv_draw_label(layer, &ld, &na);
            }

            if (s_state.entered[r][c] != 0) {
                char letter[2] = { s_state.entered[r][c], 0 };
                lv_draw_label_dsc_t ld;
                lv_draw_label_dsc_init(&ld);
                ld.font = letter_font;
                ld.color = RATIMOS_COLOR_TEXT;
                ld.align = LV_TEXT_ALIGN_CENTER;
                ld.text = letter;
                ld.text_local = 1;
                lv_area_t la = { a.x1, a.y1 + (cell_px - letter_h) / 2, a.x2, 0 };
                la.y2 = la.y1 + letter_h - 1;
                lv_draw_label(layer, &ld, &la);
            }
        }
    }
}

/* Tamanho/posicao do board por quebra-cabeca + redesenho. */
static void render_grid(void)
{
    lv_coord_t cell_px = cell_px_for_dim(s_puzzle.grid_w);
    lv_coord_t board_px = (lv_coord_t) (cell_px * s_puzzle.grid_w);
    lv_obj_set_size(s_board, board_px, board_px);
    /* Centraliza a grade (agora mais estreita que o content) no eixo X. */
    lv_obj_set_style_margin_left(s_board, (RATIMOS_SCREEN_W - 2 * RATIMOS_CONTENT_PAD - board_px) / 2, 0);
    lv_obj_invalidate(s_board);
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
    if (s_clue_list_overlay == NULL) {
        return; /* lista fechada: nada montado */
    }

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

        /* WR-04: defesa em profundidade -- hoje across_count+down_count <=
         * word_count <= RATIMOS_CRUZADINHA_MAX_WORDS e garantido pelo gerador
         * e por _Static_assert de dimensionamento, mas nada aqui checava os
         * dois contadores individualmente no PONTO DE USO antes de indexar
         * s_clue_list_across_label[]/s_clue_list_across_row[] (tamanho fixo
         * RATIMOS_CRUZADINHA_MAX_WORDS). Um `break` aqui casa com a
         * disciplina de bounds-check do resto do arquivo/engine em vez de
         * confiar so no invariante upstream. */
        if (w->is_across) {
            if (across_count >= RATIMOS_CRUZADINHA_MAX_WORDS) {
                break;
            }
            lv_label_set_text(s_clue_list_across_label[across_count], buf);
            lv_obj_set_user_data(s_clue_list_across_row[across_count], (void *) (uintptr_t) i);
            lv_obj_clear_flag(s_clue_list_across_row[across_count], LV_OBJ_FLAG_HIDDEN);
            across_count++;
        } else {
            if (down_count >= RATIMOS_CRUZADINHA_MAX_WORDS) {
                break;
            }
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
            if (ratimos_storage_record_daily_win(RATIMOS_GAME_CRUZADINHA, ratimos_daily_index())) {
                s_state.daily_win_recorded = 1;
            }
        }
    } else {
        s_state.complete = 0;
    }
}

/* Toque na grade: a celula vem da coordenada do toque (o board e' um
 * objeto so'). Celula fora do desenho nao reage -- igual a antes, quando
 * ela nao era clicavel. */
static void board_clicked_cb(lv_event_t * e)
{
    if (!s_puzzle_ready) {
        return;
    }
    lv_indev_t * indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t bc;
    lv_obj_get_coords(lv_event_get_target(e), &bc);
    if (p.x < bc.x1 || p.y < bc.y1) {
        return;
    }
    lv_coord_t cell_px = cell_px_for_dim(s_puzzle.grid_w);
    uint32_t col32 = (uint32_t) (p.x - bc.x1) / (uint32_t) cell_px;
    uint32_t row32 = (uint32_t) (p.y - bc.y1) / (uint32_t) cell_px;
    if (row32 >= s_puzzle.grid_h || col32 >= s_puzzle.grid_w || row32 >= CRUZ_MAX_DIM || col32 >= CRUZ_MAX_DIM) {
        return;
    }
    uint8_t row = (uint8_t) row32;
    uint8_t col = (uint8_t) col32;
    if (!s_grid.is_cell[row][col]) {
        return;
    }

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

static void build_clue_list_overlay(void);

/* Fecha = deleta o scrim inteiro (painel + linhas). Chamado de dentro do
 * clique de um filho dele -- o LVGL 9 trata isso (mesmo caso do "voltar"
 * deletando a tela, ver ratimos_screen_load). */
static void close_clue_list(void)
{
    if (s_clue_list_overlay == NULL) {
        return;
    }
    lv_obj_t * scrim = lv_obj_get_parent(s_clue_list_overlay);
    s_clue_list_overlay = NULL;
    memset(s_clue_list_across_row, 0, sizeof(s_clue_list_across_row));
    memset(s_clue_list_down_row, 0, sizeof(s_clue_list_down_row));
    memset(s_clue_list_across_label, 0, sizeof(s_clue_list_across_label));
    memset(s_clue_list_down_label, 0, sizeof(s_clue_list_down_label));
    lv_obj_delete(scrim);
}

static void clue_list_row_clicked_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    uint8_t entry_index = (uint8_t) (uintptr_t) lv_obj_get_user_data(row);
    jump_to_entry(entry_index);
    persist_state();
    close_clue_list();
    render_all();
}

static void ver_todas_clicked_cb(lv_event_t * e)
{
    (void) e;
    if (s_clue_list_overlay == NULL) {
        build_clue_list_overlay();
        render_clue_list();
    }
    ratimos_modal_show(s_clue_list_overlay);
}

static void clue_list_close_clicked_cb(lv_event_t * e)
{
    (void) e;
    close_clue_list();
}

static void confirm_cancel_cb(lv_event_t * e)
{
    (void) e;
    ratimos_modal_hide(s_confirm_overlay);
}

static void confirm_restart_cb(lv_event_t * e)
{
    (void) e;
    ratimos_modal_hide(s_confirm_overlay);
    /* So apaga o TABULEIRO salvo -- a progressao ja conquistada nunca
     * regride por causa de um reset manual (proibicao do plano). */
    ratimos_storage_clear_game_state(RATIMOS_GAME_CRUZADINHA);
    start_new_puzzle();
    render_all();
}

static void novo_jogo_clicked_cb(lv_event_t * e)
{
    (void) e;
    ratimos_modal_show(s_confirm_overlay);
}

/* ------------------------------------------------------------------------
 * Construcao.
 * ------------------------------------------------------------------------ */

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

/* Monta a lista de dicas (scrim + painel + 2 x MAX_WORDS linhas) na tela
 * ativa da cruzadinha -- so' quando "dicas" e' tocado; close_clue_list()
 * deleta tudo de novo. */
static void build_clue_list_overlay(void)
{
    /* Overlay: lista completa de dicas (JOGOS-04's requisito literal de
     * dica numerada para quem quer navegar). Modal compartilhado
     * (ratimos_modal_create, theme.h): scrim FLOATING de tela cheia +
     * painel centralizado -- igual aos dialogos de confirmacao. */
    s_clue_list_overlay = ratimos_modal_create(s_cruzadinha_screen, 300, 400);
    /* Lista longa e rolavel: comeca do topo (o centro do helper empurraria
     * o inicio da lista pra cima, fora do alcance da rolagem). */
    lv_obj_set_flex_align(s_clue_list_overlay, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_flow(s_clue_list_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_clue_list_overlay, 6, 0);

    lv_obj_t * close_btn = ratimos_button_create(s_clue_list_overlay, "fechar", clue_list_close_clicked_cb, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(close_btn, RATIMOS_COLOR_ACCENT, 0);

    lv_obj_t * across_header = lv_label_create(s_clue_list_overlay);
    lv_label_set_text(across_header, "horizontais");
    lv_obj_set_style_text_color(across_header, RATIMOS_COLOR_ACCENT, 0);

    for (uint8_t i = 0; i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        s_clue_list_across_row[i] = build_clue_list_row(s_clue_list_overlay, &s_clue_list_across_label[i]);
    }

    lv_obj_t * down_header = lv_label_create(s_clue_list_overlay);
    lv_label_set_text(down_header, "verticais");
    lv_obj_set_style_text_color(down_header, RATIMOS_COLOR_ACCENT, 0);

    for (uint8_t i = 0; i < RATIMOS_CRUZADINHA_MAX_WORDS; i++) {
        s_clue_list_down_row[i] = build_clue_list_row(s_clue_list_overlay, &s_clue_list_down_label[i]);
    }
}

static lv_obj_t * build_cruzadinha_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create_with_back("./home/jogos/cruzadinha", "toque numa palavra", ratimos_jogos_show);

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
    lv_obj_set_flex_align(s_clue_strip, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_clue_strip, LV_OBJ_FLAG_SCROLLABLE);

    s_clue_strip_label = lv_label_create(s_clue_strip);
    lv_obj_set_flex_grow(s_clue_strip_label, 1);
    lv_obj_set_height(s_clue_strip_label, CRUZ_CLUE_STRIP_H);
    /* Ate 2 linhas; dica mais longa corta com "..." em vez de vazar. */
    lv_label_set_long_mode(s_clue_strip_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_color(s_clue_strip_label, RATIMOS_COLOR_TEXT, 0);
    lv_label_set_text(s_clue_strip_label, "");

    /* Acoes numa unica linha (fix de checkpoint 02.1-14): proxima / dicas /
     * novo jogo, mesma altura de pilula. Antes eram 3 linhas separadas
     * (botao ao lado da dica + 2 botoes de largura total) e empurravam o
     * teclado pra baixo da dobra. */
    lv_obj_t * actions = lv_obj_create(shell.content);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, CRUZ_PILL_H);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 4, 0);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

    s_next_word_btn = ratimos_button_create(actions, "proxima", next_word_clicked_cb, LV_SIZE_CONTENT, CRUZ_PILL_H);
    lv_obj_set_flex_grow(s_next_word_btn, 1);
    lv_obj_t * dicas_btn = ratimos_button_create(actions, "dicas", ver_todas_clicked_cb, LV_SIZE_CONTENT, CRUZ_PILL_H);
    lv_obj_set_flex_grow(dicas_btn, 1);
    lv_obj_t * novo_btn = ratimos_button_create(actions, "novo jogo", novo_jogo_clicked_cb, LV_SIZE_CONTENT, CRUZ_PILL_H);
    lv_obj_set_flex_grow(novo_btn, 1);

    /* Grid -- UM objeto desenhado (board_draw_cb) com clique por coordenada
     * (board_clicked_cb); render_grid() ajusta o tamanho por quebra-cabeca. */
    s_board = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_board);
    lv_obj_set_style_bg_color(s_board, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_board, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_board, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_board, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_board, board_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event_cb(s_board, board_clicked_cb, LV_EVENT_CLICKED, NULL);

    /* Teclado A-Z compartilhado + apagar -- risco flagueado (UI-SPEC/
     * RESEARCH): celulas de ~30px ficam abaixo do alvo de toque ideal de
     * 44px porque o grid precisa caber num painel de 300px -- risco aceito
     * e explicitamente sinalizado; confirmacao real com dedo/stylus fica
     * pra verificacao de hardware da Fase 3. */
    s_keyboard = lv_buttonmatrix_create(shell.content);
    lv_buttonmatrix_set_map(s_keyboard, CRUZ_KEYBOARD_MAP);
    lv_buttonmatrix_set_ctrl_map(s_keyboard, CRUZ_KEYBOARD_CTRL);
    lv_obj_set_width(s_keyboard, lv_pct(100));
    lv_obj_set_height(s_keyboard, CRUZ_KEYBOARD_H);
    ratimos_bevel_style_buttonmatrix(s_keyboard);
    lv_obj_set_style_pad_column(s_keyboard, 2, 0);
    lv_obj_set_style_pad_row(s_keyboard, CRUZ_KEY_GAP, 0);
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

    /* Dialogo de confirmacao destrutiva (Copywriting Contract) -- mesmo
     * padrao de sudoku.c/termo.c/conexo.c. */
    s_confirm_overlay = ratimos_modal_create(shell.screen, 260, 150);
    lv_obj_set_flex_flow(s_confirm_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_confirm_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_confirm_overlay, 12, 0);
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
    ratimos_modal_hide(s_confirm_overlay);

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

    ratimos_button_create(confirm_actions, "cancelar", confirm_cancel_cb, 100, LV_SIZE_CONTENT);
    lv_obj_t * restart_btn = ratimos_button_create(confirm_actions, "recomecar", confirm_restart_cb, 100, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(restart_btn, RATIMOS_COLOR_ACCENT, 0);

    render_all();

    return shell.screen;
}

void ratimos_cruzadinha_show(lv_event_t * e)
{
    (void) e;
    if (!s_cruzadinha_screen) {
        s_cruzadinha_screen = build_cruzadinha_screen();
        lv_obj_add_event_cb(s_cruzadinha_screen, cruzadinha_screen_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    ratimos_screen_load(s_cruzadinha_screen);
}
