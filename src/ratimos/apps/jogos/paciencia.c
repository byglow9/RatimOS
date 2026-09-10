/*
 * Paciencia / Klondike (JOGOS-01) -- tela.
 *
 * Interacao: toque-para-selecionar-depois-toque-para-mover, NAO
 * arrastar-e-soltar (LVGL drag num painel de 320x480 com cartas em
 * cascata sobrepostas e traicoeiro; o modelo de toque mapeia direto no
 * vocabulario explicito de jogadas do motor). A dica do rodape diz
 * "toque para mover" -- reflete a interacao real, nao a copia original
 * do UI-SPEC ("arraste as cartas"), que prometia um gesto que nao existe.
 *
 * Persistencia (JOGOS-02): toda jogada aceita grava na hora via
 * ratimos_storage_save_game_state() -- igual a sudoku.c/conexo.c, salvar
 * so ao sair perderia progresso num fechamento inesperado.
 *
 * A tela NUNCA mexe numa pilha diretamente -- toda mutacao passa por
 * ratimos_klondike_move() (T-02.1-14). Este arquivo tambem nao abre
 * arquivo nenhum: todo byte que chega ao disco passa pela Storage/Content
 * API (D-10).
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "paciencia.h"
#include "klondike_engine.h"

#include "../../app_shell.h"
#include "../../theme.h"
#include "../../fonts/ratimos_fonts.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

#define PACIENCIA_TOP_CARD_W      40
#define PACIENCIA_TOP_CARD_H      56
#define PACIENCIA_TABLEAU_CARD_W  38
#define PACIENCIA_TABLEAU_CARD_H  54
#define PACIENCIA_CASCADE_STEP    16
#define PACIENCIA_XS_GAP          4

/* Sentinela usada no user_data empacotado de uma carta do tableau para
 * dizer "isto e o slot vazio da coluna, nao uma carta real". */
#define PACIENCIA_TABLEAU_SLOT_IDX 0xFFu

/* ------------------------------------------------------------------------
 * Tela (LVGL) -- cache-once, igual a sudoku.c/conexo.c.
 *
 * A tela e construida UMA vez e guardada em s_paciencia_screen; toda
 * jogada apenas re-renderiza os containers de cada pilha via render_all(),
 * nunca reconstroi a tela inteira -- reconstruir a cada visita e o
 * vazamento de heap do LVGL que a Fase 1 ja corrigiu.
 * ------------------------------------------------------------------------ */

static lv_obj_t * s_paciencia_screen = NULL;

static lv_obj_t * s_error_label = NULL;
static lv_obj_t * s_stock_slot = NULL;
static lv_obj_t * s_waste_slot = NULL;
static lv_obj_t * s_foundation_slots[RATIMOS_KLONDIKE_FOUNDATIONS];
static lv_obj_t * s_tableau_cols[RATIMOS_KLONDIKE_TABLEAU_COLS];
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_confirm_overlay = NULL;

static ratimos_klondike_state_t s_state;

/* Selecao ativa do modelo toque-para-selecionar-depois-toque-para-mover.
 * O descarte nunca e destino de jogada (so origem) -- so estoque/tableau/
 * fundacao entram aqui como possiveis origens. */
typedef enum {
    PACIENCIA_SEL_NONE = 0,
    PACIENCIA_SEL_TABLEAU,
    PACIENCIA_SEL_WASTE,
    PACIENCIA_SEL_FOUNDATION
} paciencia_selection_kind_t;

static paciencia_selection_kind_t s_sel_kind = PACIENCIA_SEL_NONE;
static uint8_t s_sel_index = 0; /* coluna do tableau ou indice da fundacao */
static uint8_t s_sel_count = 1; /* tamanho da sequencia selecionada (so relevante pro tableau) */

/* Pisca a borda da carta de origem quando uma jogada e recusada (RESEARCH
 * "Don't Hand-Roll": a recusa tem que ser visivel, nunca silenciosa). */
static bool s_flash_active = false;
static paciencia_selection_kind_t s_flash_kind = PACIENCIA_SEL_NONE;
static uint8_t s_flash_index = 0;
static lv_timer_t * s_flash_timer = NULL;

