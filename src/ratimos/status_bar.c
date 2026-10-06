#include "status_bar.h"
#include "theme.h"
#include "icons.h"
#include "fonts/ratimos_fonts.h"

#include <string.h>
#include <time.h>

/* Offset (px) da sombra em pixel do logo -- offset duro, sem blur, mesma
 * familia de tratamento aplicada aos icones soltos (ver icones.md:
 * lv_obj_set_style_shadow_width gera blur no LVGL, nao serve; a sombra real
 * e' um segundo lv_image tingido de escuro, desenhado ANTES do icone
 * principal). */
#define RATIMOS_LOGO_SHADOW_OFFSET 2

/* Bateria pixel-art (substitui o glifo de bateria da fonte padrao) --
 * retangulo externo (borda, sem preenchimento) + nub de terminal +
 * retangulo de preenchimento de carga, ver header-navegacao.md "Bateria em
 * pixel-art". */
#define RATIMOS_BATTERY_W 16
#define RATIMOS_BATTERY_H 8
#define RATIMOS_BATTERY_TERMINAL_W 2
#define RATIMOS_BATTERY_TERMINAL_H 2

/* Percentual mockado fixo -- o dado real de carga vem do PMIC AXP2101 via
 * XPowersLib somente quando a Phase 4 (Power Management, POWER-01) estiver
 * pronta; native_sim nao tem PMIC nenhum, entao nao ha valor vivo pra ler
 * aqui. Nao criar nenhum caminho que finja ler um valor que nao existe. */
#define RATIMOS_BATTERY_MOCK_PCT 70

