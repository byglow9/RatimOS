/*
 * Suite do theme.c compartilhado (plano 02.1-09, Task 1) -- primeira suite
 * deste projeto que introspecciona widgets LVGL reais, nao so logica pura.
 * Pina a correcao dos gaps G-02.1-1/G-02.1-2: o badge circular vermelho
 * clicavel foi removido de ratimos_badge_create(), causa raiz do bug de
 * hitbox ("precisei clicar umas 10 vezes pra entrar no conexo") -- a linha/
 * tile pai agora recebe o toque diretamente, sem um lv_obj_create()
 * fantasma interceptando no meio.
 *
 * LVGL e' inicializado headless: um display 320x480 com buffer proprio e
 * um flush_cb que so' chama lv_display_flush_ready() -- sem SDL, sem
 * janela, roda em qualquer ambiente native_sim (CI incluso).
 */
#include <unity.h>

#include "ratimos/theme.h"
#include "ratimos/row_list.h"
#include "ratimos/bg_images.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

void setUp(void) {}
void tearDown(void) {}

/*
 * Behavior 1: match de icone real -- retorna um objeto NAO clicavel cujo
 * conteudo e' exatamente a imagem do icone (lv_image_create direto, sem
 * lv_obj_create() de container/recorte de circulo/fundo vermelho por
 * baixo).
 */
void test_badge_icon_match_is_bare_non_clickable_image(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * badge = ratimos_badge_create(parent, "game_sudoku");

    TEST_ASSERT_NOT_NULL(badge);
    TEST_ASSERT_FALSE(lv_obj_has_flag(badge, LV_OBJ_FLAG_CLICKABLE));
    TEST_ASSERT_EQUAL_PTR(&lv_image_class, lv_obj_get_class(badge));
    /* Nenhum wrapper: o icone em si nao tem nenhum filho por baixo dele. */
    TEST_ASSERT_EQUAL_UINT32(0, lv_obj_get_child_count(badge));

    lv_obj_delete(parent);
}

/*
 * Behavior 2: id sem correspondencia cai pro fallback de rotulo de texto
 * puro (icon_id tratado como o texto), tambem nao clicavel.
 */
void test_badge_unknown_id_falls_back_to_label(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * badge = ratimos_badge_create(parent, "unknown_id");

    TEST_ASSERT_NOT_NULL(badge);
    TEST_ASSERT_FALSE(lv_obj_has_flag(badge, LV_OBJ_FLAG_CLICKABLE));
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(badge));
    TEST_ASSERT_EQUAL_STRING("unknown_id", lv_label_get_text(badge));

    lv_obj_delete(parent);
}

/*
 * Behavior 3: icon_id NULL nunca e' desreferenciado -- cai pro mesmo
 * fallback de rotulo, mostrando "?", tambem nao clicavel.
 */
void test_badge_null_id_never_dereferences_and_falls_back(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * badge = ratimos_badge_create(parent, NULL);

    TEST_ASSERT_NOT_NULL(badge);
    TEST_ASSERT_FALSE(lv_obj_has_flag(badge, LV_OBJ_FLAG_CLICKABLE));
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(badge));
    TEST_ASSERT_EQUAL_STRING("?", lv_label_get_text(badge));

    lv_obj_delete(parent);
}

/*
 * Behavior 4: cada chamada acrescenta EXATAMENTE um objeto ao pai --
 * preserva o contrato de indice de filho badge=0/text_col=1 de
 * ratimos_row_create() (row_list.c) para todo chamador em cascata
 * (jogos_app.c, home_screen.c).
 */
void test_badge_create_adds_exactly_one_child_per_call(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);
    TEST_ASSERT_EQUAL_UINT32(0, lv_obj_get_child_count(parent));

    ratimos_badge_create(parent, "game_sudoku");
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(parent));

    ratimos_badge_create(parent, "unmatched");
    TEST_ASSERT_EQUAL_UINT32(2, lv_obj_get_child_count(parent));

    ratimos_badge_create(parent, NULL);
    TEST_ASSERT_EQUAL_UINT32(3, lv_obj_get_child_count(parent));

    lv_obj_delete(parent);
}

