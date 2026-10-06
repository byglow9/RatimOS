/*
 * Termo/Dueto/Quarteto (JOGOS-03) -- tela.
 *
 * Persistencia (JOGOS-02): toda tecla que muda current_guess, todo palpite
 * submetido e toda troca de modo grava na hora via
 * ratimos_storage_save_game_state() -- mesma disciplina de sudoku.c/conexo.c.
 * Este arquivo NAO abre arquivo nenhum: todo byte que chega ao disco passa
 * pela Storage/Content API (D-10).
 *
 * As 3 modalidades (Termo/Dueto/Quarteto) compartilham UM UNICO slot de save
 * (RATIMOS_GAME_TERMO) -- trocar de modo substitui a sessao ativa por uma
 * nova sessao diaria do modo escolhido, igual ao modelo de troca de
 * dificuldade do sudoku.c. Por isso uma troca com progresso em curso passa
 * pelo dialogo de confirmacao destrutiva.
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "termo.h"
#include "termo_engine.h"

#include "../../app_shell.h"
#include "../../status_bar.h"
#include "../../theme.h"
#include "../../fonts/ratimos_fonts.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

#define TERMO_PILL_H     28
#define TERMO_TILE_GAP   2
#define TERMO_PANEL_PAD  4
#define TERMO_KEYBOARD_H 110

/* Tamanho de tile e linhas visiveis por modo, per UI-SPEC "Game Screen
 * Layouts" > Termo/Dueto/Quarteto. Quarteto usa um viewport de 5 linhas
 * (scrollavel) mesmo com ate 9 tentativas -- nove linhas em tamanho legivel
 * nao cabem no orcamento vertical (UI-SPEC, resolvido). */
static const uint8_t TERMO_TILE_PX[RATIMOS_TERMO_MODE_COUNT] = { 40, 26, 22 };
static const uint8_t TERMO_VISIBLE_ROWS[RATIMOS_TERMO_MODE_COUNT] = { 6, 7, 5 };

static const char * const TERMO_MODE_LABELS[RATIMOS_TERMO_MODE_COUNT] = {
    "termo", "dueto", "quarteto"
};

/* Buffer do caminho de breadcrumb da sectionbar ("./home/jogos/quarteto" e'
 * o mais longo, 21 chars + NUL). */
#define TERMO_PATH_BUF_LEN 32

/* Teclado compartilhado -- layout QWERTY de 3 linhas com enter/apagar na
 * ultima linha (convencao Wordle/term.ooo padrao). Um unico
 * lv_buttonmatrix, neutro, nunca dividido nem colorido por board (UI-SPEC:
 * resolvido a favor da legibilidade num painel de 320px). */
static const char * const TERMO_KEYBOARD_MAP[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "enter", "z", "x", "c", "v", "b", "n", "m", "apagar", NULL
};
/* 3a linha (plano 02.1-13): "enter" e "apagar" com 3 unidades cada -- em
 * largura igual o texto deles estourava a tecla, e com 2 unidades ainda
 * encostava na moldura bevel. */
static const lv_buttonmatrix_ctrl_t TERMO_KEYBOARD_CTRL[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    3, 1, 1, 1, 1, 1, 1, 1, 3
};

/* ------------------------------------------------------------------------
 * Tela (LVGL) -- cache-once, igual a sudoku.c/conexo.c. Todos os 4 boards x
 * 9 linhas x 5 colunas sao construidos UMA vez (o maximo que qualquer modo
 * usa); trocar de modo so mostra/esconde/redimensiona, nunca reconstroi.
 * ------------------------------------------------------------------------ */

static lv_obj_t * s_termo_screen = NULL;

static lv_obj_t * s_section_title_label = NULL;
static lv_obj_t * s_error_label = NULL;
static lv_obj_t * s_pill_row = NULL;
static lv_obj_t * s_pills[RATIMOS_TERMO_MODE_COUNT];
static lv_obj_t * s_status_label = NULL;
static lv_obj_t * s_boards_wrap = NULL;
static lv_obj_t * s_board_panel[RATIMOS_TERMO_MAX_BOARDS];
static lv_obj_t * s_row_wrap[RATIMOS_TERMO_MAX_BOARDS][RATIMOS_TERMO_MAX_TRIES];
static lv_obj_t * s_tile[RATIMOS_TERMO_MAX_BOARDS][RATIMOS_TERMO_MAX_TRIES][RATIMOS_TERMO_WORD_LEN];
static lv_obj_t * s_tile_label[RATIMOS_TERMO_MAX_BOARDS][RATIMOS_TERMO_MAX_TRIES][RATIMOS_TERMO_WORD_LEN];
static lv_obj_t * s_keyboard = NULL;
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_reveal_label = NULL;
static lv_obj_t * s_confirm_overlay = NULL;

