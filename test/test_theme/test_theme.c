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
#include <string.h>
#include <unity.h>

#include "ratimos/theme.h"
#include "ratimos/row_list.h"
#include "ratimos/bg_images.h"
#include "ratimos/fonts/ratimos_fonts.h"
#include "ratimos/splash.h"
#include "ratimos/icons.h"
#include "ratimos/theme_catalog.h"
#include "ratimos/home_screen.h"
#include "ratimos/app_shell.h"
#include "ratimos/theme_art.h"
#include "ratimos/heap_probe.h"
#include "storage/content_api.h"

static uint8_t s_disp_buf[RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2]; /* LV_COLOR_DEPTH 16 */

static void headless_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void) area;
    (void) px_map;
    lv_display_flush_ready(disp);
}

/* Este binario le o assets/save/ real: o tema aplicado em memoria e' fixado no
 * Classico para que os testes de fundo nao dependam do que esta salvo. */
void setUp(void)
{
    ratimos_theme_preview_end();
    ratimos_theme_apply(RATIMOS_THEME_CLASSICO);
}

void tearDown(void)
{
    ratimos_theme_preview_end();
}

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
 * T-02.1-30 (revisto no 02.1-14): o fundo ditherizado agora e' 320x480
 * RGB565 desenhado 1:1 direto da flash (array const, sem decode -- ver
 * theme.c/tools/convert_bg_dither.py). Prova empirica (nao so o calculo): constroi
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

/*
 * Plano 02.1-14, Task 3: o fundo e' o bitmap do sketch 001-C em resolucao
 * cheia (320x480 RGB565), desenhado 1:1 -- sem LV_IMAGE_ALIGN_STRETCH nem
 * escala (o stretch 4x de uma fonte 80x120 engordava as scanlines).
 */
void test_background_is_full_frame_and_drawn_one_to_one(void)
{
    TEST_ASSERT_EQUAL_UINT32(RATIMOS_SCREEN_W, ratimos_bg_dither_desc.header.w);
    TEST_ASSERT_EQUAL_UINT32(RATIMOS_SCREEN_H, ratimos_bg_dither_desc.header.h);
    TEST_ASSERT_EQUAL_UINT32(LV_COLOR_FORMAT_RGB565, ratimos_bg_dither_desc.header.cf);
    TEST_ASSERT_EQUAL_UINT32(RATIMOS_SCREEN_W * RATIMOS_SCREEN_H * 2u, ratimos_bg_dither_desc.data_size);

    lv_obj_t * scr = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr);
    lv_obj_t * bg = lv_obj_get_child(scr, 0);
    TEST_ASSERT_EQUAL_PTR(&lv_image_class, lv_obj_get_class(bg));
    TEST_ASSERT_EQUAL_PTR(&ratimos_bg_dither_desc, lv_image_get_src(bg));
    TEST_ASSERT_NOT_EQUAL(LV_IMAGE_ALIGN_STRETCH, lv_image_get_inner_align(bg));
    TEST_ASSERT_EQUAL_INT(LV_SCALE_NONE, lv_image_get_scale_x(bg));
    TEST_ASSERT_EQUAL_INT(LV_SCALE_NONE, lv_image_get_scale_y(bg));

    lv_obj_delete(scr);
}

/*
 * Plano 02.1-14, Task 4: linha do sketch 003-C (icone 26px + titulo +
 * descricao) mede ~46-48px -- nunca os ~150px de antes (text_col sem
 * altura herdava LV_DPI_DEF). Titulo em Press Start 2P 8px, descricao em
 * mono 10, text_col com altura de conteudo.
 */
