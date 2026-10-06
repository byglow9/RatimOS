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
 * ratimos_panel_create(): cantos 100% retos, fundo levemente translucido,
 * borda externa escura de 2px (RATIMOS_COLOR_BEVEL_DARK) -- e nenhum filho
 * extra por baixo (o friso interno da variante 003-C e' desenhado num
 * callback LV_EVENT_DRAW_POST, plano 02.1-13, nunca num segundo lv_obj).
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
 * Plano 02.1-13, Task 1: o painel recem-criado continua sem NENHUM filho
 * depois de ganhar o friso interno -- chamadores (row_list.c, jogos_app.c,
 * termo.c) leem filhos por indice posicional.
 */
void test_panel_bevel_adds_no_child_objects(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * panel = ratimos_panel_create(parent);

    TEST_ASSERT_EQUAL_UINT32(0, lv_obj_get_child_count(panel));
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(parent));

    lv_obj_delete(parent);
}

/* Luminancia aproximada de um pixel RGB565 (soma dos canais em 8 bits). */
static uint32_t rgb565_luma(uint16_t c)
{
    uint32_t r = ((c >> 11) & 0x1F) << 3;
    uint32_t g = ((c >> 5) & 0x3F) << 2;
    uint32_t b = (c & 0x1F) << 3;
    return r + g + b;
}

static uint16_t read_pixel_rgb565(int32_t x, int32_t y)
{
    size_t idx = ((size_t) y * (size_t) RATIMOS_SCREEN_W + (size_t) x) * 2u;
    return (uint16_t) (s_disp_buf[idx] | (uint16_t) (s_disp_buf[idx + 1] << 8));
}

/*
 * Plano 02.1-13, Task 1: renderiza um painel 100x40 num display headless
 * (mesmo harness de amostragem do test_castelo_render: buffer FULL lido
 * direto depois de lv_refr_now) e confirma a anatomia da variante 003-C:
 *   y+0..1 -> borda externa escura (BEVEL_DARK)
 *   y+2    -> friso claro de 1px (branco translucido)
 *   y+3..4 -> faixa escura de 2px (preto translucido)
 *   centro -> fundo roxo translucido liso
 * O friso tem que ser mais claro que a faixa escura E que o centro.
 */
void test_panel_bevel_renders_light_frieze_inside_dark_border(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t * panel = ratimos_panel_create(scr);
    lv_obj_set_size(panel, 100, 40);
    lv_obj_set_pos(panel, 20, 20);

    lv_screen_load(scr);
    lv_refr_now(NULL);

    lv_area_t c;
    lv_obj_get_coords(panel, &c);
    int32_t mx = (c.x1 + c.x2) / 2;
    int32_t my = (c.y1 + c.y2) / 2;

    uint32_t frieze = rgb565_luma(read_pixel_rgb565(mx, c.y1 + 2));
    uint32_t band   = rgb565_luma(read_pixel_rgb565(mx, c.y1 + 4));
    uint32_t center = rgb565_luma(read_pixel_rgb565(mx, my));

    TEST_ASSERT_TRUE_MESSAGE(frieze > band,
                              "inner light frieze (y+2) must be lighter than the dark band (y+4)");
    TEST_ASSERT_TRUE_MESSAGE(frieze > center,
                              "inner light frieze (y+2) must be lighter than the panel center");
    /* Mesma anatomia na lateral esquerda (o friso e' uma moldura, nao so' uma linha). */
    uint32_t frieze_l = rgb565_luma(read_pixel_rgb565(c.x1 + 2, my));
    uint32_t band_l   = rgb565_luma(read_pixel_rgb565(c.x1 + 4, my));
    TEST_ASSERT_TRUE_MESSAGE(frieze_l > band_l,
                              "left inner frieze (x+2) must be lighter than the left dark band (x+4)");

    lv_obj_delete(scr);
}

/*
 * Plano 02.1-13, Task 2: ratimos_button_create() produz a MESMA arvore dos
 * make_pill() locais que substitui (painel -> 1 label), com o bevel 003-C
 * completo e o label nao-clicavel (licao de hitbox do 02.1-09).
 */
