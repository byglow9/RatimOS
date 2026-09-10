/*
 * Sudoku (JOGOS-01) -- tela.
 *
 * Persistencia (JOGOS-02): toda escrita de celula, troca de modo ou reset
 * grava na hora via ratimos_storage_save_game_state() -- salvar so ao sair
 * da tela perderia o progresso num fechamento inesperado, e o cache de tela
 * em memoria (build-once), sozinho, nao sobrevive a um relaunch do processo.
 *
 * Este arquivo NAO abre arquivo nenhum: todo byte que chega ao disco passa
 * pela Storage/Content API (D-10), igual a conexo.c.
 */
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "sudoku.h"
#include "sudoku_engine.h"

#include "../../app_shell.h"
#include "../../theme.h"
#include "../../fonts/ratimos_fonts.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

#define SUDOKU_CELL_PX   32
#define SUDOKU_BOARD_PX  (SUDOKU_CELL_PX * 9)
#define SUDOKU_PILL_H    28
#define SUDOKU_KEYPAD_H  36
#define SUDOKU_NO_SELECTION 0xFFu

/* ------------------------------------------------------------------------
 * Tela (LVGL) -- cache-once, igual a jogos_app.c / conexo.c.
 *
 * A tela e construida UMA vez e guardada em s_sudoku_screen; toda jogada
 * apenas atualiza os objetos ja existentes via render_board(). Reconstruir a
 * tela a cada visita e exatamente o vazamento de heap do LVGL que a Fase 1 ja
 * corrigiu -- nao pode voltar.
 * ------------------------------------------------------------------------ */

static lv_obj_t * s_sudoku_screen = NULL;

static lv_obj_t * s_error_label = NULL;
static lv_obj_t * s_pill_row = NULL;
static lv_obj_t * s_pills[RATIMOS_SUDOKU_MODE_COUNT];
static lv_obj_t * s_pill_labels[RATIMOS_SUDOKU_MODE_COUNT];
static lv_obj_t * s_board_wrap = NULL;
static lv_obj_t * s_board = NULL;
static lv_obj_t * s_cells[9][9];
static lv_obj_t * s_cell_labels[9][9];
static lv_obj_t * s_keypad = NULL;
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_gen_fail_container = NULL;
static lv_obj_t * s_confirm_overlay = NULL;

static ratimos_sudoku_state_t s_state;
static bool s_generation_failed = false;
static ratimos_sudoku_mode_t s_failed_mode = RATIMOS_SUDOKU_MEDIO;
static ratimos_sudoku_mode_t s_pending_mode = RATIMOS_SUDOKU_MEDIO;

/* Uma unica pilula "novo jogo" pode disparar dois tipos de confirmacao
 * destrutiva -- trocar de modo ou reiniciar o mesmo modo -- entao o overlay
 * precisa saber qual das duas o botao "recomecar" deve executar. */
typedef enum {
    SUDOKU_PENDING_MODE_SWITCH = 0,
    SUDOKU_PENDING_RESET
} sudoku_pending_action_t;

static sudoku_pending_action_t s_pending_action = SUDOKU_PENDING_MODE_SWITCH;

static const char * const SUDOKU_MODE_LABELS[RATIMOS_SUDOKU_MODE_COUNT] = {
    "facil", "medio", "dificil", "diario"
};

static const int32_t SUDOKU_GRID_DSC[10] = {
    SUDOKU_CELL_PX, SUDOKU_CELL_PX, SUDOKU_CELL_PX, SUDOKU_CELL_PX, SUDOKU_CELL_PX,
    SUDOKU_CELL_PX, SUDOKU_CELL_PX, SUDOKU_CELL_PX, SUDOKU_CELL_PX, LV_GRID_TEMPLATE_LAST
};

static const char * const SUDOKU_KEYPAD_MAP[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "apagar", NULL
};

static void render_board(void);
static void persist_state(void);

/* Semente "fresh" para facil/medio/dificil (RESEARCH A5: sem RTC no
 * native_sim, o relogio do host + um contador local bastam para nao repetir
 * o mesmo tabuleiro em trocas de modo consecutivas dentro do mesmo segundo).
 * O modo diario NUNCA usa isto -- sempre ratimos_daily_seed(). */
static uint32_t fresh_seed(void)
{
    static uint32_t counter = 0;
    counter++;
    return (uint32_t) time(NULL) ^ (counter * 2654435761u);
}

static bool has_progress(void)
{
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (s_state.filled[r][c] != 0) {
                return true;
            }
        }
    }
    return false;
}