void test_row_with_icon_and_two_labels_is_compact(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_t * list = lv_obj_create(scr);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 300, 400);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    /* Descricao longa de proposito (o "continuar · ..." do launcher): tem
     * que cortar com "..." numa linha, nunca quebrar e esticar a linha. */
    lv_obj_t * row = ratimos_row_create(list, "game_sudoku", "sudoku",
                                        "continuar · grade 9x9 · fácil, médio, difícil", NULL);
    lv_obj_update_layout(scr);

    int32_t h = lv_obj_get_height(row);
    TEST_ASSERT_TRUE_MESSAGE(h >= 40 && h <= 56, "list row must be 40..56px tall (sketch 003-C ~46px)");

    lv_obj_t * icon = lv_obj_get_child(row, 0);
    TEST_ASSERT_EQUAL_PTR(&lv_image_class, lv_obj_get_class(icon));
    TEST_ASSERT_EQUAL_INT(26, lv_obj_get_width(icon));
    TEST_ASSERT_EQUAL_INT(26, lv_obj_get_height(icon));

    lv_obj_t * text_col = lv_obj_get_child(row, 1);
    TEST_ASSERT_TRUE(lv_obj_get_height(text_col) < 40);
    lv_obj_t * title = lv_obj_get_child(text_col, 0);
    lv_obj_t * sub = lv_obj_get_child(text_col, 1);
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_title_8, lv_obj_get_style_text_font(title, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_mono_10, lv_obj_get_style_text_font(sub, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT(lv_font_get_line_height(&ratimos_font_mono_10), lv_obj_get_height(sub));
    TEST_ASSERT_EQUAL_INT(lv_font_get_line_height(&ratimos_font_title_8), lv_obj_get_height(title));

    lv_obj_delete(scr);
}

/*
 * Plano 02.1-14, Task 4: todo icon id que os apps de lista passam pra
 * ratimos_row_create() existe compilado (nenhuma letra crua como "!" ou
 * "B" no lugar de icone) e os icones de linha sao 26x26 nativos.
 */
void test_row_icon_ids_used_by_apps_are_compiled_26px_icons(void)
{
    const char * ids[] = {
        "game_sudoku", "game_paciencia", "game_termo", "game_cruzadinha", "game_conexo",
        "row_empty", "row_carta", "row_playlist", "row_track",
        "cfg_brilho", "cfg_volume", "cfg_firmware", "cfg_armazenamento",
    };
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        const lv_image_dsc_t * d = ratimos_icon_by_id(ids[i]);
        TEST_ASSERT_NOT_NULL_MESSAGE(d, ids[i]);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(26, d->header.w, ids[i]);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(26, d->header.h, ids[i]);
    }
}

/*
 * Plano 02.1-16, Task 3: o fundo do sistema (home e telas de app) e' o do
 * tema EXIBIDO -- o aplicado, ou o da previa em andamento. Com o Classico o
 * resultado e' identico ao de antes (bg_dither, sem recolor).
 */
static lv_obj_t * bg_of_new_screen(void)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    ratimos_theme_apply_screen(scr);
    return scr;
}

void test_classico_system_background_is_identical_to_before(void)
{
    lv_obj_t * scr = bg_of_new_screen();
    lv_obj_t * bg = lv_obj_get_child(scr, 0);
    TEST_ASSERT_EQUAL_PTR(&ratimos_bg_dither_desc, lv_image_get_src(bg));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_TRANSP, lv_obj_get_style_image_recolor_opa(bg, LV_PART_MAIN));
    lv_obj_delete(scr);
}

void test_system_background_follows_the_displayed_theme_and_its_dim(void)
{
    ratimos_theme_preview_begin(RATIMOS_THEME_CASTELO);
    lv_obj_t * scr = bg_of_new_screen();
    lv_obj_t * bg = lv_obj_get_child(scr, 0);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_castelo_desc, lv_image_get_src(bg));
    /* escurecimento de 20% em tempo de execucao: recolor preto a 20% */
    TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(lv_color_black()),
                              lv_color_to_u32(lv_obj_get_style_image_recolor(bg, LV_PART_MAIN)));
    TEST_ASSERT_EQUAL_UINT8(20 * 255 / 100, lv_obj_get_style_image_recolor_opa(bg, LV_PART_MAIN));
    lv_obj_delete(scr);

    ratimos_theme_preview_begin(RATIMOS_THEME_NOITE);
    scr = bg_of_new_screen();
    bg = lv_obj_get_child(scr, 0);
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_noite_desc, lv_image_get_src(bg));
    TEST_ASSERT_EQUAL_UINT8(LV_OPA_TRANSP, lv_obj_get_style_image_recolor_opa(bg, LV_PART_MAIN));
    lv_obj_delete(scr);

    ratimos_theme_preview_begin(RATIMOS_THEME_XADREZ);
    scr = bg_of_new_screen();
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_xadrez_desc, lv_image_get_src(lv_obj_get_child(scr, 0)));
    lv_obj_delete(scr);
}

