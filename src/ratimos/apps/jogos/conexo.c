/*
 * Conexo (JOGOS-05) — motor + tela.
 *
 * Persistencia (JOGOS-02): TODA mudanca de estado (selecionar, enviar,
 * embaralhar) grava na hora via ratimos_storage_save_game_state(). Salvar so
 * ao sair da tela perderia o progresso num fechamento inesperado — e o cache
 * de tela em memoria, sozinho, nao sobrevive a um relaunch do processo.
 *
 * Este arquivo NAO abre arquivo nenhum: todo byte que chega ao disco passa
 * pela Storage/Content API (D-10).
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "conexo.h"

#include "../../app_shell.h"
#include "../../theme.h"
#include "../../../storage/content_api.h"
#include "daily_seed.h"

/* O estado serializado precisa caber no blob opaco da Storage API. Se algum
 * plano futuro engordar a struct alem do limite, o build quebra aqui em vez
 * de o save comecar a falhar silenciosamente. */
_Static_assert(sizeof(ratimos_conexo_state_t) <= RATIMOS_GAME_STATE_BLOB_SIZE,
               "ratimos_conexo_state_t nao cabe no blob de save");

#define CONEXO_TILE_COUNT  16
#define CONEXO_GROUP_COUNT 4

/* ------------------------------------------------------------------------
 * Motor (sem LVGL — e isto que a suite Unity exercita)
 * ------------------------------------------------------------------------ */

static uint32_t next_rand(uint32_t * s)
{
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

static uint8_t group_of(uint8_t word_id)
{
    return (uint8_t) (word_id / 4u);
}

static bool group_is_solved(const ratimos_conexo_state_t * state, uint8_t group)
{
    for (uint8_t i = 0; i < state->solved_count && i < CONEXO_GROUP_COUNT; i++) {
        if (state->solved_group_order[i] == group) {
            return true;
        }
    }
    return false;
}

static bool slot_is_selected(const ratimos_conexo_state_t * state, uint8_t slot)
{
    for (uint8_t i = 0; i < state->selected_count && i < 4; i++) {
        if (state->selected[i] == slot) {
            return true;
        }
    }
    return false;
}

void ratimos_conexo_shuffle(ratimos_conexo_state_t * state, uint32_t seed)
{
    if (!state) {
        return;
    }

    /* Junta os slots cujo grupo ainda nao foi resolvido e permuta apenas os
     * valores entre eles (Fisher-Yates) — pertencimento de grupo intacto. */
    uint8_t open_slots[CONEXO_TILE_COUNT];
    uint8_t open_count = 0;

    for (uint8_t slot = 0; slot < CONEXO_TILE_COUNT; slot++) {
        if (!group_is_solved(state, group_of(state->tile_word[slot]))) {
            open_slots[open_count++] = slot;
        }
    }

    uint32_t s = seed;
    for (uint8_t i = open_count; i > 1; i--) {
        uint8_t j = (uint8_t) ((next_rand(&s) >> 16) % i);
        uint8_t a = open_slots[i - 1];
        uint8_t b = open_slots[j];
        uint8_t tmp = state->tile_word[a];
        state->tile_word[a] = state->tile_word[b];
        state->tile_word[b] = tmp;
    }
}

void ratimos_conexo_start(ratimos_conexo_state_t * state, uint16_t puzzle_index, uint32_t seed)
{
    if (!state) {
        return;
    }

    /* O anel de historico precisa sobreviver a este reset -- e ele que faz
     * "novo jogo" (e a primeira vez apos um save invalido) evitar repetir o
     * quebra-cabeca anterior. Todo o resto do tabuleiro comeca zerado. */
    ratimos_puzzle_history_t history = state->history;
    memset(state, 0, sizeof(*state));
    state->history = history;
    state->puzzle_index = puzzle_index;

    ratimos_conexo_puzzle_t puzzle;
    if (ratimos_conexo_get_puzzle((size_t) puzzle_index, &puzzle)) {
        snprintf(state->puzzle_id, sizeof(state->puzzle_id), "%s", puzzle.id);
    }

    for (uint8_t i = 0; i < CONEXO_TILE_COUNT; i++) {
        state->tile_word[i] = i;
    }
    memset(state->solved_group_order, RATIMOS_CONEXO_NO_GROUP, sizeof(state->solved_group_order));

    ratimos_conexo_shuffle(state, seed);
}

/* Revela todos os grupos ainda nao resolvidos, na ordem de dificuldade —
 * usado no caminho de derrota, que mostra as categorias em vez de um "voce
 * perdeu" seco. */
static void reveal_remaining_groups(ratimos_conexo_state_t * state)
{
    for (uint8_t g = 0; g < CONEXO_GROUP_COUNT; g++) {
        if (!group_is_solved(state, g) && state->solved_count < CONEXO_GROUP_COUNT) {
            state->solved_group_order[state->solved_count] = g;
            state->solved_count++;
        }
    }
}

ratimos_conexo_submit_t ratimos_conexo_submit(ratimos_conexo_state_t * state,
                                              const ratimos_conexo_puzzle_t * puzzle)
{
    if (!state || !puzzle) {
        return RATIMOS_CONEXO_SUBMIT_REJECTED_INCOMPLETE;
    }

    /* Selecao incompleta nao e erro do jogador — nao conta falta e nao muda
     * absolutamente nada no tabuleiro. */
    if (state->selected_count != 4 || state->finished != 0) {
        return RATIMOS_CONEXO_SUBMIT_REJECTED_INCOMPLETE;
    }

    uint8_t counts[CONEXO_GROUP_COUNT] = { 0, 0, 0, 0 };
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t slot = state->selected[i];
        if (slot >= CONEXO_TILE_COUNT) {
            return RATIMOS_CONEXO_SUBMIT_REJECTED_INCOMPLETE;
        }
        counts[group_of(state->tile_word[slot])]++;
    }

    uint8_t best_group = 0;
    uint8_t best = 0;
    for (uint8_t g = 0; g < CONEXO_GROUP_COUNT; g++) {
        if (counts[g] > best) {
            best = counts[g];
            best_group = g;
        }
    }

    state->selected_count = 0;
    memset(state->selected, 0, sizeof(state->selected));

    if (best == 4) {
        if (state->solved_count < CONEXO_GROUP_COUNT) {
            state->solved_group_order[state->solved_count] = best_group;
            state->solved_count++;
        }
        if (state->solved_count >= CONEXO_GROUP_COUNT) {
            state->finished = 1;
        }
        return RATIMOS_CONEXO_SUBMIT_CORRECT;
    }

    if (state->mistakes < 0xFF) {
        state->mistakes++;
    }
    if (state->mistakes >= RATIMOS_CONEXO_MAX_MISTAKES) {
        reveal_remaining_groups(state);
        state->finished = 2;
    }

    /* "Faltou uma": exatamente 3 das 4 no mesmo grupo. Nunca resolve o grupo
     * parcialmente — conta um erro e o tabuleiro fica como estava. */
    return (best == 3) ? RATIMOS_CONEXO_SUBMIT_ONE_AWAY : RATIMOS_CONEXO_SUBMIT_WRONG;
}