/* Gera um tabuleiro novo para `mode` e substitui s_state SOMENTE em caso de
 * sucesso -- numa falha, o estado anterior fica intacto e o chamador decide
 * o que fazer (mostrar a tela de falha, sem tocar no ultimo save valido). */
static bool generate_for_mode(ratimos_sudoku_mode_t mode)
{
    uint32_t seed = (mode == RATIMOS_SUDOKU_DIARIO) ? ratimos_daily_seed(RATIMOS_GAME_SUDOKU) : fresh_seed();

    ratimos_sudoku_state_t fresh;
    if (!ratimos_sudoku_generate(&fresh, mode, seed, 64)) {
        return false;
    }

    fresh.cursor_row = SUDOKU_NO_SELECTION;
    fresh.cursor_col = SUDOKU_NO_SELECTION;
    s_state = fresh;
    return true;
}

static void switch_to_mode(ratimos_sudoku_mode_t mode)
{
    if (generate_for_mode(mode)) {
        s_generation_failed = false;
        persist_state();
    } else {
        s_generation_failed = true;
        s_failed_mode = mode;
    }
    render_board();
}

static void persist_state(void)
{
    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &s_state, sizeof(s_state));
    blob.used = sizeof(s_state);

    ratimos_storage_save_game_state(RATIMOS_GAME_SUDOKU, &blob);
}

/* Decide o estado inicial: restaura um save valido, comeca no modo medio
 * quando nao ha save, ou comeca limpo COM aviso quando o save esta
 * corrompido. Mitigacao T-02.1-02/T-02.1-12: qualquer `mode` fora de
 * RATIMOS_SUDOKU_MODE_COUNT ou digito fora de 0-9 e tratado como save
 * invalido -- nunca alimenta o render/geracao com um valor fora de faixa. */
static bool load_or_start_state(void)
{
    ratimos_game_state_t blob;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_SUDOKU, &blob);

    if (status == RATIMOS_GAME_STATE_OK && blob.used == sizeof(s_state)) {
        ratimos_sudoku_state_t restored;
        memcpy(&restored, blob.bytes, sizeof(restored));

        bool valid = restored.mode < RATIMOS_SUDOKU_MODE_COUNT;
        if (valid) {
            for (int r = 0; r < 9 && valid; r++) {
                for (int c = 0; c < 9; c++) {
                    if (restored.given[r][c] > 9 || restored.filled[r][c] > 9) {
                        valid = false;
                        break;
                    }
                }
            }
        }

        if (valid) {
            s_state = restored;
            status = RATIMOS_GAME_STATE_OK;
        } else {
            status = RATIMOS_GAME_STATE_INVALID;
        }
    } else if (status == RATIMOS_GAME_STATE_OK) {
        status = RATIMOS_GAME_STATE_INVALID; /* tamanho do blob nao bate com esta versao */
    }

    if (status == RATIMOS_GAME_STATE_OK) {
        s_generation_failed = false;
        return false; /* restaurado, sem aviso de erro */
    }

    /* ABSENT ou INVALID: comeca um jogo novo no modo medio. */
    if (generate_for_mode(RATIMOS_SUDOKU_MEDIO)) {
        s_generation_failed = false;
        persist_state();
    } else {
        s_generation_failed = true;
        s_failed_mode = RATIMOS_SUDOKU_MEDIO;
    }

    return status == RATIMOS_GAME_STATE_INVALID;
}

static void render_pills(void)
{
    for (int m = 0; m < RATIMOS_SUDOKU_MODE_COUNT; m++) {
        bool selected = ((int) s_state.mode == m);
        lv_obj_set_style_bg_color(s_pills[m], selected ? RATIMOS_COLOR_PANEL_ACTIVE : RATIMOS_COLOR_PANEL, 0);
        lv_obj_set_style_border_color(s_pills[m], RATIMOS_COLOR_ACCENT, 0);
        lv_obj_set_style_border_width(s_pills[m], selected ? 2 : 0, 0);
        lv_obj_set_style_text_color(s_pill_labels[m], selected ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_TEXT_MUTED, 0);
    }
}