static void render_all(void);
static void persist_state(void);

/* Semente "fresh" para um redeal manual ("novo jogo", sempre nao-diario) --
 * mesma tecnica do sudoku.c: sem RTC no native_sim, o relogio do host + um
 * contador local bastam pra nao repetir o mesmo deal em resets
 * consecutivos dentro do mesmo segundo. O deal diario NUNCA usa isto --
 * sempre ratimos_daily_seed(). */
static uint32_t fresh_seed(void)
{
    static uint32_t counter = 0;
    counter++;
    return (uint32_t) time(NULL) ^ (counter * 2654435761u);
}

/* ------------------------------------------------------------------------
 * Renderizacao de uma carta -- helpers puramente visuais, sem logica de
 * jogo. Cada um cria um filho novo dentro de `parent`, que o chamador ja
 * limpou via lv_obj_clean() antes de reconstruir.
 * ------------------------------------------------------------------------ */

static void card_label_text(ratimos_card_t card, char * buf, size_t buf_size)
{
    static const char * const RANKS[14] = {
        "?", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
    };
    static const char SUIT_CHARS[4] = { 'P', 'O', 'C', 'E' }; /* paus, ouros, copas, espadas */

    uint8_t rank = ratimos_card_rank(card);
    uint8_t suit = ratimos_card_suit(card);
    const char * rank_str = (rank >= 1 && rank <= 13) ? RANKS[rank] : "?";
    char suit_ch = (suit < 4) ? SUIT_CHARS[suit] : '?';

    snprintf(buf, buf_size, "%s\n%c", rank_str, suit_ch);
}

static lv_obj_t * make_card_outline(lv_obj_t * parent, int16_t w, int16_t h)
{
    lv_obj_t * box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, RATIMOS_COLOR_PANEL_ACTIVE, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static lv_obj_t * make_card_back(lv_obj_t * parent, int16_t w, int16_t h)
{
    lv_obj_t * box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_style_bg_color(box, RATIMOS_COLOR_PANEL_ACTIVE, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static lv_obj_t * make_card_face(lv_obj_t * parent, ratimos_card_t card, int16_t w, int16_t h, bool highlighted)
{
    lv_obj_t * box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_style_bg_color(box, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, highlighted ? 2 : 1, 0);
    lv_obj_set_style_border_color(box, highlighted ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_PANEL_ACTIVE, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    char buf[8];
    card_label_text(card, buf, sizeof(buf));
    lv_obj_t * label = lv_label_create(box);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, ratimos_card_is_red(card) ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_TEXT, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 2, 2);

    return box;
}

/* ------------------------------------------------------------------------
 * Selecao / jogada
 * ------------------------------------------------------------------------ */

static void clear_selection(void)
{
    s_sel_kind = PACIENCIA_SEL_NONE;
    s_sel_index = 0;
    s_sel_count = 1;
}

static void flash_timer_cb(lv_timer_t * timer)
{
    (void) timer;
    s_flash_active = false;
    s_flash_timer = NULL;
    render_all();
}

static void begin_flash(paciencia_selection_kind_t kind, uint8_t index)
{
    s_flash_active = true;
    s_flash_kind = kind;
    s_flash_index = index;
    if (s_flash_timer) {
        lv_timer_del(s_flash_timer);
    }
    s_flash_timer = lv_timer_create(flash_timer_cb, 250, NULL);
    lv_timer_set_repeat_count(s_flash_timer, 1);
}

/* PROGRESSAO-01: credita o castelo EXATAMENTE uma vez por deal diario
 * vencido -- o guard `daily_win_recorded` fica dentro do proprio estado
 * persistido, entao reentrar num deal diario ja vencido apos um restart
 * nunca soma de novo. Vencer um deal nao-diario (via "novo jogo") so marca
 * `won` (banner sem linha de castelo) e nunca chama esta funcao com
 * efeito. */
static void handle_win_credit_if_needed(void)
{
    if (s_state.won && s_state.daily && !s_state.daily_win_recorded) {
        if (ratimos_storage_record_daily_win(RATIMOS_GAME_PACIENCIA, ratimos_daily_index())) {
            s_state.daily_win_recorded = 1;
        }
    }
}

static void apply_move_result(bool moved)
{
    if (moved) {
        clear_selection();
        handle_win_credit_if_needed();
        persist_state();
    } else {
        /* Jogada ilegal -- recusa visivel (pisca a borda da origem) em vez
         * de aceitar em silencio. */
        begin_flash(s_sel_kind, s_sel_index);
        clear_selection();
    }
    render_all();
}

static void attempt_move_to_tableau(uint8_t to_col)
{
    bool moved = false;
    switch (s_sel_kind) {
        case PACIENCIA_SEL_TABLEAU:
            moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_TABLEAU, s_sel_index, to_col,
                                           s_sel_count);
            break;
        case PACIENCIA_SEL_WASTE:
            moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_WASTE_TO_TABLEAU, 0, to_col, 1);
            break;
        case PACIENCIA_SEL_FOUNDATION:
            moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_FOUNDATION_TO_TABLEAU, s_sel_index, to_col,
                                           1);
            break;
        default:
            return; /* nada selecionado -- tocar numa coluna nao faz nada */
    }
    apply_move_result(moved);
}