/* Chama a Storage API diretamente (PROGRESSAO-01) -- nao depende de LVGL,
 * entao a suite Unity testa a regra "conta uma unica vez" sem precisar de
 * tela nenhuma. Uma derrota (finished == 2) nunca soma o castelo. */
bool ratimos_conexo_record_win_if_needed(ratimos_conexo_state_t * state)
{
    if (!state || state->finished != 1 || state->daily_win_recorded) {
        return false;
    }

    if (!ratimos_storage_record_daily_win(RATIMOS_GAME_CONEXO)) {
        return false;
    }

    state->daily_win_recorded = 1;
    return true;
}

/* ------------------------------------------------------------------------
 * Tela (LVGL) — cache-once, igual a jogos_app.c / cartas_app.c.
 *
 * A tela e construida UMA vez e guardada em s_conexo_screen; toda jogada
 * apenas atualiza os objetos ja existentes via render_board(). Reconstruir a
 * tela a cada visita e exatamente o vazamento de heap do LVGL que a Fase 1 ja
 * corrigiu — nao pode voltar.
 * ------------------------------------------------------------------------ */

#define TILE_W 72
#define TILE_H 56
#define BAND_H 30
#define PILL_W 146
#define PILL_H 32

static lv_obj_t * s_conexo_screen = NULL;
static lv_obj_t * s_error_label = NULL;
static lv_obj_t * s_unavailable_label = NULL;
static lv_obj_t * s_bands[CONEXO_GROUP_COUNT];
static lv_obj_t * s_band_labels[CONEXO_GROUP_COUNT];
static lv_obj_t * s_grid = NULL;
static lv_obj_t * s_tiles[CONEXO_TILE_COUNT];
static lv_obj_t * s_tile_labels[CONEXO_TILE_COUNT];
static lv_obj_t * s_banner_label = NULL;
static lv_obj_t * s_castle_label = NULL;
static lv_obj_t * s_mistakes_label = NULL;
static lv_obj_t * s_confirm_overlay = NULL;