static ratimos_termo_state_t s_state;
static ratimos_termo_mode_t s_pending_mode = RATIMOS_TERMO_MODE_TERMO;

/* Verdadeiro por um render depois de um palpite rejeitado -- engrossa a
 * borda da linha atual em vez de limpar o que o jogador digitou (UI-SPEC:
 * "flashes its outline ... rather than clearing the player's typing"). */
static bool s_reject_flash = false;

static void render_all(void);
static void render_boards(void);
static void persist_state(void);
static void switch_to_mode(ratimos_termo_mode_t mode);

static lv_coord_t termo_board_width(ratimos_termo_mode_t mode)
{
    lv_coord_t tile = TERMO_TILE_PX[mode];
    return (lv_coord_t) (RATIMOS_TERMO_WORD_LEN * tile
                          + (RATIMOS_TERMO_WORD_LEN - 1) * TERMO_TILE_GAP
                          + 2 * TERMO_PANEL_PAD);
}

static lv_coord_t termo_board_height(ratimos_termo_mode_t mode)
{
    lv_coord_t tile = TERMO_TILE_PX[mode];
    uint8_t rows = TERMO_VISIBLE_ROWS[mode];
    return (lv_coord_t) (rows * tile + (rows - 1) * TERMO_TILE_GAP + 2 * TERMO_PANEL_PAD);
}

static void persist_state(void)
{
    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &s_state, sizeof(s_state));
    blob.used = sizeof(s_state);

    ratimos_storage_save_game_state(RATIMOS_GAME_TERMO, &blob);
}

/* Decide o estado inicial: restaura um save valido, comeca o modo termo
 * quando nao ha save, ou comeca limpo COM aviso quando o save esta
 * corrompido. Mitigacao T-02.1-02: `mode` fora de RATIMOS_TERMO_MODE_COUNT
 * ou `finished` fora de {0,1,2} e tratado como save invalido; board_count e
 * max_tries NUNCA sao confiados do disco -- sempre recalculados a partir da
 * tabela de modos, e tries_used e limitado ao max_tries recalculado. */
static bool load_or_start_state(void)
{
    ratimos_game_state_t blob;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_TERMO, &blob);

    if (status == RATIMOS_GAME_STATE_OK && blob.used == sizeof(s_state)) {
        ratimos_termo_state_t restored;
        memcpy(&restored, blob.bytes, sizeof(restored));

        bool valid = restored.mode < RATIMOS_TERMO_MODE_COUNT && restored.finished <= 2;

        if (valid) {
            restored.board_count = ratimos_termo_board_count(restored.mode);
            restored.max_tries = ratimos_termo_max_tries(restored.mode);
            if (restored.tries_used > restored.max_tries) {
                restored.tries_used = restored.max_tries;
            }
            s_state = restored;
            status = RATIMOS_GAME_STATE_OK;
        } else {
            status = RATIMOS_GAME_STATE_INVALID;
        }
    } else if (status == RATIMOS_GAME_STATE_OK) {
        status = RATIMOS_GAME_STATE_INVALID; /* tamanho do blob nao bate com esta versao */
    }

    if (status == RATIMOS_GAME_STATE_OK) {
        return false; /* restaurado, sem aviso de erro */
    }

    /* ABSENT ou INVALID: comeca a sessao diaria do modo termo (UI-SPEC nao
     * define um modo padrao explicito para o caso "sem save" -- termo e o
     * primeiro/mais simples dos 3, escolha natural). */
    ratimos_termo_start_daily(&s_state, RATIMOS_TERMO_MODE_TERMO, ratimos_daily_index());
    persist_state();

    return status == RATIMOS_GAME_STATE_INVALID;
}

/* Monta o caminho de breadcrumb da sectionbar para o modo dado
 * ("./home/jogos/termo", "./home/jogos/dueto", "./home/jogos/quarteto") --
 * unico lugar que formata esse caminho, usado na construcao da tela e na
 * troca de modo. */
