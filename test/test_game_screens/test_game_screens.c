/*
 * Suite das telas de jogo reais (plano 02.1-14, fixes do checkpoint da
 * Task 5). Constroi as telas de producao num display LVGL headless e
 * verifica o que a usuaria reportou no simulador:
 *   - o dialogo "comecar de novo?" aparece centralizado na TELA (antes caia
 *     no canto inferior esquerdo, cortado);
 *   - o enter do termo submete "peste" e um palpite rejeitado mostra o
 *     motivo na tela (antes so' engrossava a borda em 1px);
 *   - tabuleiro + teclado de todo jogo (e dos 3 modos do termo) cabem no
 *     content sem rolar, e a sectionbar e' compacta (20px, sem gap);
 *
 * Isolamento: main() faz chdir() pra um diretorio temporario antes de
 * qualquer chamada de storage -- os saves (assets/save/, caminho relativo)
 * nunca tocam o progresso real do simulador no repo.
 */
#include <unity.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lvgl.h"
#include "ratimos/theme.h"
#include "ratimos/apps/jogos/conexo.h"
#include "ratimos/apps/jogos/cruzadinha.h"
#include "ratimos/apps/jogos/paciencia.h"
#include "ratimos/apps/jogos/sudoku.h"
#include "ratimos/apps/jogos/termo.h"
#include "storage/content_api.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

/*
 * Pool EXTRA de heap LVGL so' pra este processo de teste. As 5 telas de
 * jogo ficam em cache pra sempre (sem delete-on-navigate -- deferred-items
 * #1, escopo do plano 02.1-15) e juntas usam ~445KB dos 512KB do
 * LV_MEM_SIZE: sem folga, a suite estourava o heap. O simulador NAO ganha
 * este pool -- o vazamento real continua visivel la' ate o 02.1-15.
 */
static uint8_t s_extra_lv_pool[512 * 1024] __attribute__((aligned(8)));

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

void setUp(void) {}
void tearDown(void) {}

typedef void (*show_fn_t)(lv_event_t * e);

static const show_fn_t GAME_SHOW[] = {
    ratimos_sudoku_show, ratimos_paciencia_show, ratimos_termo_show,
    ratimos_cruzadinha_show, ratimos_conexo_show,
};
static const char * const GAME_NAME[] = { "sudoku", "paciencia", "termo", "cruzadinha", "conexo" };
#define GAME_N (sizeof(GAME_SHOW) / sizeof(GAME_SHOW[0]))

static void log_heap(const char * tag)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    fprintf(stderr, "heap %s: used %u / %u\n", tag, (unsigned) (m.total_size - m.free_size), (unsigned) m.total_size);
}