static lv_obj_t * bar_row_create(lv_obj_t * parent, lv_coord_t height)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, RATIMOS_SCREEN_W, height);
    lv_obj_set_style_bg_color(row, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

/*
 * Cria a bateria pixel-art: um wrapper (sem estilo) contendo a celula
 * bordeada (retangulo externo + preenchimento interno de carga) e o nub de
 * terminal como IRMAO da celula (nao filho dela) -- LVGL recorta filhos no
 * limite do pai por padrao (seria preciso opt-in pro nub "vazar" pra fora
 * da celula se fosse filho dela), entao o wrapper e' dimensionado pra
 * caber a celula + o nub lado a lado sem precisar desabilitar o recorte.
 */
static lv_obj_t * pixel_battery_create(lv_obj_t * parent)
{
    lv_obj_t * wrap = lv_obj_create(parent);
    lv_obj_remove_style_all(wrap);
    lv_obj_set_size(wrap, RATIMOS_BATTERY_W + RATIMOS_BATTERY_TERMINAL_W, RATIMOS_BATTERY_H);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * cell = lv_obj_create(wrap);
    lv_obj_remove_style_all(cell);
    lv_obj_set_pos(cell, 0, 0);
    lv_obj_set_size(cell, RATIMOS_BATTERY_W, RATIMOS_BATTERY_H);
    lv_obj_set_style_border_width(cell, 1, 0);
    lv_obj_set_style_border_color(cell, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(cell, 0, 0);
    lv_obj_set_style_pad_all(cell, 0, 0);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    /* Preenchimento de carga -- ver nota da Phase 4/POWER-01 acima do
     * #define RATIMOS_BATTERY_MOCK_PCT: valor fixo mockado, nunca ligado a
     * uma fonte de dado real que ainda nao existe em native_sim. Usa
     * RATIMOS_COLOR_TEXT (nao uma cor semantica de jogo) -- theme.h proibe
     * explicitamente usar as cores de feedback de tabuleiro como chrome. */
    lv_obj_t * fill = lv_obj_create(cell);
    lv_obj_remove_style_all(fill);
    lv_coord_t fill_w = (lv_coord_t) ((RATIMOS_BATTERY_W - 2) * RATIMOS_BATTERY_MOCK_PCT / 100);
    lv_obj_set_pos(fill, 1, 1);
    lv_obj_set_size(fill, fill_w, RATIMOS_BATTERY_H - 2);
    lv_obj_set_style_bg_color(fill, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fill, 0, 0);
    lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);

    /* Nub do polo positivo, irmao da celula (ver comentario da funcao). */
    lv_obj_t * terminal = lv_obj_create(wrap);
    lv_obj_remove_style_all(terminal);
    lv_obj_set_pos(terminal, RATIMOS_BATTERY_W, (RATIMOS_BATTERY_H - RATIMOS_BATTERY_TERMINAL_H) / 2);
    lv_obj_set_size(terminal, RATIMOS_BATTERY_TERMINAL_W, RATIMOS_BATTERY_TERMINAL_H);
    lv_obj_set_style_bg_color(terminal, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(terminal, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(terminal, 0, 0);
    lv_obj_clear_flag(terminal, LV_OBJ_FLAG_SCROLLABLE);

    return wrap;
}

void ratimos_topbar_create(lv_obj_t * parent)
{
    lv_obj_t * row = bar_row_create(parent, RATIMOS_TOPBAR_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_40, 0);
    /* Scrim escuro translucido sobre o fundo ditherizado -- o design
     * aprovado (sketch 004, header-navegacao.md) e' `rgba(0,0,0,0.4)`:
     * preto a 40% de opacidade, nao o painel violeta opaco que
     * bar_row_create() aplica por padrao. Override local (mesmo padrao da
     * sectionbar/bottombar), bar_row_create() continua com LV_OPA_COVER. */
    lv_obj_set_style_bg_color(row, lv_color_black(), 0);

    lv_obj_t * brand = lv_obj_create(row);
    lv_obj_remove_style_all(brand);
    lv_obj_set_size(brand, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(brand, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(brand, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(brand, 4, 0);
    lv_obj_clear_flag(brand, LV_OBJ_FLAG_SCROLLABLE);

    /* Logo do sistema: peao-torre vermelho fornecido pela usuaria,
     * processado pelo pipeline real de icones (tools/prepare_system_rook.py
     * + tools/convert_images.py, Task 1 deste plano) -- substitui
     * totalmente o glifo de "home" da fonte padrao. */
    const lv_image_dsc_t * rook = ratimos_icon_by_id("system_rook");
    if (rook) {
        lv_obj_t * logo_stack = lv_obj_create(brand);
        lv_obj_remove_style_all(logo_stack);
        lv_obj_set_size(logo_stack, rook->header.w + RATIMOS_LOGO_SHADOW_OFFSET,
                         rook->header.h + RATIMOS_LOGO_SHADOW_OFFSET);
        lv_obj_clear_flag(logo_stack, LV_OBJ_FLAG_SCROLLABLE);

        /* Sombra em pixel: segundo lv_image tingido de escuro, criado ANTES
         * do icone principal pra pintar por baixo na ordem z (icones.md). */
        lv_obj_t * logo_shadow = lv_image_create(logo_stack);
        lv_image_set_src(logo_shadow, rook);
        lv_obj_set_pos(logo_shadow, RATIMOS_LOGO_SHADOW_OFFSET, RATIMOS_LOGO_SHADOW_OFFSET);
        lv_obj_set_style_image_recolor(logo_shadow, lv_color_black(), 0);
        lv_obj_set_style_image_recolor_opa(logo_shadow, LV_OPA_COVER, 0);

        lv_obj_t * logo_main = lv_image_create(logo_stack);
        lv_image_set_src(logo_main, rook);
        lv_obj_set_pos(logo_main, 0, 0);
    }

    /* Marca como no sketch 003-C (`.mock-topbar`): "RATIMOS" em caixa
     * alta, mono 10px, cor de texto clara -- o vermelho fica so' no rook. */
    lv_obj_t * brand_label = lv_label_create(brand);
    lv_label_set_text(brand_label, "RATIMOS");
    lv_obj_set_style_text_color(brand_label, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(brand_label, &ratimos_font_mono_10, 0);

    lv_obj_t * right = lv_obj_create(row);
    lv_obj_remove_style_all(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 6, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    /* Relogio: hora real do PC (native_sim), lida uma unica vez na
     * construcao do topbar -- as telas sao cache-build-once (padrao ja
     * estabelecido no projeto), entao isso NAO "tica" ao vivo numa sessao
     * longa. Substituto real e' o RTC PCF85063 (Phase 4, SHELL-02/POWER-02);
     * nao criar aqui um lv_timer_create() por tela, isso e' escopo novo
     * alem do fechamento de gap desta sessao. */
    time_t now = time(NULL);
    struct tm tm_now = {0};
    struct tm * tm_ptr = localtime(&now);
    if (tm_ptr) {
        tm_now = *tm_ptr;
    }
    char clock_buf[8];
    lv_snprintf(clock_buf, sizeof(clock_buf), "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);

    lv_obj_t * clock = lv_label_create(right);
    lv_label_set_text(clock, clock_buf);
    lv_obj_set_style_text_color(clock, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(clock, &ratimos_font_mono_10, 0);

    pixel_battery_create(right);
}

/*
 * Escreve um caminho de sectionbar no label. Sketch 003-C (`.mock-sectionbar`):
 * o caminho inteiro numa cor so' (RATIMOS_COLOR_TEXT), mono 11px -- o
 * segmento final em vermelho (RATIMOS_COLOR_ACCENT) foi removido no plano
 * 02.1-14: era ilegivel onde a barra passa sobre o magenta do fundo
 * (deferred-items #4) e nao existe no modelo aprovado. Unico ponto que
 * escreve o texto do caminho (create() e set_path() chamam esta funcao).
 */
static void format_breadcrumb(lv_obj_t * label, const char * path)
{
    lv_label_set_recolor(label, false);
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    lv_label_set_text(label, path ? path : "");
}

void ratimos_sectionbar_create(lv_obj_t * parent, const char * path)
{
    lv_obj_t * row = bar_row_create(parent, RATIMOS_SECTIONBAR_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Unico filho do row: o label do caminho recolorido -- o antigo glifo
     * de checkmark (fonte padrao) foi removido (nao fazia parte do design
     * validado, ver header-navegacao.md "Sectionbar: caminho crescente").
     * JetBrains Mono 11px, como o `.mock-sectionbar` do sketch 003-C. */
    lv_obj_t * title = lv_label_create(row);
    lv_obj_set_style_text_font(title, &ratimos_font_mono_11, 0);
    format_breadcrumb(title, path);
}

void ratimos_sectionbar_set_path(lv_obj_t * title_label, const char * path)
{
    format_breadcrumb(title_label, path);
}

void ratimos_bottombar_create(lv_obj_t * parent, const char * left_text, lv_event_cb_t left_cb,
                               const char * right_text)
{
    /* 28px (era 22): cabe o botao bevel de "voltar" (label 16px + pad 2+2 +
     * borda 2+2). O content de app_shell.c usa flex_grow, entao ele so'
     * encolhe 6px -- nada e' empurrado pra fora dos 480px. */
    lv_obj_t * row = bar_row_create(parent, RATIMOS_BOTTOMBAR_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    if (left_cb) {
        /* Acao "voltar" como botao bevel 003-C (plano 02.1-13) -- antes era
         * so' texto vermelho num lv_button sem estilo. LV_SYMBOL_LEFT e'
         * mantido: o label usa a mono padrao, e o glifo da seta vem do
         * `.fallback` dela (Montserrat 14), ver lv_conf.h. */
        char buf[48];
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT " %s", left_text);
        lv_obj_t * btn = ratimos_button_create(row, buf, left_cb, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(btn, 8, 0);
        lv_obj_set_style_pad_ver(btn, 2, 0);
        lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), RATIMOS_COLOR_TEXT, 0);
    }
    else {
        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, left_text);
        lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    }

    /* Dica opcional: NULL = nenhum label (o sketch 003-C nao tem dica
     * nenhuma; a dica fixa de "toque" saiu dos 4 apps no plano 02.1-14). */
    if (right_text) {
        lv_obj_t * right = lv_label_create(row);
        lv_label_set_text(right, right_text);
        lv_obj_set_style_text_color(right, RATIMOS_COLOR_TEXT_MUTED, 0);
    }
}
