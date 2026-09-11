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
    return UNITY_END();
}