static void render_cells(void)
{
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            uint8_t given = s_state.given[r][c];
            uint8_t filled = s_state.filled[r][c];
            uint8_t digit = given != 0 ? given : filled;

            char buf[2] = { 0, 0 };
            if (digit >= 1 && digit <= 9) {
                buf[0] = (char) ('0' + digit);
            }
            lv_label_set_text(s_cell_labels[r][c], buf);

            if (given != 0) {
                lv_obj_set_style_text_color(s_cell_labels[r][c], RATIMOS_COLOR_TEXT, 0);
                lv_obj_clear_flag(s_cells[r][c], LV_OBJ_FLAG_CLICKABLE);
            } else {
                bool conflicting = digit != 0 && ratimos_sudoku_cell_conflicts(&s_state, r, c);
                lv_obj_set_style_text_color(s_cell_labels[r][c],
                                            conflicting ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_TEXT_MUTED, 0);
                lv_obj_add_flag(s_cells[r][c], LV_OBJ_FLAG_CLICKABLE);
            }

            bool selected = (s_state.cursor_row == r && s_state.cursor_col == c);
            if (selected) {
                lv_obj_set_style_bg_color(s_cells[r][c], RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_border_color(s_cells[r][c], RATIMOS_COLOR_ACCENT, 0);
                lv_obj_set_style_border_width(s_cells[r][c], 2, 0);
            } else {
                /* Divisor 3x3 (UI-SPEC): as celulas na ultima linha/coluna
                 * de cada caixa (indices 2 e 5) ganham uma borda mais grossa
                 * e tingida de acento -- implementado so por celula, sem
                 * objeto extra nenhum. */
                bool thick_divider = (c == 2 || c == 5 || r == 2 || r == 5);
                lv_obj_set_style_bg_color(s_cells[r][c], RATIMOS_COLOR_PANEL, 0);
                lv_obj_set_style_border_color(s_cells[r][c],
                                              thick_divider ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_border_width(s_cells[r][c], thick_divider ? 2 : 1, 0);
            }
        }
    }
}

/* Banner "resolvido!" (Display-tier, UI-SPEC): so aparece quando o board
 * atual esta marcado como resolvido. A linha "+1 no castelo" so acompanha o
 * banner quando a vitoria e do modo diario E ja foi registrada no contador
 * compartilhado -- vencer facil/medio/dificil celebra sem creditar nada. */