static lv_obj_t * show_game(size_t i)
{
    GAME_SHOW[i](NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_update_layout(scr);
    log_heap(GAME_NAME[i]);
    return scr;
}

/* Painel de dialogo = filho de um scrim FLOATING de tela cheia (filho
 * direto da tela). Procura o primeiro scrim cujo painel tem o texto do
 * dialogo de confirmacao. */
static bool subtree_has_text(lv_obj_t * obj, const char * needle)
{
    if (lv_obj_get_class(obj) == &lv_label_class && strstr(lv_label_get_text(obj), needle)) {
        return true;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        if (subtree_has_text(lv_obj_get_child(obj, (int32_t) i), needle)) {
            return true;
        }
    }
    return false;
}

static lv_obj_t * find_confirm_panel(lv_obj_t * scr)
{
    for (uint32_t i = 0; i < lv_obj_get_child_count(scr); i++) {
        lv_obj_t * c = lv_obj_get_child(scr, (int32_t) i);
        if (lv_obj_has_flag(c, LV_OBJ_FLAG_FLOATING) && lv_obj_get_child_count(c) == 1) {
            lv_obj_t * panel = lv_obj_get_child(c, 0);
            if (subtree_has_text(panel, "comecar de novo?")) {
                return panel;
            }
        }
    }
    return NULL;
}

/*
 * Fix 3: o dialogo de confirmacao de TODOS os jogos e' um modal
 * compartilhado -- scrim FLOATING cobrindo a tela inteira, painel
 * centralizado na tela, inteiro dentro dos 320x480, e o content do app nao
 * encolhe quando ele aparece.
 */
void test_confirm_dialog_is_centered_on_screen_in_every_game(void)
{
    for (size_t g = 0; g < GAME_N; g++) {
        lv_obj_t * scr = show_game(g);
        lv_obj_t * content = lv_obj_get_child(scr, 3);
        int32_t content_h_before = lv_obj_get_height(content);

        lv_obj_t * panel = find_confirm_panel(scr);
        TEST_ASSERT_NOT_NULL_MESSAGE(panel, GAME_NAME[g]);
        TEST_ASSERT_FALSE_MESSAGE(ratimos_modal_is_visible(panel), GAME_NAME[g]);

        ratimos_modal_show(panel);
        lv_obj_update_layout(scr);

        lv_obj_t * scrim = lv_obj_get_parent(panel);
        lv_area_t s;
        lv_obj_get_coords(scrim, &s);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, s.x1, GAME_NAME[g]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, s.y1, GAME_NAME[g]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(RATIMOS_SCREEN_W, lv_area_get_width(&s), GAME_NAME[g]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(RATIMOS_SCREEN_H, lv_area_get_height(&s), GAME_NAME[g]);

        lv_area_t p;
        lv_obj_get_coords(panel, &p);
        int32_t cx = (p.x1 + p.x2) / 2;
        int32_t cy = (p.y1 + p.y2) / 2;
        TEST_ASSERT_INT_WITHIN_MESSAGE(2, RATIMOS_SCREEN_W / 2, cx, GAME_NAME[g]);
        TEST_ASSERT_INT_WITHIN_MESSAGE(2, RATIMOS_SCREEN_H / 2, cy, GAME_NAME[g]);
        TEST_ASSERT_TRUE_MESSAGE(p.x1 >= 0 && p.y1 >= 0 && p.x2 < RATIMOS_SCREEN_W && p.y2 < RATIMOS_SCREEN_H,
                                 GAME_NAME[g]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(content_h_before, lv_obj_get_height(content), GAME_NAME[g]);

        ratimos_modal_hide(panel);
        TEST_ASSERT_FALSE(ratimos_modal_is_visible(panel));
    }
}

/* Termo: content = [0 erro, 1 pilulas, 2 status, 3 boards, 4 teclado, ...]. */
static void termo_press(lv_obj_t * kb, const char * key)
{
    for (uint32_t id = 0; id < 64; id++) {
        const char * t = lv_buttonmatrix_get_button_text(kb, id);
        if (t == NULL) {
            break;
        }
        if (strcmp(t, key) == 0) {
            lv_buttonmatrix_set_selected_button(kb, id);
            lv_obj_send_event(kb, LV_EVENT_VALUE_CHANGED, NULL);
            return;
        }
    }
    TEST_FAIL_MESSAGE(key);
}

static void termo_type(lv_obj_t * kb, const char * word)
{
    for (const char * c = word; *c; c++) {
        char k[2] = { (char) toupper((unsigned char) *c), 0 };
        termo_press(kb, k);
    }
}

void test_termo_enter_submits_and_rejections_are_visible(void)
{
    lv_obj_t * scr = show_game(2);
    lv_obj_t * content = lv_obj_get_child(scr, 3);
    lv_obj_t * status = lv_obj_get_child(content, 2);
    lv_obj_t * kb = lv_obj_get_child(content, 4);
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(status));
    TEST_ASSERT_EQUAL_PTR(&lv_buttonmatrix_class, lv_obj_get_class(kb));

    /* Palpite incompleto: motivo na tela, nenhuma tentativa gasta. */
    termo_type(kb, "pes");
    termo_press(kb, "enter");
    TEST_ASSERT_EQUAL_STRING("complete as 5 letras", lv_label_get_text(status));
    termo_type(kb, "te");
    TEST_ASSERT_EQUAL_STRING("tentativa 1 de 6", lv_label_get_text(status));

    /* "peste": aceita, consome a tentativa 1. */
    termo_press(kb, "enter");
    TEST_ASSERT_EQUAL_STRING("tentativa 2 de 6", lv_label_get_text(status));

    /* Fora da lista: motivo na tela, ainda tentativa 2. */
    termo_type(kb, "zzzzz");
    termo_press(kb, "enter");
    TEST_ASSERT_EQUAL_STRING("palavra fora da lista", lv_label_get_text(status));
    termo_press(kb, LV_SYMBOL_BACKSPACE);
    TEST_ASSERT_EQUAL_STRING("tentativa 2 de 6", lv_label_get_text(status));
}

/* Tudo que esta visivel no content cabe nele, sem rolagem. */
static void assert_content_fits(lv_obj_t * scr, const char * what)
{
    lv_obj_update_layout(scr);
    lv_obj_t * content = lv_obj_get_child(scr, 3);
    TEST_ASSERT_TRUE_MESSAGE(lv_obj_get_scroll_bottom(content) <= 0, what);
    lv_area_t ca;
    lv_obj_get_content_coords(content, &ca);
    for (uint32_t i = 0; i < lv_obj_get_child_count(content); i++) {
        lv_obj_t * c = lv_obj_get_child(content, (int32_t) i);
        if (lv_obj_has_flag(c, LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        lv_area_t a;
        lv_obj_get_coords(c, &a);
        TEST_ASSERT_TRUE_MESSAGE(a.y2 <= ca.y2, what);
    }
}

void test_every_game_fits_without_scrolling(void)
{
    for (size_t g = 0; g < GAME_N; g++) {
        lv_obj_t * scr = show_game(g);
        assert_content_fits(scr, GAME_NAME[g]);
    }
}

/* Troca de modo pela pilula; confirma o dialogo se ele aparecer. */
static void termo_switch_mode(lv_obj_t * scr, int mode)
{
    lv_obj_t * content = lv_obj_get_child(scr, 3);
    lv_obj_t * pill = lv_obj_get_child(lv_obj_get_child(content, 1), mode);
    lv_obj_send_event(pill, LV_EVENT_CLICKED, NULL);
    lv_obj_t * panel = find_confirm_panel(scr);
    if (panel && ratimos_modal_is_visible(panel)) {
        lv_obj_t * actions = lv_obj_get_child(panel, 1);
        lv_obj_send_event(lv_obj_get_child(actions, 1), LV_EVENT_CLICKED, NULL);
    }
}

void test_termo_all_modes_fit_and_keys_are_even(void)
{
    static const char * const MODES[] = { "termo", "dueto", "quarteto" };
    lv_obj_t * scr = show_game(2);
    for (int m = 2; m >= 0; m--) {
        termo_switch_mode(scr, m);
        assert_content_fits(scr, MODES[m]);
    }

    /* Teclado de 3 fileiras: letras com a mesma largura (+-1px) em todas. */
    lv_obj_t * kb = lv_obj_get_child(lv_obj_get_child(scr, 3), 4);
    lv_obj_update_layout(scr);
    int32_t kb_w = lv_obj_get_content_width(kb);
    /* largura de 2 unidades por fileira = (w - gaps) * 2 / 20 */
    int32_t gap = lv_obj_get_style_pad_column(kb, LV_PART_MAIN);
    int32_t w_row1 = (kb_w - 9 * gap) * 2 / 20;
    int32_t w_row2 = (kb_w - 10 * gap) * 2 / 20;
    int32_t w_row3 = (kb_w - 8 * gap) * 2 / 20;
    TEST_ASSERT_INT_WITHIN(1, w_row1, w_row2);
    TEST_ASSERT_INT_WITHIN(1, w_row1, w_row3);
    TEST_ASSERT_TRUE(w_row1 >= 26);
    TEST_ASSERT_EQUAL_STRING("Q", lv_buttonmatrix_get_button_text(kb, 0));
    TEST_ASSERT_EQUAL_STRING("enter", lv_buttonmatrix_get_button_text(kb, 21));
    TEST_ASSERT_EQUAL_STRING(LV_SYMBOL_BACKSPACE, lv_buttonmatrix_get_button_text(kb, 29));
}

/* Sectionbar compacta: 20px colada no topbar, content logo abaixo. */
void test_sectionbar_is_compact_and_content_moves_up(void)
{
    lv_obj_t * scr = show_game(0);
    lv_area_t top, sec, content;
    lv_obj_get_coords(lv_obj_get_child(scr, 1), &top);
    lv_obj_get_coords(lv_obj_get_child(scr, 2), &sec);
    lv_obj_get_coords(lv_obj_get_child(scr, 3), &content);
    TEST_ASSERT_EQUAL_INT(RATIMOS_TOPBAR_H, lv_area_get_height(&top));
    TEST_ASSERT_EQUAL_INT(RATIMOS_SECTIONBAR_H, lv_area_get_height(&sec));
    TEST_ASSERT_EQUAL_INT(top.y2 + 1, sec.y1);
    TEST_ASSERT_EQUAL_INT(sec.y2 + 1, content.y1);
}

int main(void)
{
    char tmpl[] = "/tmp/ratimos_game_screens_XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    lv_init();
    lv_mem_add_pool(s_extra_lv_pool, sizeof(s_extra_lv_pool));
    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    ratimos_storage_mount();
    ratimos_storage_index_games();
    ratimos_storage_index_settings();
    ratimos_storage_index_game_state();

    UNITY_BEGIN();
    /* Ordem importa: cada tela de jogo e' construida uma vez e fica em
     * cache pra sempre (sem delete-on-navigate -- deferred-items #1, plano
     * 02.1-15), e as 5 juntas usam ~445KB dos 512KB do heap LVGL. Os testes
     * que mexem muito no termo rodam ANTES das outras 4 telas existirem. */
    RUN_TEST(test_termo_enter_submits_and_rejections_are_visible);
    RUN_TEST(test_termo_all_modes_fit_and_keys_are_even);
    RUN_TEST(test_sectionbar_is_compact_and_content_moves_up);
    RUN_TEST(test_confirm_dialog_is_centered_on_screen_in_every_game);
    RUN_TEST(test_every_game_fits_without_scrolling);
    return UNITY_END();
}