static void format_termo_path(char * buf, size_t buf_size, ratimos_termo_mode_t mode)
{
    lv_snprintf(buf, buf_size, "./home/jogos/%s", TERMO_MODE_LABELS[mode]);
}

static void switch_to_mode(ratimos_termo_mode_t mode)
{
    ratimos_termo_start_daily(&s_state, mode, ratimos_daily_index());
    persist_state();
    /* Passa pelo helper compartilhado (status_bar.c), nunca por
     * lv_label_set_text bruto -- senao o split de cor muted/atual do
     * breadcrumb se perde depois da troca de modo. */
    char path[TERMO_PATH_BUF_LEN];
    format_termo_path(path, sizeof(path), mode);
    ratimos_sectionbar_set_path(s_section_title_label, path);
    render_all();
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
    switch_to_mode(s_pending_mode);
}

static void mode_pill_clicked_cb(lv_event_t * e)
{
    ratimos_termo_mode_t target = (ratimos_termo_mode_t) (uintptr_t) lv_event_get_user_data(e);

    if (target == s_state.mode) {
        return; /* ja esta nesse modo */
    }

    if (s_state.tries_used > 0) {
        s_pending_mode = target;
        ratimos_modal_show(s_confirm_overlay);
        return;
    }

    switch_to_mode(target);
}

/* Selecao so' troca fundo + cor do texto (ratimos_button_set_selected) --
 * a moldura bevel 003-C fica intacta em qualquer estado (plano 02.1-13). */
static void render_pills(void)
{
    for (int m = 0; m < RATIMOS_TERMO_MODE_COUNT; m++) {
        ratimos_button_set_selected(s_pills[m], (int) s_state.mode == m);
    }
}