static ratimos_conexo_state_t s_state;
static ratimos_conexo_puzzle_t s_puzzle;
static bool s_puzzle_ready = false;

static lv_color_t difficulty_color(ratimos_conexo_difficulty_t d)
{
    switch (d) {
        case RATIMOS_CONEXO_DIFF_YELLOW: return RATIMOS_COLOR_CONEXO_YELLOW;
        case RATIMOS_CONEXO_DIFF_GREEN:  return RATIMOS_COLOR_CONEXO_GREEN;
        case RATIMOS_CONEXO_DIFF_BLUE:   return RATIMOS_COLOR_CONEXO_BLUE;
        default:                         return RATIMOS_COLOR_CONEXO_PURPLE;
    }
}

static const char * word_text(uint8_t word_id)
{
    uint8_t g = group_of(word_id);
    uint8_t w = (uint8_t) (word_id % 4u);
    if (g >= CONEXO_GROUP_COUNT) {
        return "";
    }
    return s_puzzle.groups[g].words[w];
}

static void persist_state(void)
{
    ratimos_game_state_t blob;
    memset(&blob, 0, sizeof(blob));
    memcpy(blob.bytes, &s_state, sizeof(s_state));
    blob.used = sizeof(s_state);

    ratimos_storage_save_game_state(RATIMOS_GAME_CONEXO, &blob);
}