void test_button_create_is_bevel_panel_with_single_unclickable_label(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);

    lv_obj_t * btn = ratimos_button_create(parent, "novo jogo", NULL, 120, 32);

    TEST_ASSERT_NOT_NULL(btn);
    TEST_ASSERT_TRUE(lv_obj_has_flag(btn, LV_OBJ_FLAG_CLICKABLE));
    TEST_ASSERT_FALSE(lv_obj_has_flag(btn, LV_OBJ_FLAG_SCROLLABLE));
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_radius(btn, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT(2, lv_obj_get_style_border_width(btn, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_BEVEL_DARK),
                              lv_color_to_u32(lv_obj_get_style_border_color(btn, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT32(1, lv_obj_get_child_count(btn));
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(label));
    TEST_ASSERT_EQUAL_STRING("novo jogo", lv_label_get_text(label));
    TEST_ASSERT_FALSE(lv_obj_has_flag(label, LV_OBJ_FLAG_CLICKABLE));

    lv_obj_delete(parent);
}

/*
 * Plano 02.1-13, Task 2: selecionar/desselecionar um botao muda so' fundo e
 * cor do texto -- a moldura (borda 2px BEVEL_DARK, radius 0) nunca some
 * (o render_pills antigo do sudoku zerava a borda dos nao-selecionados).
 */
void test_button_set_selected_never_touches_the_bevel_frame(void)
{
    lv_obj_t * parent = lv_obj_create(NULL);
    lv_obj_t * btn = ratimos_button_create(parent, "medio", NULL, 70, 28);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    ratimos_button_set_selected(btn, true);
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL_ACTIVE),
                              lv_color_to_u32(lv_obj_get_style_bg_color(btn, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_ACCENT),
                              lv_color_to_u32(lv_obj_get_style_text_color(label, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_INT(2, lv_obj_get_style_border_width(btn, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_radius(btn, LV_PART_MAIN));

    ratimos_button_set_selected(btn, false);
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_PANEL),
                              lv_color_to_u32(lv_obj_get_style_bg_color(btn, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_TEXT_MUTED),
                              lv_color_to_u32(lv_obj_get_style_text_color(label, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_INT(2, lv_obj_get_style_border_width(btn, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_BEVEL_DARK),
                              lv_color_to_u32(lv_obj_get_style_border_color(btn, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_radius(btn, LV_PART_MAIN));

    lv_obj_delete(parent);
}

/*
 * Plano 02.1-13, Task 2: teclas de lv_buttonmatrix com o bevel 003-C --
 * radius 0, translucidas (nunca LV_OPA_COVER), borda 2px BEVEL_DARK -- e o
 * friso interno realmente desenhado em cada tecla (amostragem de pixel).
 */
void test_buttonmatrix_keys_get_square_translucent_bevel_with_frieze(void)
{
    static const char * const map[] = { "a", "b", NULL };

    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t * m = lv_buttonmatrix_create(scr);
    lv_buttonmatrix_set_map(m, map);
    lv_obj_set_size(m, 200, 40);
    lv_obj_set_pos(m, 20, 100);
    lv_obj_set_style_pad_column(m, 4, 0);
    ratimos_bevel_style_buttonmatrix(m);

    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_radius(m, LV_PART_ITEMS));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_70, lv_obj_get_style_bg_opa(m, LV_PART_ITEMS));
    TEST_ASSERT_EQUAL_INT(2, lv_obj_get_style_border_width(m, LV_PART_ITEMS));
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(RATIMOS_COLOR_BEVEL_DARK),
                              lv_color_to_u32(lv_obj_get_style_border_color(m, LV_PART_ITEMS)));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_TRANSP, lv_obj_get_style_bg_opa(m, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_style_border_width(m, LV_PART_MAIN));

    lv_screen_load(scr);
    lv_refr_now(NULL);

    /* Primeira tecla: com pad 0 do container ela comeca na borda esquerda da
     * matriz; amostra no meio da altura. */
    lv_area_t c;
    lv_obj_get_coords(m, &c);
    int32_t my = (c.y1 + c.y2) / 2;
    uint32_t frieze = rgb565_luma(read_pixel_rgb565(c.x1 + 2, my));
    uint32_t band   = rgb565_luma(read_pixel_rgb565(c.x1 + 4, my));
    uint32_t center = rgb565_luma(read_pixel_rgb565(c.x1 + 20, c.y1 + 10));
    TEST_ASSERT_TRUE_MESSAGE(frieze > band,
                              "key inner frieze (x+2) must be lighter than its dark band (x+4)");
    TEST_ASSERT_TRUE_MESSAGE(frieze > center,
                              "key inner frieze (x+2) must be lighter than the key fill");

    lv_obj_delete(scr);
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


/*
 * Plano 02.1-14, Task 1: a tipografia do sketch 003-C e' o padrao -- um
 * label sem fonte explicita herda JetBrains Mono 12 (LV_FONT_DEFAULT via
 * tema default do LVGL), nao mais Montserrat (D-08 antigo).
 */
void test_default_theme_font_is_jetbrains_mono_12(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_t * lbl = lv_label_create(scr);

    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_12, LV_FONT_DEFAULT);
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_12, lv_obj_get_style_text_font(lbl, LV_PART_MAIN));

    lv_obj_delete(scr);
}

/* Glifo resolvido na PROPRIA fonte (sem cair em fallback). */
static bool font_has_own_glyph(const lv_font_t * font, uint32_t cp)
{
    lv_font_glyph_dsc_t dsc;
    bool ok = lv_font_get_glyph_dsc(font, &dsc, cp, 0);
    return ok && dsc.resolved_font == font;
}

/*
 * As descricoes de linha usam "·" (U+00B7) e o texto PT-BR usa acentos:
 * as tres mono tem os glifos de verdade (nao caixa vazia, nao fallback).
 */
void test_mono_fonts_have_middle_dot_and_pt_br_glyphs(void)
{
    const lv_font_t * fonts[] = { &ratimos_font_mono_10, &ratimos_font_mono_11, &ratimos_font_mono_12 };
    const uint32_t cps[] = { 0x00B7 /* · */, 0x00E7 /* ç */, 0x00E3 /* ã */, 0x00E9 /* é */, 'a', '9' };
    for (size_t f = 0; f < sizeof(fonts) / sizeof(fonts[0]); f++) {
        for (size_t c = 0; c < sizeof(cps) / sizeof(cps[0]); c++) {
            TEST_ASSERT_TRUE_MESSAGE(font_has_own_glyph(fonts[f], cps[c]),
                                     "mono font is missing a glyph the UI uses");
        }
    }
    TEST_ASSERT_TRUE(font_has_own_glyph(&ratimos_font_title_8, 'a'));
    TEST_ASSERT_TRUE(font_has_own_glyph(&ratimos_font_title_8, 0x00EA /* ê */));
}

/*
 * LV_SYMBOL_* (area de uso privado do FontAwesome) nao existe na
 * JetBrains Mono: o fallback das mono pra Montserrat 14 garante que o
 * "voltar" da bottombar (LV_SYMBOL_LEFT) nunca vira caixa vazia.
 */
void test_mono_font_symbols_resolve_through_montserrat_fallback(void)
{
    const lv_font_t * fonts[] = { &ratimos_font_mono_10, &ratimos_font_mono_11, &ratimos_font_mono_12 };
    for (size_t f = 0; f < sizeof(fonts) / sizeof(fonts[0]); f++) {
        lv_font_glyph_dsc_t dsc;
        TEST_ASSERT_TRUE(lv_font_get_glyph_dsc(fonts[f], &dsc, 0xF053 /* LV_SYMBOL_LEFT */, 0));
        TEST_ASSERT_EQUAL_PTR(&lv_font_montserrat_14, dsc.resolved_font);
        TEST_ASSERT_TRUE(dsc.box_w > 0);
    }
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
    RUN_TEST(test_panel_bevel_adds_no_child_objects);
    RUN_TEST(test_panel_bevel_renders_light_frieze_inside_dark_border);
    RUN_TEST(test_button_create_is_bevel_panel_with_single_unclickable_label);
    RUN_TEST(test_button_set_selected_never_touches_the_bevel_frame);
    RUN_TEST(test_buttonmatrix_keys_get_square_translucent_bevel_with_frieze);
    RUN_TEST(test_row_create_text_col_is_not_clickable);
    RUN_TEST(test_background_decode_across_two_screens_does_not_exhaust_heap);
    RUN_TEST(test_default_theme_font_is_jetbrains_mono_12);
    RUN_TEST(test_mono_fonts_have_middle_dot_and_pt_br_glyphs);
    RUN_TEST(test_mono_font_symbols_resolve_through_montserrat_fallback);
    return UNITY_END();
}
