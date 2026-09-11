/*
 * Suite do castelo_app.c (plano 02.1-11, Task 1) -- fecha o gap G-02.1-3 do
 * UAT (imagem do estagio renderizando em branco/vazio) com o PRIMEIRO teste
 * deste projeto que chama um entry point de producao completo
 * (ratimos_castelo_show()) contra um display LVGL headless real e inspeciona
 * pixels efetivamente renderizados no framebuffer -- nao uma reimplementacao
 * do widget tree nem um stub de storage.
 *
 * Tecnica headless: mesma de test_theme.c/test_status_bar.c (planos
 * 02.1-09/10) -- lv_init(), um display 320x480 com buffer proprio
 * (LV_DISPLAY_RENDER_MODE_FULL) e um flush_cb que so' chama
 * lv_display_flush_ready(). A diferenca aqui e' que o buffer NAO e'
 * descartado: e' o mesmo `s_disp_buf` que lv_display_set_buffers() usa como
 * superficie de renderizacao em modo FULL, entao depois de lv_timer_handler()
 * ele contem os pixels RGB565 (LV_COLOR_DEPTH 16) reais do frame desenhado --
 * lido diretamente, sem precisar copiar nada no flush_cb.
 *
 * Como platformio.ini usa test_build_src = yes, o game_state.c REAL (nao um
 * stub) esta linkado -- ratimos_storage_save_progression() grava um save de
 * verdade em assets/save/progression.bin antes de cada teste chamar
 * ratimos_castelo_show(), exercitando o MESMO caminho de leitura que uma
 * sessao interativa percorre (fechando a lacuna que o diagnostico headless
 * desta fase de planejamento, com storage stubado, tinha deixado aberta).
 *
 * Amostragem de pixel: SEMPRE via lv_obj_get_coords() (coordenadas
 * ABSOLUTAS de tela), nunca lv_obj_get_x()/lv_obj_get_y() (relativas ao
 * pai) -- essa troca foi exatamente o falso-positivo que a investigacao
 * desta fase de planejamento bateu e teve que debugar (ver
 * <investigation_notes> do plano 02.1-11).
 */
#include <string.h>
#include <unity.h>

#include "ratimos/apps/castelo_app.h"
#include "ratimos/theme.h"
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

/*
 * Grava uma progressao real (via a API publica de storage, nao um stub) com
 * `shared_completions` fixo -- last_win_day_index sempre RATIMOS_NO_WIN_DAY
 * (sentinela "nunca creditado"), sem depender de nenhum jogo real ter sido
 * jogado.
 */
static void save_progression_with_completions(uint16_t shared_completions)
{
    ratimos_storage_index_game_state();

    ratimos_progression_state_t state;
    memset(&state, 0, sizeof(state));
    state.shared_completions = shared_completions;
    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        state.last_win_day_index[i] = RATIMOS_NO_WIN_DAY;
    }

    TEST_ASSERT_TRUE(ratimos_storage_save_progression(&state));
}

/*
 * Forca layout + paint reais -- ratimos_castelo_show() so agenda a
 * invalidacao/troca de tela, quem efetivamente desenha e' o timer handler.
 */
static void pump_render(void)
{
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
    }
}

/*
 * Layout conhecido (app_shell.c + castelo_app.c + theme.c, verificado nesta
 * execucao): ratimos_theme_apply_screen() (plano 02.1-09 Task 3) agora
 * insere o bitmap de fundo ditherizado como o PRIMEIRO filho de
 * shell.screen (LV_OBJ_FLAG_FLOATING, fora do layout flex-column) --
 * shell.screen recebe, NESTA ORDEM, fundo + topbar/sectionbar (chrome,
 * planos 02.1-09/02.1-10) + shell.content + bottombar, entao content e'
 * agora o QUARTO filho (indice 3), nao mais o terceiro (indice 2 era valido
 * antes do plano 02.1-09 Task 3 existir). castelo_app.c constroi
 * s_stage_image como o PRIMEIRO filho de shell.content (indice 0), antes da
 * legenda/conquistas -- essa parte continua inalterada, so' o indice de
 * `content` dentro de `scr` mudou. Mesmo assim, cada passo abaixo falha
 * alto (TEST_ASSERT) em vez de silenciosamente amostrar o widget errado
 * caso essa suposicao pare de valer no futuro.
 */
static lv_obj_t * find_stage_image(void)
{
    lv_obj_t * scr = lv_screen_active();
    TEST_ASSERT_NOT_NULL(scr);
    TEST_ASSERT_TRUE(lv_obj_get_child_count(scr) >= 4);

    lv_obj_t * content = lv_obj_get_child(scr, 3);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(lv_obj_get_child_count(content) >= 1);

    lv_obj_t * stage_image = lv_obj_get_child(content, 0);
    TEST_ASSERT_NOT_NULL(stage_image);
    TEST_ASSERT_EQUAL_PTR(&lv_image_class, lv_obj_get_class(stage_image));

    return stage_image;
}