static void render_board(void)
{
    if (!s_puzzle_ready) {
        return;
    }

    /* Faixas das categorias resolvidas, na ORDEM em que foram resolvidas. */
    for (uint8_t i = 0; i < CONEXO_GROUP_COUNT; i++) {
        if (i < s_state.solved_count && s_state.solved_group_order[i] < CONEXO_GROUP_COUNT) {
            uint8_t g = s_state.solved_group_order[i];
            lv_obj_set_style_bg_color(s_bands[i], difficulty_color(s_puzzle.groups[g].difficulty), 0);
            lv_label_set_text(s_band_labels[i], s_puzzle.groups[g].category_name);
            lv_obj_clear_flag(s_bands[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_bands[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Tiles: slots de grupos ja resolvidos somem do grid (o flex do LVGL
     * ignora filhos escondidos, entao as restantes se reagrupam sozinhas). */
    for (uint8_t slot = 0; slot < CONEXO_TILE_COUNT; slot++) {
        uint8_t word_id = s_state.tile_word[slot];

        if (group_is_solved(&s_state, group_of(word_id))) {
            lv_obj_add_flag(s_tiles[slot], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(s_tiles[slot], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_tile_labels[slot], word_text(word_id));

        if (slot_is_selected(&s_state, slot)) {
            lv_obj_set_style_bg_color(s_tiles[slot], RATIMOS_COLOR_PANEL_ACTIVE, 0);
            lv_obj_set_style_border_color(s_tiles[slot], RATIMOS_COLOR_ACCENT, 0);
            lv_obj_set_style_border_width(s_tiles[slot], 2, 0);
        } else {
            lv_obj_set_style_bg_color(s_tiles[slot], RATIMOS_COLOR_PANEL, 0);
            lv_obj_set_style_border_color(s_tiles[slot], RATIMOS_COLOR_PANEL_ACTIVE, 0);
            lv_obj_set_style_border_width(s_tiles[slot], 1, 0);
        }
    }

    lv_label_set_text_fmt(s_mistakes_label, "erros: %d/%d",
                          (int) s_state.mistakes, RATIMOS_CONEXO_MAX_MISTAKES);

    if (s_state.finished == 1) {
        lv_label_set_text(s_banner_label, "categorias completas!");
        lv_obj_set_style_text_color(s_banner_label, RATIMOS_COLOR_TEXT, 0);
        lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    } else if (s_state.finished == 2) {
        lv_label_set_text(s_banner_label, "quase la - aqui estao as categorias");
        lv_obj_set_style_text_color(s_banner_label, RATIMOS_COLOR_TEXT_MUTED, 0);
        lv_obj_clear_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void tile_clicked_cb(lv_event_t * e)
{
    if (s_state.finished != 0) {
        return;
    }

    lv_obj_t * tile = lv_event_get_target(e);
    uint8_t slot = (uint8_t) (uintptr_t) lv_obj_get_user_data(tile);
    if (slot >= CONEXO_TILE_COUNT) {
        return;
    }

    if (slot_is_selected(&s_state, slot)) {
        uint8_t out = 0;
        for (uint8_t i = 0; i < s_state.selected_count; i++) {
            if (s_state.selected[i] != slot) {
                s_state.selected[out++] = s_state.selected[i];
            }
        }
        s_state.selected_count = out;
    } else if (s_state.selected_count < 4) {
        s_state.selected[s_state.selected_count++] = slot;
    } else {
        /* Quinta tile: ignorada de proposito, sem feedback de erro. */
        return;
    }

    persist_state();
    render_board();
}

static void submit_clicked_cb(lv_event_t * e)
{
    (void) e;
    ratimos_conexo_submit(&s_state, &s_puzzle);
    /* Vitoria (JOGOS-05) soma o castelo (PROGRESSAO-01) exatamente uma vez
     * por board -- o guard fica dentro da propria funcao do motor. */
    ratimos_conexo_record_win_if_needed(&s_state);
    persist_state();
    render_board();
}

static void shuffle_clicked_cb(lv_event_t * e)
{
    (void) e;
    if (s_state.finished != 0) {
        return;
    }
    ratimos_conexo_shuffle(&s_state, ratimos_daily_seed(RATIMOS_GAME_CONEXO) + s_state.mistakes + 1u);
    persist_state();
    render_board();
}

/* Sorteia o proximo quebra-cabeca evitando o historico recente (D-11),
 * inicia o motor preservando esse historico e persiste o board novo na
 * hora. Usado tanto pelo primeiro play quanto pelo "novo jogo" manual. */
static void start_new_board(void)
{
    size_t index = ratimos_puzzle_pick(&s_state.history, ratimos_conexo_puzzle_count(),
                                       ratimos_daily_seed(RATIMOS_GAME_CONEXO));
    ratimos_conexo_get_puzzle(index, &s_puzzle);
    ratimos_conexo_start(&s_state, (uint16_t) index, ratimos_daily_seed(RATIMOS_GAME_CONEXO));
    ratimos_puzzle_history_push(&s_state.history, (uint16_t) index);
    persist_state();
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
    ratimos_storage_clear_game_state(RATIMOS_GAME_CONEXO);
    start_new_board();
    render_board();
}

static void novo_jogo_clicked_cb(lv_event_t * e)
{
    (void) e;
    lv_obj_clear_flag(s_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t * make_pill_w(lv_obj_t * parent, const char * text, lv_color_t bg, lv_event_cb_t cb, lv_coord_t width)
{
    lv_obj_t * pill = ratimos_panel_create(parent);
    lv_obj_set_size(pill, width, PILL_H);
    lv_obj_set_style_pad_all(pill, 0, 0);
    lv_obj_set_style_bg_color(pill, bg, 0);
    lv_obj_set_style_radius(pill, PILL_H / 2, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pill, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(pill);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    lv_obj_center(label);

    return pill;
}

static lv_obj_t * make_pill(lv_obj_t * parent, const char * text, lv_color_t bg, lv_event_cb_t cb)
{
    return make_pill_w(parent, text, bg, cb, PILL_W);
}

/* Decide o estado inicial: restaura um save valido, comeca limpo quando nao
 * ha save, ou comeca limpo COM aviso quando o save esta corrompido. */
static bool load_or_start_state(void)
{
    ratimos_game_state_t blob;
    ratimos_game_state_status_t status = ratimos_storage_get_game_state(RATIMOS_GAME_CONEXO, &blob);

    if (status == RATIMOS_GAME_STATE_OK && blob.used == sizeof(s_state)) {
        memcpy(&s_state, blob.bytes, sizeof(s_state));
        if (ratimos_conexo_get_puzzle((size_t) s_state.puzzle_index, &s_puzzle)) {
            return false; /* restaurado, sem aviso de erro */
        }
        status = RATIMOS_GAME_STATE_INVALID; /* save aponta pra um quebra-cabeca inexistente */
    } else if (status == RATIMOS_GAME_STATE_OK) {
        status = RATIMOS_GAME_STATE_INVALID; /* tamanho do blob nao bate com esta versao */
    }

    start_new_board();

    return status == RATIMOS_GAME_STATE_INVALID;
}

static lv_obj_t * build_conexo_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("conexo", "toque 4 para agrupar");

    /* Banco vazio/quebrado: caminho defensivo, sem grid nenhum. */
    if (ratimos_conexo_puzzle_count() == 0) {
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

    /* Aviso de save invalido (UI-SPEC, estado de erro): montado uma unica vez
     * na construcao da tela, nunca a cada render. */
    s_error_label = lv_label_create(shell.content);
    lv_label_set_text(s_error_label,
                      "nao foi possivel carregar seu progresso salvo - comecando um jogo novo");
    lv_obj_set_style_text_color(s_error_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_set_width(s_error_label, lv_pct(100));
    lv_label_set_long_mode(s_error_label, LV_LABEL_LONG_MODE_WRAP);
    if (!show_load_error) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t i = 0; i < CONEXO_GROUP_COUNT; i++) {
        s_bands[i] = ratimos_panel_create(shell.content);
        lv_obj_set_size(s_bands[i], lv_pct(100), BAND_H);
        lv_obj_set_style_pad_all(s_bands[i], 0, 0);
        lv_obj_set_style_border_width(s_bands[i], 0, 0);
        lv_obj_clear_flag(s_bands[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_bands[i], LV_OBJ_FLAG_HIDDEN);

        s_band_labels[i] = lv_label_create(s_bands[i]);
        lv_label_set_text(s_band_labels[i], "");
        lv_obj_set_style_text_color(s_band_labels[i], RATIMOS_COLOR_BG, 0);
        lv_obj_center(s_band_labels[i]);
    }

    s_grid = lv_obj_create(shell.content);
    lv_obj_remove_style_all(s_grid);
    lv_obj_set_width(s_grid, lv_pct(100));
    lv_obj_set_height(s_grid, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(s_grid, 4, 0);
    lv_obj_set_style_pad_column(s_grid, 4, 0);
    lv_obj_clear_flag(s_grid, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t slot = 0; slot < CONEXO_TILE_COUNT; slot++) {
        s_tiles[slot] = ratimos_panel_create(s_grid);
        lv_obj_set_size(s_tiles[slot], TILE_W, TILE_H);
        lv_obj_set_style_pad_all(s_tiles[slot], 2, 0);
        lv_obj_clear_flag(s_tiles[slot], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_tiles[slot], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(s_tiles[slot], (void *) (uintptr_t) slot);
        lv_obj_add_event_cb(s_tiles[slot], tile_clicked_cb, LV_EVENT_CLICKED, NULL);

        s_tile_labels[slot] = lv_label_create(s_tiles[slot]);
        /* Palavra longa em PT-BR quebra linha dentro da tile em vez de vazar. */
        lv_obj_set_width(s_tile_labels[slot], lv_pct(100));
        lv_label_set_long_mode(s_tile_labels[slot], LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_style_text_align(s_tile_labels[slot], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(s_tile_labels[slot], RATIMOS_COLOR_TEXT, 0);
        lv_obj_center(s_tile_labels[slot]);
        lv_label_set_text(s_tile_labels[slot], "");
    }

    s_banner_label = lv_label_create(shell.content);
    lv_label_set_text(s_banner_label, "");
    lv_obj_set_width(s_banner_label, lv_pct(100));
    lv_label_set_long_mode(s_banner_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(s_banner_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(s_banner_label, LV_OBJ_FLAG_HIDDEN);

    /* Linha "+1 no castelo" (PROGRESSAO-01): so aparece junto do banner de
     * vitoria, nunca no banner de derrota/revelacao. */
    s_castle_label = lv_label_create(shell.content);
    lv_label_set_text(s_castle_label, "+1 no castelo");
    lv_obj_set_width(s_castle_label, lv_pct(100));
    lv_obj_set_style_text_align(s_castle_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_castle_label, RATIMOS_COLOR_TEXT_MUTED, 0);
    lv_obj_add_flag(s_castle_label, LV_OBJ_FLAG_HIDDEN);

    s_mistakes_label = lv_label_create(shell.content);
    lv_label_set_text(s_mistakes_label, "erros: 0/4");
    lv_obj_set_style_text_color(s_mistakes_label, RATIMOS_COLOR_TEXT_MUTED, 0);

    lv_obj_t * actions = lv_obj_create(shell.content);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(actions, 8, 0);
    lv_obj_set_style_pad_column(actions, 8, 0);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

    make_pill(actions, "embaralhar", RATIMOS_COLOR_PANEL, shuffle_clicked_cb);
    make_pill(actions, "enviar", RATIMOS_COLOR_ACCENT, submit_clicked_cb);
    make_pill(actions, "novo jogo", RATIMOS_COLOR_PANEL, novo_jogo_clicked_cb);

    /* Dialogo de confirmacao destrutiva (Copywriting Contract): montado uma
     * unica vez, escondido ate "novo jogo" ser tocado. Filho de shell.screen
     * (nao de shell.content) para flutuar por cima do resto da tela. */
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

void ratimos_conexo_show(lv_event_t * e)
{
    (void) e;
    if (!s_conexo_screen) {
        s_conexo_screen = build_conexo_screen();
    }
    lv_screen_load(s_conexo_screen);
}
