/*
 * Suite do status_bar.c compartilhado (plano 02.1-10, Task 2) -- pina o
 * rework da sectionbar de "titulo plano" pra breadcrumb estilo explorador
 * de arquivos: sem cursor piscando e exatamente um filho no row (o antigo
 * checkmark LV_SYMBOL_OK foi removido). Plano 02.1-14 (sketch 003-C): o
 * caminho inteiro numa cor so' (RATIMOS_COLOR_TEXT), mono 11px -- o
 * segmento final vermelho saiu (ilegivel sobre o magenta do fundo).
 *
 * ratimos_sectionbar_create(parent, path) cria o ROW (a barra) como filho
 * de `parent`, e o label do caminho como filho do row -- os testes aqui
 * sempre buscam `lv_obj_get_child(parent, 0)` pro row e so' entao
 * `lv_obj_get_child(row, 0)` pro label.
 *
 * LVGL e' inicializado headless: um display 320x480 com buffer proprio e
 * um flush_cb que so' chama lv_display_flush_ready() -- sem SDL, mesma
 * tecnica de test_theme.c (plano 02.1-09).
 */
#include <unity.h>

#include "ratimos/status_bar.h"
#include "ratimos/theme.h"
#include "ratimos/fonts/ratimos_fonts.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

void setUp(void) {}
void tearDown(void) {}

/* Sketch 003-C: caminho em uma cor so' (TEXT), JetBrains Mono 11px. */
static void assert_single_color_mono11(lv_obj_t * label)
{
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_TEXT),
                              lv_color_to_u32(lv_obj_get_style_text_color(label, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_11, lv_obj_get_style_text_font(label, LV_PART_MAIN));
}

/*
 * Behavior 1: caminho de nivel unico ("./home") -- so' uma '/' (a do
 * prefixo), nada pra esmaecer nem destacar, texto inteiro em
 * RATIMOS_COLOR_TEXT. O row tem exatamente um filho (o label).
 */
void test_sectionbar_single_segment_is_plain_text_color_and_one_child(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    TEST_ASSERT_NOT_NULL(row);
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row));

    lv_obj_t * label = lv_obj_get_child(row, 0);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(label));
    TEST_ASSERT_FALSE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("./home", lv_label_get_text(label));
    assert_single_color_mono11(label);

    lv_obj_delete(parent);
}

/*
 * Behavior 2: caminho de 3 segmentos ("./home/jogos/conexo") -- texto puro,
 * sem markup de recolor, cor unica (nao ha mais segmento em destaque).
 */
void test_sectionbar_three_segment_path_is_single_color(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home/jogos/conexo");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row));

    lv_obj_t * label = lv_obj_get_child(row, 0);
    TEST_ASSERT_FALSE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("./home/jogos/conexo", lv_label_get_text(label));
    assert_single_color_mono11(label);

    lv_obj_delete(parent);
}

/*
 * Um caminho de profundidade 2 ("./home/jogos") tambem fica em cor unica.
 */
void test_sectionbar_two_segment_path_is_single_color(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home/jogos");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    lv_obj_t * label = lv_obj_get_child(row, 0);
    TEST_ASSERT_FALSE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("./home/jogos", lv_label_get_text(label));
    assert_single_color_mono11(label);

    lv_obj_delete(parent);
}

/*
 * Nenhum objeto extra e' inserido -- o antigo checkmark LV_SYMBOL_OK
 * (child 0 na versao anterior) nao existe mais; o unico filho do row e'
 * sempre o label do caminho.
 */
void test_sectionbar_never_creates_a_checkmark_sibling(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home/musica");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row));
    lv_obj_t * only_child = lv_obj_get_child(row, 0);
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(only_child));

    lv_obj_delete(parent);
}

/*
 * Behavior 3 (helper de atualizacao): ratimos_sectionbar_set_path() aplica
 * a MESMA formatacao a um label ja existente, sem duplicar a
 * logica de recoloracao (create() e set_path() convergem no mesmo
 * resultado pro mesmo path).
 */
void test_sectionbar_set_path_reformats_existing_label(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);
    ratimos_sectionbar_create(parent, "./home/jogos/termo");
    lv_obj_t * row = lv_obj_get_child(parent, 0);
    lv_obj_t * label = lv_obj_get_child(row, 0);

    ratimos_sectionbar_set_path(label, "./home/jogos/dueto");

    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row)); /* nenhum filho novo */
    TEST_ASSERT_FALSE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("./home/jogos/dueto", lv_label_get_text(label));
    assert_single_color_mono11(label);

    lv_obj_delete(parent);
}

/* Primeiro label filho direto de `obj` (ou NULL). */
static lv_obj_t * first_label_child(lv_obj_t * obj)
{
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        lv_obj_t * c = lv_obj_get_child(obj, (int32_t) i);
        if (lv_obj_get_class(c) == &lv_label_class) {
            return c;
        }
    }
    return NULL;
}

/*
 * Plano 02.1-14: topbar do sketch 003-C -- marca "RATIMOS" em caixa alta,
 * mono 10px, cor de texto clara (nao mais "RatimOS" vermelho em Montserrat).
 * Arvore: row -> [brand, right]; brand -> [logo_stack, label].
 */
void test_topbar_brand_is_uppercase_mono10_text_color(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_topbar_create(parent);

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    lv_obj_t * brand = lv_obj_get_child(row, 0);
    lv_obj_t * label = first_label_child(brand);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_EQUAL_STRING("RATIMOS", lv_label_get_text(label));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_10, lv_obj_get_style_text_font(label, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_TEXT),
                              lv_color_to_u32(lv_obj_get_style_text_color(label, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_INT(26, lv_obj_get_style_height(row, LV_PART_MAIN));

    lv_obj_delete(parent);
}

static void noop_cb(lv_event_t * e) { (void) e; }

/*
 * Plano 02.1-14: dica NULL na bottombar = nenhum label de dica (o row so'
 * tem o botao "voltar"); com dica, o label existe como antes.
 */
void test_bottombar_null_hint_creates_no_hint_label(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_bottombar_create(parent, "voltar", noop_cb, NULL);
    lv_obj_t * row = lv_obj_get_child(parent, 0);
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row));
    TEST_ASSERT_NULL(first_label_child(row));

    ratimos_bottombar_create(parent, "voltar", noop_cb, "fase 0 local");
    lv_obj_t * row2 = lv_obj_get_child(parent, 1);
    TEST_ASSERT_EQUAL_UINT32(2, lv_obj_get_child_count(row2));
    lv_obj_t * hint = first_label_child(row2);
    TEST_ASSERT_NOT_NULL(hint);
    TEST_ASSERT_EQUAL_STRING("fase 0 local", lv_label_get_text(hint));

    lv_obj_delete(parent);
}

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    UNITY_BEGIN();
    RUN_TEST(test_sectionbar_single_segment_is_plain_text_color_and_one_child);
    RUN_TEST(test_sectionbar_three_segment_path_is_single_color);
    RUN_TEST(test_sectionbar_two_segment_path_is_single_color);
    RUN_TEST(test_sectionbar_never_creates_a_checkmark_sibling);
    RUN_TEST(test_sectionbar_set_path_reformats_existing_label);
    RUN_TEST(test_topbar_brand_is_uppercase_mono10_text_color);
    RUN_TEST(test_bottombar_null_hint_creates_no_hint_label);
    return UNITY_END();
}