static uint16_t read_pixel_rgb565(int32_t x, int32_t y)
{
    size_t idx = ((size_t) y * (size_t) RATIMOS_SCREEN_W + (size_t) x) * 2u;
    return (uint16_t) (s_disp_buf[idx] | (uint16_t) (s_disp_buf[idx + 1] << 8));
}

/*
 * Conta valores RGB565 distintos dentro de `area` (coordenadas absolutas de
 * tela), amostrando de 3 em 3 pixels em cada eixo -- suficiente para
 * distinguir "arte multi-cor real" de "preenchimento uniforme
 * branco/vazio", sem precisar varrer cada pixel do bitmap 288x180.
 */
static size_t count_distinct_colors_in_area(const lv_area_t * area)
{
    uint16_t seen[256];
    size_t seen_count = 0;

    for (int32_t y = area->y1; y <= area->y2; y += 3) {
        if (y < 0 || y >= RATIMOS_SCREEN_H) {
            continue;
        }
        for (int32_t x = area->x1; x <= area->x2; x += 3) {
            if (x < 0 || x >= RATIMOS_SCREEN_W) {
                continue;
            }

            uint16_t color = read_pixel_rgb565(x, y);

            bool already_seen = false;
            for (size_t i = 0; i < seen_count; i++) {
                if (seen[i] == color) {
                    already_seen = true;
                    break;
                }
            }

            if (!already_seen && seen_count < (sizeof(seen) / sizeof(seen[0]))) {
                seen[seen_count++] = color;
            }
        }
    }

    return seen_count;
}

/*
 * Behavior 1 (comportamento principal deste gap): renderiza pelo menos 3
 * valores RGB565 distintos dentro da moldura do estagio para
 * shared_completions=0 -- um retangulo branco/vazio produziria exatamente 1
 * valor distinto.
 */
void test_stage_image_renders_multiple_colors_for_completions_zero(void)
{
    save_progression_with_completions(0);

    ratimos_castelo_show(NULL);
    pump_render();

    lv_obj_t * stage_image = find_stage_image();
    TEST_ASSERT_FALSE(lv_obj_has_flag(stage_image, LV_OBJ_FLAG_HIDDEN));

    lv_area_t area;
    lv_obj_get_coords(stage_image, &area);
    TEST_ASSERT_TRUE(area.x2 > area.x1);
    TEST_ASSERT_TRUE(area.y2 > area.y1);

    size_t distinct = count_distinct_colors_in_area(&area);
    TEST_ASSERT_TRUE(distinct >= 3);
}

/*
 * Behavior 2: a mesma asserção vale para um shared_completions que resolve
 * pra um estagio DIFERENTE (12 -> "castle_stage_02_muros_canteiro", ver
 * test_progression.c) -- prova que o fix nao e' especifico do primeiro
 * asset.
 */
void test_stage_image_renders_multiple_colors_for_completions_twelve(void)
{
    save_progression_with_completions(12);

    ratimos_castelo_show(NULL);
    pump_render();

    lv_obj_t * stage_image = find_stage_image();
    TEST_ASSERT_FALSE(lv_obj_has_flag(stage_image, LV_OBJ_FLAG_HIDDEN));

    lv_area_t area;
    lv_obj_get_coords(stage_image, &area);
    TEST_ASSERT_TRUE(area.x2 > area.x1);
    TEST_ASSERT_TRUE(area.y2 > area.y1);

    size_t distinct = count_distinct_colors_in_area(&area);
    TEST_ASSERT_TRUE(distinct >= 3);
}

/*
 * Behavior 3: o objeto de imagem do estagio nunca fica LV_OBJ_FLAG_HIDDEN
 * para nenhum shared_completions valido (dentro do range), incluindo os
 * limites 0 e o valor de conclusao (30) e um valor bem alem do maior
 * threshold do manifesto (9999, clampado por
 * ratimos_progression_stage_index()).
 */
void test_stage_image_never_hidden_for_any_valid_completions(void)
{
    const uint16_t values[] = { 0, 5, 6, 12, 29, 30, 9999 };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        save_progression_with_completions(values[i]);

        ratimos_castelo_show(NULL);
        pump_render();

        lv_obj_t * stage_image = find_stage_image();
        TEST_ASSERT_FALSE(lv_obj_has_flag(stage_image, LV_OBJ_FLAG_HIDDEN));
    }
}

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_display_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_display_set_buffers(disp, s_disp_buf, NULL, sizeof(s_disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, headless_flush_cb);

    UNITY_BEGIN();
    RUN_TEST(test_stage_image_renders_multiple_colors_for_completions_zero);
    RUN_TEST(test_stage_image_renders_multiple_colors_for_completions_twelve);
    RUN_TEST(test_stage_image_never_hidden_for_any_valid_completions);
    return UNITY_END();
}
