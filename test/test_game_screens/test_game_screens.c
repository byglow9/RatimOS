/*
 * Suite das telas de jogo reais (plano 02.1-14, fixes do checkpoint da
 * Task 5). Constroi as telas de producao num display LVGL headless e
 * verifica o que a usuaria reportou no simulador:
 *   - o dialogo "comecar de novo?" aparece centralizado na TELA (antes caia
 *     no canto inferior esquerdo, cortado);
 *
 * Isolamento: main() faz chdir() pra um diretorio temporario antes de
 * qualquer chamada de storage -- os saves (assets/save/, caminho relativo)
 * nunca tocam o progresso real do simulador no repo.
 */
#include <unity.h>

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

static lv_obj_t * show_game(size_t i)
{
    GAME_SHOW[i](NULL);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_update_layout(scr);
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

int main(void)
{
    char tmpl[] = "/tmp/ratimos_game_screens_XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    lv_init();
    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    ratimos_storage_mount();
    ratimos_storage_index_games();
    ratimos_storage_index_settings();
    ratimos_storage_index_game_state();

    UNITY_BEGIN();
    RUN_TEST(test_confirm_dialog_is_centered_on_screen_in_every_game);
    return UNITY_END();
}