static void attempt_move_to_foundation(uint8_t to_foundation)
{
    bool moved = false;
    switch (s_sel_kind) {
        case PACIENCIA_SEL_TABLEAU:
            moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_TABLEAU_TO_FOUNDATION, s_sel_index,
                                           to_foundation, 1);
            break;
        case PACIENCIA_SEL_WASTE:
            moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_WASTE_TO_FOUNDATION, 0, to_foundation, 1);
            break;
        default:
            return; /* fundacao nunca e origem de jogada para outra fundacao */
    }
    apply_move_result(moved);
}

/* ------------------------------------------------------------------------
 * Callbacks de toque
 * ------------------------------------------------------------------------ */

static void stock_clicked_cb(lv_event_t * e)
{
    (void) e;
    clear_selection();

    bool moved;
    if (s_state.stock.count > 0) {
        moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_STOCK_TO_WASTE, 0, 0, 0);
    } else {
        moved = ratimos_klondike_move(&s_state, RATIMOS_KLONDIKE_MOVE_RECYCLE_WASTE, 0, 0, 0);
    }
    if (moved) {
        persist_state();
    }
    render_all();
}

static void waste_clicked_cb(lv_event_t * e)
{
    (void) e;
    if (s_state.waste.count == 0) {
        return;
    }

    if (s_sel_kind == PACIENCIA_SEL_WASTE) {
        clear_selection();
        render_all();
        return;
    }

    /* O descarte nunca e destino de jogada -- tocar nele sempre
     * (re)comeca uma selecao a partir do seu topo, mesmo com outra
     * selecao ja em curso. */
    s_sel_kind = PACIENCIA_SEL_WASTE;
    s_sel_index = 0;
    s_sel_count = 1;
    render_all();
}

static void foundation_clicked_cb(lv_event_t * e)
{
    uintptr_t idx = (uintptr_t) lv_event_get_user_data(e);
    uint8_t f = (uint8_t) idx;

    if (s_sel_kind == PACIENCIA_SEL_NONE) {
        if (s_state.foundation[f].count == 0) {
            return; /* nada pra selecionar numa fundacao vazia */
        }
        s_sel_kind = PACIENCIA_SEL_FOUNDATION;
        s_sel_index = f;
        s_sel_count = 1;
        render_all();
        return;
    }

    if (s_sel_kind == PACIENCIA_SEL_FOUNDATION && s_sel_index == f) {
        clear_selection();
        render_all();
        return;
    }

    attempt_move_to_foundation(f);
}

