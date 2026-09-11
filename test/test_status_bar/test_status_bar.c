/*
 * Suite do status_bar.c compartilhado (plano 02.1-10, Task 2) -- pina o
 * rework da sectionbar de "titulo plano" pra breadcrumb estilo explorador
 * de arquivos: segmentos-pai esmaecidos, segmento atual em destaque, sem
 * cursor piscando, e exatamente um filho no row (o antigo checkmark
 * LV_SYMBOL_OK foi removido).
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
    TEST_ASSERT_TRUE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("#f5f2f8 ./home#", lv_label_get_text(label));

    lv_obj_delete(parent);
}

/*
 * Behavior 2: caminho de 3 segmentos ("./home/jogos/conexo") -- tudo ate a
 * ULTIMA '/' (inclusive) esmaecido, o segmento final ("conexo") em
 * destaque, como um unico label recolorido.
 */
void test_sectionbar_three_segment_path_splits_muted_and_accent(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home/jogos/conexo");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(row));

    lv_obj_t * label = lv_obj_get_child(row, 0);
    TEST_ASSERT_TRUE(lv_label_get_recolor(label));
    TEST_ASSERT_EQUAL_STRING("#a997ba ./home/jogos/##e6010f conexo#", lv_label_get_text(label));

    lv_obj_delete(parent);
}

/*
 * Um caminho de profundidade 2 ("./home/jogos") tambem dispara o split
 * (>=2 ocorrencias de '/') -- confirma que a regra nao e' hardcoded so'
 * pro caso de 3 segmentos.
 */
void test_sectionbar_two_segment_path_also_splits(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    ratimos_sectionbar_create(parent, "./home/jogos");

    lv_obj_t * row = lv_obj_get_child(parent, 0);
    lv_obj_t * label = lv_obj_get_child(row, 0);
    TEST_ASSERT_EQUAL_STRING("#a997ba ./home/##e6010f jogos#", lv_label_get_text(label));

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
 * a MESMA formatacao muted/atual a um label ja existente, sem duplicar a
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
    TEST_ASSERT_EQUAL_STRING("#a997ba ./home/jogos/##e6010f dueto#", lv_label_get_text(label));

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
    RUN_TEST(test_sectionbar_three_segment_path_splits_muted_and_accent);
    RUN_TEST(test_sectionbar_two_segment_path_also_splits);
    RUN_TEST(test_sectionbar_never_creates_a_checkmark_sibling);
    RUN_TEST(test_sectionbar_set_path_reformats_existing_label);
    return UNITY_END();
}