void test_preview_changes_the_displayed_theme_only(void)
{
    ratimos_theme_id_t stored = ratimos_storage_get_theme_id();
    uint32_t gen = ratimos_theme_generation();

    ratimos_theme_preview_begin(RATIMOS_THEME_XADREZ);
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_theme_displayed()->id);
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_applied()->id);
    TEST_ASSERT_TRUE(ratimos_theme_generation() != gen);
    TEST_ASSERT_EQUAL_INT_MESSAGE(stored, ratimos_storage_get_theme_id(), "a preview must not write the storage");

    ratimos_theme_preview_end();
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_displayed()->id);
}

void test_theme_apply_raises_the_generation_and_the_home_rebuilds_with_the_new_background(void)
{
    ratimos_home_screen_show(NULL);
    lv_obj_t * home1 = lv_screen_active();
    TEST_ASSERT_EQUAL_PTR(&ratimos_bg_dither_desc, lv_image_get_src(lv_obj_get_child(home1, 0)));

    ratimos_home_screen_show(NULL);
    TEST_ASSERT_EQUAL_PTR_MESSAGE(home1, lv_screen_active(), "the home must stay cached while the theme is unchanged");

    uint32_t gen = ratimos_theme_generation();
    ratimos_theme_apply(RATIMOS_THEME_NOITE);
    TEST_ASSERT_TRUE(ratimos_theme_generation() > gen);

    size_t heap_before = ratimos_heap_used();
    ratimos_home_screen_show(NULL);
    lv_obj_t * home2 = lv_screen_active();
    TEST_ASSERT_TRUE_MESSAGE(home2 != home1, "the home must be rebuilt after a theme change");
    TEST_ASSERT_EQUAL_PTR(&ratimos_home_bg_noite_desc, lv_image_get_src(lv_obj_get_child(home2, 0)));
    TEST_ASSERT_TRUE_MESSAGE(ratimos_heap_used() <= heap_before + 2048, "the stale home was not deleted");

    /* e volta ao Classico sem vazar */
    ratimos_theme_apply(RATIMOS_THEME_CLASSICO);
    ratimos_home_screen_show(NULL);
    TEST_ASSERT_EQUAL_PTR(&ratimos_bg_dither_desc, lv_image_get_src(lv_obj_get_child(lv_screen_active(), 0)));
    TEST_ASSERT_TRUE(ratimos_heap_used() <= heap_before + 2048);
}

void test_invalid_theme_ids_fall_back_to_classico_and_are_never_applied(void)
{
    TEST_ASSERT_EQUAL_PTR(ratimos_theme_get(RATIMOS_THEME_CLASSICO), ratimos_theme_get((ratimos_theme_id_t) 77));
    TEST_ASSERT_EQUAL_PTR(ratimos_theme_get(RATIMOS_THEME_CLASSICO), ratimos_theme_get((ratimos_theme_id_t) -1));
    uint32_t gen = ratimos_theme_generation();
    ratimos_theme_apply((ratimos_theme_id_t) 99);
    ratimos_theme_preview_begin((ratimos_theme_id_t) 99);
    TEST_ASSERT_EQUAL_UINT32(gen, ratimos_theme_generation());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_theme_displayed()->id);
}

/*
 * Plano 02.1-16: a tela de boot e' OPACA e nao carrega nenhum lv_image --
 * o fundo do tema e o carregamento sao desenhados pelo "stage" (nenhum objeto
 * por quadro). O boot preto liso do 02.1-14 foi substituido pelo fundo do
 * tema, a pedido da usuaria; os pixels de cada tema/carregamento sao
 * verificados em test_boot_screen. Aqui os testes ficam independentes de
 * qual tema/carregamento esta salvo (este binario le o assets/save/ real).
 * Fica por ultimo na suite: a splash agenda lv_timers de boot que os outros
 * testes nao precisam ver.
 */