static void tableau_clicked_cb(lv_event_t * e)
{
    uintptr_t packed = (uintptr_t) lv_event_get_user_data(e);
    uint8_t col = (uint8_t) (packed & 0xFFu);
    uint8_t idx = (uint8_t) ((packed >> 8) & 0xFFu);

    if (s_sel_kind == PACIENCIA_SEL_NONE) {
        if (idx == PACIENCIA_TABLEAU_SLOT_IDX || idx >= s_state.tableau[col].count) {
            return; /* toque na area vazia sem selecao -- nada pra selecionar */
        }
        s_sel_kind = PACIENCIA_SEL_TABLEAU;
        s_sel_index = col;
        s_sel_count = (uint8_t) (s_state.tableau[col].count - idx);
        render_all();
        return;
    }

    if (s_sel_kind == PACIENCIA_SEL_TABLEAU && s_sel_index == col) {
        clear_selection();
        render_all();
        return;
    }

    attempt_move_to_tableau(col);
}

/* ------------------------------------------------------------------------
 * Render de cada pilha
 * ------------------------------------------------------------------------ */

static void render_stock(void)
{
    lv_obj_clean(s_stock_slot);

    lv_obj_t * card = (s_state.stock.count > 0)
        ? make_card_back(s_stock_slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H)
        : make_card_outline(s_stock_slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
    lv_obj_set_pos(card, 0, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, stock_clicked_cb, LV_EVENT_CLICKED, NULL);
}

static void render_waste(void)
{
    lv_obj_clean(s_waste_slot);

    bool has_card = s_state.waste.count > 0;
    bool selected = (s_sel_kind == PACIENCIA_SEL_WASTE);

    lv_obj_t * card;
    if (has_card) {
        ratimos_card_t top = s_state.waste.cards[s_state.waste.count - 1];
        card = make_card_face(s_waste_slot, top, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H, selected);
    } else {
        card = make_card_outline(s_waste_slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
    }
    lv_obj_set_pos(card, 0, 0);

    if (has_card) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, waste_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
}

static void render_foundation(uint8_t f)
{
    lv_obj_t * parent = s_foundation_slots[f];
    lv_obj_clean(parent);

    bool has_card = s_state.foundation[f].count > 0;
    bool selected = (s_sel_kind == PACIENCIA_SEL_FOUNDATION && s_sel_index == f);

    lv_obj_t * card;
    if (has_card) {
        ratimos_card_t top = s_state.foundation[f].cards[s_state.foundation[f].count - 1];
        card = make_card_face(parent, top, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H, selected);
    } else {
        card = make_card_outline(parent, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
    }
    lv_obj_set_pos(card, 0, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, foundation_clicked_cb, LV_EVENT_CLICKED, (void *) (uintptr_t) f);
}

static void render_tableau_column(uint8_t col)
{
    lv_obj_t * parent = s_tableau_cols[col];
    lv_obj_clean(parent);

    /* Slot sempre presente por baixo das cartas reais -- alvo de toque
     * quando a coluna esta vazia ou quando o toque cai numa faixa nao
     * coberta por nenhuma carta (LVGL escolhe o objeto mais no topo sob o
     * ponto tocado, entao cartas reais tem prioridade sobre o slot). */
    lv_obj_t * slot = make_card_outline(parent, PACIENCIA_TABLEAU_CARD_W, PACIENCIA_TABLEAU_CARD_H);
    lv_obj_set_pos(slot, 0, 0);
    lv_obj_add_flag(slot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slot, tableau_clicked_cb, LV_EVENT_CLICKED,
                        (void *) (uintptr_t) ((uintptr_t) col | ((uintptr_t) PACIENCIA_TABLEAU_SLOT_IDX << 8)));

    const ratimos_pile_t * pile = &s_state.tableau[col];
    uint8_t sel_start = (s_sel_kind == PACIENCIA_SEL_TABLEAU && s_sel_index == col && s_sel_count <= pile->count)
        ? (uint8_t) (pile->count - s_sel_count)
        : 0xFFu;

    for (uint8_t i = 0; i < pile->count; i++) {
        ratimos_card_t card = pile->cards[i];
        int16_t y = (int16_t) (i * PACIENCIA_CASCADE_STEP);
        bool face_up = ratimos_card_is_face_up(card);
        bool selected = (sel_start != 0xFFu && i >= sel_start);
        bool flashing = (s_flash_active && s_flash_kind == PACIENCIA_SEL_TABLEAU && s_flash_index == col
                         && i == (uint8_t) (pile->count - 1));

        lv_obj_t * card_obj;
        if (face_up) {
            card_obj = make_card_face(parent, card, PACIENCIA_TABLEAU_CARD_W, PACIENCIA_TABLEAU_CARD_H,
                                       selected || flashing);
            lv_obj_add_flag(card_obj, LV_OBJ_FLAG_CLICKABLE);
            uintptr_t packed = (uintptr_t) col | ((uintptr_t) i << 8);
            lv_obj_add_event_cb(card_obj, tableau_clicked_cb, LV_EVENT_CLICKED, (void *) packed);
        } else {
            card_obj = make_card_back(parent, PACIENCIA_TABLEAU_CARD_W, PACIENCIA_TABLEAU_CARD_H);
        }
        lv_obj_set_pos(card_obj, 0, y);
    }
}

/* Banner "venceu!" (Display-tier, UI-SPEC): so aparece quando o deal atual
 * esta marcado como vencido. A linha "+1 no castelo" so acompanha o
 * banner quando a vitoria e do deal diario E ja foi creditada no contador
 * compartilhado -- vencer um deal nao-diario (apos "novo jogo") celebra
 * sem creditar nada, igual ao sudoku. */
static void render_banner(void)
{
    if (!s_state.won) {
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(s_banner_label, "venceu!");
    lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);

    if (s_state.daily && s_state.daily_win_recorded) {
        lv_obj_clear_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void render_all(void)
{
    render_stock();
    render_waste();
    for (uint8_t f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        render_foundation(f);
    }
    for (uint8_t c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        render_tableau_column(c);
    }
    render_banner();
}

/* ------------------------------------------------------------------------
 * Recolher (auto-collect) / novo jogo
 * ------------------------------------------------------------------------ */

static void recolher_clicked_cb(lv_event_t * e)
{
    (void) e;
    clear_selection();

    /* O loop e todo dirigido pelo motor (ratimos_klondike_auto_collect),
     * que ja limita suas proprias jogadas em RATIMOS_KLONDIKE_AUTOCOLLECT_MAX
     * e nunca bypassa ratimos_klondike_move() -- nao ha nenhuma escrita
     * direta de pilha aqui. */
    uint16_t moved = ratimos_klondike_auto_collect(&s_state);
    if (moved > 0) {
        handle_win_credit_if_needed();
        persist_state();
    }
    render_all();
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

    /* So apaga o TABULEIRO salvo -- a progressao ja conquistada (contador
     * compartilhado + desbloqueio exclusivo) nunca regride por causa de um
     * reset manual (proibicao do plano). O novo deal nunca e diario: o
     * modo diario so existe na primeira vez que a tela e aberta no dia. */
    ratimos_storage_clear_game_state(RATIMOS_GAME_PACIENCIA);
    clear_selection();
    ratimos_klondike_deal(&s_state, fresh_seed(), false);
    persist_state();
    render_all();
}

static void novo_jogo_clicked_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t * make_pill(lv_obj_t * parent, const char * text, lv_color_t bg, lv_event_cb_t cb, lv_coord_t width)
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

/* ------------------------------------------------------------------------
 * Persistencia
 * ------------------------------------------------------------------------ */

static void persist_state(void)
{
    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &s_state, sizeof(s_state));
    blob.used = sizeof(s_state);

    ratimos_storage_save_game_state(RATIMOS_GAME_PACIENCIA, &blob);
}

/* Um save restaurado com qualquer contagem de pilha fora de
 * [0, RATIMOS_KLONDIKE_MAX_PILE] e tratado como invalido (T-02.1-02) --
 * nunca clampado em vigor, descartado por inteiro, mesma disciplina do
 * sudoku.c. */
static bool validate_restored_state(const ratimos_klondike_state_t * st)
{
    for (int c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        if (st->tableau[c].count > RATIMOS_KLONDIKE_MAX_PILE) {
            return false;
        }
    }
    for (int f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        if (st->foundation[f].count > RATIMOS_KLONDIKE_MAX_PILE) {
            return false;
        }
    }
    if (st->stock.count > RATIMOS_KLONDIKE_MAX_PILE || st->waste.count > RATIMOS_KLONDIKE_MAX_PILE) {
        return false;
    }
    return true;
}

/* Restaura um save valido, ou comeca a semente diaria quando nao ha save
 * (ABSENT) ou o save esta corrompido (INVALID, com aviso). O modo diario
 * e sempre a semente inicial de paciencia -- nao ha selecao de
 * dificuldade neste jogo, diferente do sudoku. */
static bool load_or_start_state(void)
{
    ratimos_game_state_t blob;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_PACIENCIA, &blob);

    if (status == RATIMOS_GAME_STATE_OK && blob.used == sizeof(s_state)) {
        ratimos_klondike_state_t restored;
        memcpy(&restored, blob.bytes, sizeof(restored));
        if (validate_restored_state(&restored)) {
            s_state = restored;
        } else {
            status = RATIMOS_GAME_STATE_INVALID;
        }
    } else if (status == RATIMOS_GAME_STATE_OK) {
        status = RATIMOS_GAME_STATE_INVALID; /* tamanho do blob nao bate com esta versao */
    }

    if (status == RATIMOS_GAME_STATE_OK) {
        clear_selection();
        return false; /* restaurado, sem aviso de erro */
    }

    ratimos_klondike_deal(&s_state, ratimos_daily_seed(RATIMOS_GAME_PACIENCIA), true);
    clear_selection();
    persist_state();

    return status == RATIMOS_GAME_STATE_INVALID;
}

/* ------------------------------------------------------------------------
 * Construcao da tela
 * ------------------------------------------------------------------------ */

static lv_obj_t * build_paciencia_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("paciencia", "toque para mover");

    bool show_load_error = load_or_start_state();

    s_error_label = lv_label_create(shell.content);
    lv_label_set_text(s_error_label,
                      "nao foi possivel carregar seu progresso salvo - comecando um jogo novo");
    lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(s_error_label, lv_pct(100));
    lv_label_set_long_mode(s_error_label, LV_LABEL_LONG_MODE_WRAP);
    if (!show_load_error) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Zona superior: estoque+descarte a esquerda, quatro fundacoes a
     * direita, espacadas pelas pontas (garante pelo menos o gap lg entre
     * os dois clusters, UI-SPEC). */
    lv_obj_t * top_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(top_row);
    lv_obj_set_width(top_row, lv_pct(100));
    lv_obj_set_height(top_row, PACIENCIA_TOP_CARD_H);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(top_row, 16, 0);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * stock_waste_cluster = lv_obj_create(top_row);
    lv_obj_remove_style_all(stock_waste_cluster);
    lv_obj_set_size(stock_waste_cluster, PACIENCIA_TOP_CARD_W * 2 + PACIENCIA_XS_GAP, PACIENCIA_TOP_CARD_H);
    lv_obj_set_flex_flow(stock_waste_cluster, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(stock_waste_cluster, PACIENCIA_XS_GAP, 0);
    lv_obj_clear_flag(stock_waste_cluster, LV_OBJ_FLAG_SCROLLABLE);

    s_stock_slot = lv_obj_create(stock_waste_cluster);
    lv_obj_remove_style_all(s_stock_slot);
    lv_obj_set_size(s_stock_slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
    lv_obj_clear_flag(s_stock_slot, LV_OBJ_FLAG_SCROLLABLE);

    s_waste_slot = lv_obj_create(stock_waste_cluster);
    lv_obj_remove_style_all(s_waste_slot);
    lv_obj_set_size(s_waste_slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
    lv_obj_clear_flag(s_waste_slot, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * foundation_cluster = lv_obj_create(top_row);
    lv_obj_remove_style_all(foundation_cluster);
    lv_obj_set_size(foundation_cluster, RATIMOS_KLONDIKE_FOUNDATIONS * PACIENCIA_TOP_CARD_W
                     + (RATIMOS_KLONDIKE_FOUNDATIONS - 1) * PACIENCIA_XS_GAP, PACIENCIA_TOP_CARD_H);
    lv_obj_set_flex_flow(foundation_cluster, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(foundation_cluster, PACIENCIA_XS_GAP, 0);
    lv_obj_clear_flag(foundation_cluster, LV_OBJ_FLAG_SCROLLABLE);

    for (int f = 0; f < RATIMOS_KLONDIKE_FOUNDATIONS; f++) {
        lv_obj_t * slot = lv_obj_create(foundation_cluster);
        lv_obj_remove_style_all(slot);
        lv_obj_set_size(slot, PACIENCIA_TOP_CARD_W, PACIENCIA_TOP_CARD_H);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
        s_foundation_slots[f] = slot;
    }

    /* Tableau: 7 colunas, cada uma com altura auto-ajustada ao conteudo
     * (cartas posicionadas manualmente em cascata via lv_obj_set_pos). */
    lv_obj_t * tableau_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(tableau_row);
    lv_obj_set_width(tableau_row, lv_pct(100));
    lv_obj_set_flex_grow(tableau_row, 1);
    lv_obj_set_flex_flow(tableau_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tableau_row, PACIENCIA_XS_GAP, 0);
    lv_obj_clear_flag(tableau_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int c = 0; c < RATIMOS_KLONDIKE_TABLEAU_COLS; c++) {
        lv_obj_t * col = lv_obj_create(tableau_row);
        lv_obj_remove_style_all(col);
        lv_obj_set_size(col, PACIENCIA_TABLEAU_CARD_W, LV_SIZE_CONTENT);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
        s_tableau_cols[c] = col;
    }

    /* Banner de vitoria (Display-tier, UI-SPEC): "venceu!" sempre que
     * s_state.won; "+1 no castelo" so junto quando a vitoria diaria ja
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

    /* Linha de acoes: "recolher" (auto-collect) e "novo jogo" (redeal
     * destrutivo com confirmacao) -- disponiveis o tempo todo, nao so
     * apos vencer, igual ao sudoku/conexo. */
    lv_obj_t * actions_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(actions_row);
    lv_obj_set_width(actions_row, lv_pct(100));
    lv_obj_set_height(actions_row, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(actions_row, 8, 0);
    lv_obj_set_flex_flow(actions_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(actions_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions_row, 8, 0);
    lv_obj_clear_flag(actions_row, LV_OBJ_FLAG_SCROLLABLE);
    make_pill(actions_row, "recolher", RATIMOS_COLOR_PANEL, recolher_clicked_cb, 130);
    make_pill(actions_row, "novo jogo", RATIMOS_COLOR_PANEL, novo_jogo_clicked_cb, 130);

    /* Dialogo de confirmacao destrutiva (Copywriting Contract): montado uma
     * unica vez, escondido ate "novo jogo" ser tocado. Filho de
     * shell.screen (nao de shell.content) para flutuar por cima do resto
     * da tela, igual a sudoku.c/conexo.c. */
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
    lv_label_set_text(confirm_msg, "comecar de novo? seu progresso atual nesse jogo sera perdido.");
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

    /* ASCII-only, igual ao precedente ja estabelecido em sudoku.c/conexo.c
     * (o UI-SPEC/PLAN literal usa "recomeçar" com cedilha, mas toda copia
     * nova do projeto fica ASCII -- ver Deviations do SUMMARY). */
    make_pill(confirm_actions, "cancelar", RATIMOS_COLOR_PANEL, confirm_cancel_cb, 100);
    make_pill(confirm_actions, "recomecar", RATIMOS_COLOR_ACCENT, confirm_restart_cb, 100);

    render_all();

    return shell.screen;
}

void ratimos_paciencia_show(lv_event_t * e)
{
    (void) e;
    if (!s_paciencia_screen) {
        s_paciencia_screen = build_paciencia_screen();
    }
    lv_screen_load(s_paciencia_screen);
}