/*
 * ratimos_panel_create() (Task 2): cantos 100% retos, fundo levemente
 * translucido, borda externa escura de 2px (RATIMOS_COLOR_BEVEL_DARK) -- e
 * nenhum filho extra por baixo (friso interno via segundo lv_obj foi
 * explicitamente rejeitado, cards-superficies.md's fallback de borda
 * unica).
 */
void test_panel_create_has_square_translucent_dark_bevel(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * panel = ratimos_panel_create(parent);

    TEST_ASSERT_NOT_NULL(panel);
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_radius(panel, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_70, lv_obj_get_style_bg_opa(panel, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT(2, lv_obj_get_style_border_width(panel, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_BEVEL_DARK),
                              lv_color_to_u32(lv_obj_get_style_border_color(panel, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT32(0, lv_obj_get_child_count(panel));

    lv_obj_delete(parent);
}

/*
 * Segunda ocorrencia da MESMA classe de bug de hitbox que a Task 1 fechou
 * pro badge, encontrada em revisao manual (verificacao real no simulador
 * SDL2): ratimos_row_create() (row_list.c) cria `text_col` via
 * lv_obj_create(row) + lv_obj_remove_style_all(text_col) -- e
 * lv_obj_remove_style_all() so' remove ESTILOS, nunca flags (confirmado em
 * lv_obj_style.c:remove_style_core() vendorizado). text_col mantinha
 * LV_OBJ_FLAG_CLICKABLE default do LVGL sem nenhum handler proprio,
 * engolindo o toque sobre a area de titulo/subtitulo (a maior parte da
 * largura da linha) antes dele alcancar o click_cb de `row`. Fixado com um
 * lv_obj_clear_flag() explicito, mesmo padrao da correcao do badge.
 */
void test_row_create_text_col_is_not_clickable(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * row = ratimos_row_create(parent, "game_sudoku", "titulo", "subtitulo", NULL);

    TEST_ASSERT_NOT_NULL(row);
    lv_obj_t * text_col = lv_obj_get_child(row, 1);
    TEST_ASSERT_NOT_NULL(text_col);
    TEST_ASSERT_FALSE_MESSAGE(lv_obj_has_flag(text_col, LV_OBJ_FLAG_CLICKABLE),
                               "text_col must not be clickable -- it would swallow taps "
                               "over the title/subtitle area meant for the parent row");

    lv_obj_delete(parent);
}

/*
 * T-02.1-30: o fundo ditherizado (Task 3) e' decodificado a partir de uma
 * fonte 80x120 RGB565 (nao 320x480), mantendo o buffer de decode ~19KB em
 * vez de ~300KB (RGB565 = 2 bytes/pixel -- ver o comentario de formato em
 * theme.c/tools/convert_bg_dither.py para o porque RGB565 e nao o I4
 * indexado usado pelos icones). Prova empirica (nao so o calculo): constroi
 * e carrega pelo menos duas telas distintas via ratimos_theme_apply_screen()
 * (cada uma cria seu proprio lv_image de fundo) e confirma que o heap
 * builtin do LVGL (LV_MEM_SIZE, 512KB) continua com folga confortavel
 * depois de ambas decodificadas/cacheadas.
 */
void test_background_decode_across_two_screens_does_not_exhaust_heap(void)
{
    lv_obj_t * scr1 = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr1);
    lv_screen_load(scr1);
    lv_refr_now(NULL);

    lv_obj_t * scr2 = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr2);
    lv_screen_load(scr2);
    lv_refr_now(NULL);

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    TEST_ASSERT_TRUE_MESSAGE(mon.free_size > 100000,
                              "LVGL heap free_size dropped below 100000 bytes after "
                              "decoding the dithered background on two screens");

    lv_obj_delete(scr1);
    lv_obj_delete(scr2);
}

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    UNITY_BEGIN();
    RUN_TEST(test_badge_icon_match_is_bare_non_clickable_image);
    RUN_TEST(test_badge_unknown_id_falls_back_to_label);
    RUN_TEST(test_badge_null_id_never_dereferences_and_falls_back);
    RUN_TEST(test_badge_create_adds_exactly_one_child_per_call);
    RUN_TEST(test_panel_create_has_square_translucent_dark_bevel);
    RUN_TEST(test_row_create_text_col_is_not_clickable);
    RUN_TEST(test_background_decode_across_two_screens_does_not_exhaust_heap);
    return UNITY_END();
}