static void render_banner(void)
{
    if (!s_state.solved) {
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(s_banner_label, "resolvido!");
    lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);

    if (s_state.mode == RATIMOS_SUDOKU_DIARIO && s_state.daily_win_recorded) {
        lv_obj_clear_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void render_board(void)
{
    if (s_generation_failed) {
        lv_obj_add_flag(s_pill_row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_board_wrap, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_keypad, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_gen_fail_container, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(s_gen_fail_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_pill_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_board_wrap, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_keypad, LV_OBJ_FLAG_HIDDEN);

    render_pills();
    render_cells();
    render_banner();
}

static void cell_clicked_cb(lv_event_t * e)
{
    if (s_generation_failed) {
        return;
    }

    lv_obj_t * cell = lv_event_get_target(e);
    uintptr_t packed = (uintptr_t) lv_obj_get_user_data(cell);
    uint8_t row = (uint8_t) (packed / 9u);
    uint8_t col = (uint8_t) (packed % 9u);

    /* Selecionar uma celula nao e uma jogada -- so persiste quando o
     * jogador realmente escreve um digito (ver keypad_value_changed_cb). */
    s_state.cursor_row = row;
    s_state.cursor_col = col;
    render_board();
}

static void keypad_value_changed_cb(lv_event_t * e)
{
    if (s_generation_failed) {
        return;
    }
    if (s_state.cursor_row > 8 || s_state.cursor_col > 8) {
        return; /* nenhuma celula selecionada ainda */
    }

    lv_obj_t * matrix = lv_event_get_target(e);
    uint16_t id = lv_buttonmatrix_get_selected_button(matrix);
    const char * text = lv_buttonmatrix_get_button_text(matrix, id);
    if (!text) {
        return;
    }

    uint8_t digit = (strcmp(text, "apagar") == 0) ? 0 : (uint8_t) (text[0] - '0');

    if (!ratimos_sudoku_set_cell(&s_state, s_state.cursor_row, s_state.cursor_col, digit)) {
        return; /* pista: recusado */
    }

    /* PROGRESSAO-01: credita o castelo EXATAMENTE uma vez por board diario
     * vencido -- o guard `daily_win_recorded` fica dentro do proprio estado
     * persistido, entao reentrar num board diario ja vencido apos um
     * restart nunca soma de novo. Vencer facil/medio/dificil so marca
     * `solved` (banner sem linha de castelo) e nunca chama esta funcao. */
    if (ratimos_sudoku_is_solved(&s_state)) {
        s_state.solved = 1;
        if (s_state.mode == RATIMOS_SUDOKU_DIARIO && !s_state.daily_win_recorded) {
            if (ratimos_storage_record_daily_win(RATIMOS_GAME_SUDOKU, ratimos_daily_index())) {
                s_state.daily_win_recorded = 1;
            }
        }
    }

    persist_state();
    render_board();
}

static void retry_clicked_cb(lv_event_t * e)
{
    (void) e;
    switch_to_mode(s_failed_mode);
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

    if (s_pending_action == SUDOKU_PENDING_RESET) {
        /* So apaga o TABULEIRO salvo -- a progressao ja conquistada (contador
         * compartilhado + desbloqueio exclusivo) nunca regride por causa de
         * um reset manual (proibicao do plano). */
        ratimos_storage_clear_game_state(RATIMOS_GAME_SUDOKU);
        switch_to_mode(s_state.mode);
    } else {
        switch_to_mode(s_pending_mode);
    }
}

static void mode_pill_clicked_cb(lv_event_t * e)
{
    ratimos_sudoku_mode_t target = (ratimos_sudoku_mode_t) (uintptr_t) lv_event_get_user_data(e);

    if (!s_generation_failed && target == s_state.mode) {
        return; /* ja esta nesse modo */
    }

    if (!s_generation_failed && has_progress()) {
        s_pending_action = SUDOKU_PENDING_MODE_SWITCH;
        s_pending_mode = target;
        lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    switch_to_mode(target);
}

/* "novo jogo" (UI-SPEC): disponivel mesmo no meio de uma partida, sempre
 * pede confirmacao (mesma copia destrutiva do Copywriting Contract) antes de
 * descartar o tabuleiro atual, ao contrario da troca de dificuldade que so
 * confirma quando ha progresso do jogador em risco. */
static void novo_jogo_clicked_cb(lv_event_t * e)
{
    (void) e;
    if (s_generation_failed) {
        return;
    }
    s_pending_action = SUDOKU_PENDING_RESET;
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t * make_pill_w(lv_obj_t * parent, const char * text, lv_color_t bg, lv_event_cb_t cb, lv_coord_t width)
{
    lv_obj_t * pill = ratimos_panel_create(parent);
    lv_obj_set_size(pill, width, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(pill, 8, 0);
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

static lv_obj_t * build_sudoku_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("sudoku", "numeros 1-9");

    bool show_load_error = load_or_start_state();

    /* Aviso de save invalido (UI-SPEC, estado de erro): montado uma unica
     * vez na construcao da tela, nunca a cada render. */
    s_error_label = lv_label_create(shell.content);
    lv_label_set_text(s_error_label,
                      "nao foi possivel carregar seu progresso salvo - comecando um jogo novo");
    lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(s_error_label, lv_pct(100));
    lv_label_set_long_mode(s_error_label, LV_LABEL_LONG_MODE_WRAP);
    if (!show_load_error) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Linha de dificuldade/modo -- 4 pilulas, lg(16px) de espaco abaixo. */
    s_pill_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_pill_row);
    lv_obj_set_width(s_pill_row, lv_pct(100));
    lv_obj_set_height(s_pill_row, SUDOKU_PILL_H);
    lv_obj_set_flex_flow(s_pill_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_pill_row, 4, 0);
    lv_obj_set_style_margin_bottom(s_pill_row, 16, 0);
    lv_obj_clear_flag(s_pill_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int m = 0; m < RATIMOS_SUDOKU_MODE_COUNT; m++) {
        lv_obj_t * pill = ratimos_panel_create(s_pill_row);
        lv_obj_set_style_pad_all(pill, 0, 0);
        lv_obj_set_height(pill, SUDOKU_PILL_H);
        lv_obj_set_flex_grow(pill, 1);
        lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(pill, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(pill, mode_pill_clicked_cb, LV_EVENT_CLICKED, (void *) (uintptr_t) m);
        s_pills[m] = pill;

        lv_obj_t * label = lv_label_create(pill);
        lv_label_set_text(label, SUDOKU_MODE_LABELS[m]);
        lv_obj_center(label);
        s_pill_labels[m] = label;
    }

    /* Tabuleiro 9x9 (Grid do LVGL) -- envolto num wrapper de largura total
     * para centralizar o board de 288px dentro do content box de 300px. */
    s_board_wrap = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_board_wrap);
    lv_obj_set_width(s_board_wrap, lv_pct(100));
    lv_obj_set_height(s_board_wrap, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_board_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_board_wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_board_wrap, LV_OBJ_FLAG_SCROLLABLE);

    s_board = lv_obj_create(s_board_wrap);
    lv_obj_remove_style_all(s_board);
    lv_obj_set_size(s_board, SUDOKU_BOARD_PX, SUDOKU_BOARD_PX);
    lv_obj_set_layout(s_board, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(s_board, SUDOKU_GRID_DSC, SUDOKU_GRID_DSC);
    lv_obj_clear_flag(s_board, LV_OBJ_FLAG_SCROLLABLE);

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            lv_obj_t * cell = lv_obj_create(s_board);
            lv_obj_remove_style_all(cell);
            lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_PANEL, 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(cell, 1, 0);
            lv_obj_set_style_border_color(cell, RATIMOS_COLOR_PANEL_ACTIVE, 0);
            lv_obj_set_style_radius(cell, 0, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, c, 1, LV_GRID_ALIGN_STRETCH, r, 1);
            lv_obj_set_user_data(cell, (void *) (uintptr_t) (r * 9 + c));
            lv_obj_add_event_cb(cell, cell_clicked_cb, LV_EVENT_CLICKED, NULL);
            s_cells[r][c] = cell;

            lv_obj_t * label = lv_label_create(cell);
            lv_label_set_text(label, "");
            lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
            lv_obj_center(label);
            s_cell_labels[r][c] = label;
        }
    }

    /* Teclado numerico -- 10 teclas (1-9 + apagar) num lv_buttonmatrix.
     * Comentario de risco flagueado (UI-SPEC/RESEARCH Pitfall 3): ~30px de
     * largura por tecla fica abaixo do alvo de toque ideal de 44px porque
     * dez colunas precisam caber num painel de 300px -- risco aceito e
     * explicitamente sinalizado; a confirmacao real com dedo/stylus fica
     * pra verificacao de hardware da Fase 3, ja que o mouse do native_sim
     * nao revela problema de alvo de toque pequeno. */
    s_keypad = lv_buttonmatrix_create(shell.content);
    lv_buttonmatrix_set_map(s_keypad, SUDOKU_KEYPAD_MAP);
    lv_obj_set_width(s_keypad, lv_pct(100));
    lv_obj_set_height(s_keypad, SUDOKU_KEYPAD_H);
    lv_obj_set_style_pad_column(s_keypad, 4, 0);
    lv_obj_set_style_pad_row(s_keypad, 4, 0);
    lv_obj_set_style_bg_opa(s_keypad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_keypad, 0, 0);
    lv_obj_set_style_bg_color(s_keypad, RATIMOS_COLOR_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_keypad, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_keypad, RATIMOS_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_keypad, 4, LV_PART_ITEMS);
    lv_obj_add_event_cb(s_keypad, keypad_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Banner de vitoria (Display-tier, UI-SPEC): "resolvido!" sempre que
     * s_state.solved; "+1 no castelo" so junto quando a vitoria diaria ja
     * foi creditada. Escondidos ate render_banner() decidir. */
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

    /* "novo jogo" (UI-SPEC): disponivel o tempo todo, nao so apos vencer. */
    lv_obj_t * novo_jogo_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(novo_jogo_row);
    lv_obj_set_width(novo_jogo_row, lv_pct(100));
    lv_obj_set_height(novo_jogo_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(novo_jogo_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(novo_jogo_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(novo_jogo_row, LV_OBJ_FLAG_SCROLLABLE);
    make_pill_w(novo_jogo_row, "novo jogo", RATIMOS_COLOR_PANEL, novo_jogo_clicked_cb, 140);

    /* Falha de geracao (UI-SPEC, estado de erro): nenhum tabuleiro, so a
     * copia de erro + pilula de retry. Escondido ate render_board() decidir
     * que essa e a situacao. */
    s_gen_fail_container = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_gen_fail_container);
    lv_obj_set_width(s_gen_fail_container, lv_pct(100));
    lv_obj_set_height(s_gen_fail_container, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_gen_fail_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_gen_fail_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_gen_fail_container, 12, 0);
    lv_obj_clear_flag(s_gen_fail_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * fail_label = lv_label_create(s_gen_fail_container);
    lv_label_set_text(fail_label, "nao foi possivel gerar o quebra-cabeca - tente novamente");
    lv_obj_set_width(fail_label, lv_pct(100));
    lv_label_set_long_mode(fail_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(fail_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(fail_label, RATIMOS_COLOR_TEXT_MUTED, 0);

    make_pill_w(s_gen_fail_container, "tentar novamente", RATIMOS_COLOR_ACCENT, retry_clicked_cb, 160);

    /* Dialogo de confirmacao destrutiva (Copywriting Contract): montado uma
     * unica vez, escondido ate uma troca de modo com progresso em curso ser
     * tocada. Filho de shell.screen (nao de shell.content) para flutuar por
     * cima do resto da tela, igual a conexo.c. */
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

    render_board();

    return shell.screen;
}

void ratimos_sudoku_show(lv_event_t * e)
{
    (void) e;
    if (!s_sudoku_screen) {
        s_sudoku_screen = build_sudoku_screen();
    }
    lv_screen_load(s_sudoku_screen);
}