void test_splash_screen_is_opaque_without_images_and_starts_at_zero(void)
{
    lv_obj_t * prev = lv_screen_active();
    ratimos_splash_show();
    lv_obj_t * scr = lv_screen_active();
    TEST_ASSERT_TRUE(scr != prev);

    TEST_ASSERT_EQUAL_UINT8(LV_OPA_COVER, lv_obj_get_style_bg_opa(scr, LV_PART_MAIN));
    for (uint32_t i = 0; i < lv_obj_get_child_count(scr); i++) {
        lv_obj_t * c = lv_obj_get_child(scr, (int32_t) i);
        TEST_ASSERT_TRUE_MESSAGE(lv_obj_get_class(c) != &lv_image_class,
                                 "splash must not carry any lv_image child");
    }

    /* Percentual em Press Start 2P 8px, comecando em "000%" (o carregamento
     * padrao nao tem prefixo). filho 0 = stage, 1 = sombra, 2 = texto. */
    lv_obj_t * pct = lv_obj_get_child(scr, 2);
    TEST_ASSERT_EQUAL_PTR(&lv_label_class, lv_obj_get_class(pct));
    TEST_ASSERT_EQUAL_PTR(&ratimos_font_title_8, lv_obj_get_style_text_font(pct, LV_PART_MAIN));
    TEST_ASSERT_NOT_NULL(strstr(lv_label_get_text(pct), "000%"));
    TEST_ASSERT_EQUAL_UINT8(0, ratimos_splash_units_shown());
}

/* Avanca o relogio do LVGL em passos de 10ms rodando os timers. */
static void pump_ms(int ms)
{
    for (int t = 0; t < ms; t += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

/*
 * Preenchimento por UNIDADES, sempre atras do progresso REAL (shown <= alvo
 * dos passos de boot concluidos, nunca a frente), no maximo 1 por tick;
 * completa, mostra "pronto!" e so' entao abre a home. Continua a splash do
 * teste anterior.
 */
void test_splash_fills_one_unit_at_a_time_behind_real_progress(void)
{
    lv_obj_t * splash = lv_screen_active();
    uint8_t prev = ratimos_splash_units_shown();
    bool saw_partial = false;
    bool saw_pronto = false;
    int guard = 0;

    while (lv_screen_active() == splash && guard++ < 600) {
        pump_ms(10);
        if (lv_screen_active() != splash) {
            break;
        }
        uint8_t shown = ratimos_splash_units_shown();
        TEST_ASSERT_TRUE_MESSAGE(shown <= ratimos_splash_units_target(), "bar ran ahead of real progress");
        TEST_ASSERT_TRUE_MESSAGE(shown - prev <= 1, "bar jumped more than one unit at once");
        if (shown > 0 && shown < RATIMOS_SPLASH_UNITS) {
            saw_partial = true;
        }
        if (strcmp(lv_label_get_text(lv_obj_get_child(splash, 2)), "pronto!") == 0) {
            saw_pronto = true;
        }
        prev = shown;
    }
    TEST_ASSERT_TRUE(saw_partial);
    TEST_ASSERT_TRUE_MESSAGE(saw_pronto, "'pronto!' must show before the home opens");
    TEST_ASSERT_EQUAL_UINT8(RATIMOS_SPLASH_UNITS, ratimos_splash_units_shown());
    TEST_ASSERT_TRUE_MESSAGE(lv_screen_active() != splash, "home must open after 'pronto!'");
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
    RUN_TEST(test_background_is_full_frame_and_drawn_one_to_one);
    RUN_TEST(test_row_with_icon_and_two_labels_is_compact);
    RUN_TEST(test_row_icon_ids_used_by_apps_are_compiled_26px_icons);
    RUN_TEST(test_classico_system_background_is_identical_to_before);
    RUN_TEST(test_system_background_follows_the_displayed_theme_and_its_dim);
    RUN_TEST(test_preview_changes_the_displayed_theme_only);
    RUN_TEST(test_theme_apply_raises_the_generation_and_the_home_rebuilds_with_the_new_background);
    RUN_TEST(test_invalid_theme_ids_fall_back_to_classico_and_are_never_applied);
    RUN_TEST(test_splash_screen_is_opaque_without_images_and_starts_at_zero);
    RUN_TEST(test_splash_fills_one_unit_at_a_time_behind_real_progress);
    return UNITY_END();
}