static void render_status(void)
{
    if (s_state.finished != 0) {
        lv_obj_add_flag(s_status_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(s_status_label, LV_OBJ_FLAG_HIDDEN);

    unsigned attempt = (unsigned) s_state.tries_used + 1;
    if (attempt > s_state.max_tries) {
        attempt = s_state.max_tries;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "tentativa %u de %u", attempt, (unsigned) s_state.max_tries);
    lv_label_set_text(s_status_label, buf);
}

/* Banner Display-tier (UI-SPEC): "acertou!" quando finished==1 (com "+1 no
 * castelo" so quando a vitoria diaria ja foi creditada), "nao foi dessa
 * vez" quando finished==2 -- copia calorosa, nunca punitiva, seguida da
 * revelacao das respostas ainda nao resolvidas em texto silenciado. */
static void render_banner(void)
{
    if (s_state.finished == 0) {
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_reveal_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (s_state.finished == 1) {
        lv_label_set_text(s_banner_label, "acertou!");
        lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_reveal_label, LV_OBJ_FLAG_HIDDEN);

        if (s_state.daily_win_recorded) {
            lv_obj_clear_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    lv_label_set_text(s_banner_label, "nao foi dessa vez");
    lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);

    char buf[160];
    size_t offset = 0;
    int written = snprintf(buf, sizeof(buf), "respostas: ");
    offset = (written > 0) ? (size_t) written : 0;

    bool first = true;
    for (uint8_t b = 0; b < s_state.board_count && offset < sizeof(buf); b++) {
        if (s_state.boards[b].solved) {
            continue;
        }
        written = snprintf(buf + offset, sizeof(buf) - offset, "%s%s",
                            first ? "" : ", ", s_state.boards[b].answer);
        if (written > 0) {
            offset += (size_t) written;
        }
        first = false;
    }

    lv_label_set_text(s_reveal_label, buf);
    lv_obj_clear_flag(s_reveal_label, LV_OBJ_FLAG_HIDDEN);
}

/* Redesenha todos os boards para o modo ativo -- mostra/esconde boards e
 * linhas, redimensiona tiles, e classifica cada linha como historico
 * (feedback ja calculado), atual (outline de acento, texto sendo
 * digitado), congelada-vazia (board ja resolvido, opacidade reduzida) ou
 * futura (em branco, neutra). */
static void render_boards(void)
{
    ratimos_termo_mode_t mode = s_state.mode;
    lv_coord_t tile = TERMO_TILE_PX[mode];
    lv_coord_t board_w = termo_board_width(mode);
    lv_coord_t board_h = termo_board_height(mode);
    bool scroll_needed = (mode == RATIMOS_TERMO_MODE_QUARTETO);

    for (uint8_t b = 0; b < RATIMOS_TERMO_MAX_BOARDS; b++) {
        if (b >= s_state.board_count) {
            lv_obj_add_flag(s_board_panel[b], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(s_board_panel[b], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(s_board_panel[b], board_w, board_h);

        if (scroll_needed) {
            lv_obj_add_flag(s_board_panel[b], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scroll_dir(s_board_panel[b], LV_DIR_VER);
        } else {
            lv_obj_clear_flag(s_board_panel[b], LV_OBJ_FLAG_SCROLLABLE);
        }

        const ratimos_termo_board_t * board = &s_state.boards[b];

        for (uint8_t r = 0; r < RATIMOS_TERMO_MAX_TRIES; r++) {
            if (r >= s_state.max_tries) {
                lv_obj_add_flag(s_row_wrap[b][r], LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            lv_obj_clear_flag(s_row_wrap[b][r], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_pad_column(s_row_wrap[b][r], TERMO_TILE_GAP, 0);
            lv_obj_set_height(s_row_wrap[b][r], tile);
            lv_obj_set_style_opa(s_row_wrap[b][r], LV_OPA_COVER, 0);

            bool is_history_row = (r < board->guesses_made);
            bool is_current_row = (!board->solved && r == s_state.tries_used && s_state.finished == 0);

            if (!is_history_row && board->solved) {
                lv_obj_set_style_opa(s_row_wrap[b][r], LV_OPA_50, 0);
            }

            for (uint8_t c = 0; c < RATIMOS_TERMO_WORD_LEN; c++) {
                lv_obj_t * cell = s_tile[b][r][c];
                lv_obj_t * label = s_tile_label[b][r][c];
                lv_obj_set_size(cell, tile, tile);

                char ch = 0;
                lv_color_t bg = RATIMOS_COLOR_PANEL;
                lv_color_t border = RATIMOS_COLOR_PANEL_ACTIVE;
                int border_w = 1;

                if (is_history_row) {
                    ch = s_state.history[r][c];
                    uint8_t fb = board->feedback[r][c];
                    if (fb == RATIMOS_TERMO_FEEDBACK_CORRECT) {
                        bg = RATIMOS_COLOR_GAME_CORRECT;
                    } else if (fb == RATIMOS_TERMO_FEEDBACK_PRESENT) {
                        bg = RATIMOS_COLOR_GAME_PRESENT;
                    } else {
                        bg = RATIMOS_COLOR_GAME_ABSENT;
                    }
                    border = bg;
                } else if (is_current_row) {
                    size_t guess_len = strlen(s_state.current_guess);
                    if (c < guess_len) {
                        ch = s_state.current_guess[c];
                    }
                    border = RATIMOS_COLOR_ACCENT;
                    border_w = s_reject_flash ? 3 : 2;
                }

                char buf[2] = { 0, 0 };
                if (ch != 0) {
                    buf[0] = (char) toupper((unsigned char) ch);
                }
                lv_label_set_text(label, buf);

                lv_obj_set_style_bg_color(cell, bg, 0);
                lv_obj_set_style_border_color(cell, border, 0);
                lv_obj_set_style_border_width(cell, border_w, 0);
            }
        }
    }

    if (scroll_needed) {
        for (uint8_t b = 0; b < s_state.board_count; b++) {
            /* Rola ate a linha mais recente -- LVGL limita ao maximo real
             * de rolagem, entao um valor grande "vai ate o fim" sem
             * precisar calcular a altura exata do conteudo. */
            lv_obj_scroll_to_y(s_board_panel[b], 10000, LV_ANIM_OFF);
        }
    }
}

static void render_all(void)
{
    render_pills();
    render_status();
    render_boards();
    render_banner();
}

static void try_submit(void)
{
    bool ok = ratimos_termo_submit(&s_state, s_state.current_guess);
    s_reject_flash = !ok;

    if (ok) {
        memset(s_state.current_guess, 0, sizeof(s_state.current_guess));

        /* PROGRESSAO-01: credita o castelo EXATAMENTE uma vez -- o guard
         * `daily_win_recorded` fica dentro do proprio estado persistido,
         * entao reentrar numa sessao ja vencida apos um restart nunca soma
         * de novo. Uma troca de modo ou um "nao foi dessa vez" nunca chega
         * aqui. */
        if (s_state.finished == 1 && !s_state.daily_win_recorded) {
            if (ratimos_storage_record_daily_win(RATIMOS_GAME_TERMO, ratimos_daily_index())) {
                s_state.daily_win_recorded = 1;
            }
        }
    }

    persist_state();
    render_all();
}

static void keyboard_value_changed_cb(lv_event_t * e)
{
    if (s_state.finished != 0) {
        return;
    }

    lv_obj_t * matrix = lv_event_get_target(e);
    uint16_t id = lv_buttonmatrix_get_selected_button(matrix);
    const char * text = lv_buttonmatrix_get_button_text(matrix, id);
    if (!text) {
        return;
    }

    if (strcmp(text, "enter") == 0) {
        try_submit();
        return;
    }

    if (strcmp(text, "apagar") == 0) {
        size_t len = strlen(s_state.current_guess);
        if (len > 0) {
            s_state.current_guess[len - 1] = '\0';
            s_reject_flash = false;
            persist_state();
            render_boards();
        }
        return;
    }

    size_t len = strlen(s_state.current_guess);
    if (len < RATIMOS_TERMO_WORD_LEN) {
        s_state.current_guess[len] = text[0];
        s_state.current_guess[len + 1] = '\0';
        s_reject_flash = false;
        persist_state();
        render_boards();
    }
}

static lv_obj_t * build_termo_screen(void)
{
    bool show_load_error = load_or_start_state();

    char initial_path[TERMO_PATH_BUF_LEN];
    format_termo_path(initial_path, sizeof(initial_path), s_state.mode);
    ratimos_app_shell_t shell = ratimos_app_shell_create(initial_path, "digite uma palavra");

    /* app_shell.h/status_bar.h nao expoe um handle pro label de titulo da
     * sectionbar (nenhum app antes de termo precisava mudar o titulo
     * depois de construido) -- pega pela posicao estrutural conhecida em
     * vez de mudar a API compartilhada por um unico chamador. Filhos de
     * shell.screen (ver app_shell.c): 0 = imagem de fundo ditherizada
     * (ratimos_theme_apply_screen(), plano 02.1-09), 1 = topbar,
     * 2 = linha da sectionbar, 3 = content, 4 = bottombar. A linha da
     * sectionbar tem um UNICO filho, o label do caminho recolorido (plano
     * 02.1-10) -- ver ratimos_sectionbar_create() em status_bar.c. */
    lv_obj_t * sectionbar_row = lv_obj_get_child(shell.screen, 2);
    s_section_title_label = lv_obj_get_child(sectionbar_row, 0);

    s_error_label = lv_label_create(shell.content);
    lv_label_set_text(s_error_label,
                      "nao foi possivel carregar seu progresso salvo - comecando um jogo novo");
    lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(s_error_label, lv_pct(100));
    lv_label_set_long_mode(s_error_label, LV_LABEL_LONG_MODE_WRAP);
    if (!show_load_error) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Seletor de modo -- 3 pilulas, mesma linguagem visual das pilulas de
     * dificuldade do sudoku.c. */
    s_pill_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_pill_row);
    lv_obj_set_width(s_pill_row, lv_pct(100));
    lv_obj_set_height(s_pill_row, TERMO_PILL_H);
    lv_obj_set_flex_flow(s_pill_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_pill_row, 4, 0);
    lv_obj_clear_flag(s_pill_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int m = 0; m < RATIMOS_TERMO_MODE_COUNT; m++) {
        /* Bevel 003-C compartilhado (plano 02.1-13); largura vem do
         * flex_grow, entao sem pad horizontal. cb registrado aqui pra
         * carregar o indice do modo como user_data. */
        lv_obj_t * pill = ratimos_button_create(s_pill_row, TERMO_MODE_LABELS[m], NULL, LV_SIZE_CONTENT, TERMO_PILL_H);
        lv_obj_set_style_pad_hor(pill, 0, 0);
        lv_obj_set_flex_grow(pill, 1);
        lv_obj_add_event_cb(pill, mode_pill_clicked_cb, LV_EVENT_CLICKED, (void *) (uintptr_t) m);
        s_pills[m] = pill;
    }

    /* Status "tentativa X de Y". */
    s_status_label = lv_label_create(shell.content);
    lv_label_set_text(s_status_label, "");
    lv_obj_set_width(s_status_label, lv_pct(100));
    lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_status_label, RATIMOS_COLOR_TEXT_MUTED, 0);

    /* Boards -- ate 4 paineis num wrap centralizado; render_boards() decide
     * quantos ficam visiveis e o tamanho de cada um por modo. */
    s_boards_wrap = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_boards_wrap);
    lv_obj_set_width(s_boards_wrap, lv_pct(100));
    lv_obj_set_height(s_boards_wrap, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_boards_wrap, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_boards_wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_boards_wrap, 8, 0);
    lv_obj_set_style_pad_column(s_boards_wrap, 8, 0);
    lv_obj_clear_flag(s_boards_wrap, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t b = 0; b < RATIMOS_TERMO_MAX_BOARDS; b++) {
        lv_obj_t * panel = ratimos_panel_create(s_boards_wrap);
        lv_obj_set_style_pad_all(panel, TERMO_PANEL_PAD, 0);
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(panel, TERMO_TILE_GAP, 0);
        s_board_panel[b] = panel;

        for (uint8_t r = 0; r < RATIMOS_TERMO_MAX_TRIES; r++) {
            lv_obj_t * row = lv_obj_create(panel);
            lv_obj_remove_style_all(row);
            lv_obj_set_width(row, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            s_row_wrap[b][r] = row;

            for (uint8_t c = 0; c < RATIMOS_TERMO_WORD_LEN; c++) {
                lv_obj_t * cell = lv_obj_create(row);
                lv_obj_remove_style_all(cell);
                lv_obj_set_style_bg_color(cell, RATIMOS_COLOR_PANEL, 0);
                lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(cell, 1, 0);
                lv_obj_set_style_border_color(cell, RATIMOS_COLOR_PANEL_ACTIVE, 0);
                lv_obj_set_style_radius(cell, 0, 0);
                lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
                s_tile[b][r][c] = cell;

                lv_obj_t * label = lv_label_create(cell);
                lv_label_set_text(label, "");
                lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
                lv_obj_center(label);
                s_tile_label[b][r][c] = label;
            }
        }
    }

    /* Teclado compartilhado -- neutro, nunca dividido/colorido por board
     * (UI-SPEC). Risco flagueado (UI-SPEC/RESEARCH Pitfall 3): ~28-30px de
     * largura por tecla fica abaixo do alvo de toque ideal de 44px porque
     * dez colunas precisam caber num painel de 300px -- risco aceito e
     * explicitamente sinalizado; confirmacao real com dedo/stylus fica pra
     * verificacao de hardware da Fase 3. */
    s_keyboard = lv_buttonmatrix_create(shell.content);
    lv_buttonmatrix_set_map(s_keyboard, TERMO_KEYBOARD_MAP);
    lv_buttonmatrix_set_ctrl_map(s_keyboard, TERMO_KEYBOARD_CTRL);
    lv_obj_set_width(s_keyboard, lv_pct(100));
    lv_obj_set_height(s_keyboard, TERMO_KEYBOARD_H);
    lv_obj_set_style_pad_column(s_keyboard, 2, 0);
    lv_obj_set_style_pad_row(s_keyboard, 2, 0);
    ratimos_bevel_style_buttonmatrix(s_keyboard);
    lv_obj_add_event_cb(s_keyboard, keyboard_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Banner de vitoria/derrota (Display-tier, UI-SPEC). */
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

    s_reveal_label = lv_label_create(shell.content);
    lv_label_set_text(s_reveal_label, "");
    lv_obj_set_width(s_reveal_label, lv_pct(100));
    lv_label_set_long_mode(s_reveal_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(s_reveal_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_reveal_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_add_flag(s_reveal_label, LV_OBJ_FLAG_HIDDEN);

    /* Dialogo de confirmacao destrutiva (Copywriting Contract) -- so
     * aparece numa troca de modo com progresso em curso. Modal
     * compartilhado centralizado na tela (ratimos_modal_create, theme.h). */
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

void ratimos_termo_show(lv_event_t * e)
{
    (void) e;
    if (!s_termo_screen) {
        s_termo_screen = build_termo_screen();
    }
    lv_screen_load(s_termo_screen);
}
