#include "status_bar.h"
#include "theme.h"
#include "icons.h"

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

/* Tamanho maximo do buffer de markup recolorido da sectionbar -- folga
 * generosa acima do maior caminho real esperado nesta fase ("./home/jogos/
 * cruzadinha", o nome de app mais longo) mais o overhead dos dois spans
 * "#RRGGBB ...#". */
#define RATIMOS_SECTIONBAR_PATH_BUF_LEN 96

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
    lv_obj_t * row = bar_row_create(parent, 26);

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

    lv_obj_t * brand_label = lv_label_create(brand);
    lv_label_set_text(brand_label, "RatimOS");
    lv_obj_set_style_text_color(brand_label, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(brand_label, &lv_font_montserrat_14, 0);

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

    pixel_battery_create(right);
}

/*
 * Constroi (em `label`) o markup recolorido de um caminho de sectionbar --
 * unico lugar do arquivo que monta a string "#RRGGBB ...#" (create() e
 * set_path() chamam esta funcao, nunca duplicam a logica de recoloracao).
 *
 * Regra: caminhos com profundidade >=2 (2+ ocorrencias de '/', ex:
 * "./home/jogos/conexo") tem tudo ate a ULTIMA '/' (inclusive) esmaecido
 * (RATIMOS_COLOR_TEXT_MUTED) e o segmento final em destaque
 * (RATIMOS_COLOR_ACCENT). Um caminho de nivel unico (ex: "./home", que tem
 * so' uma '/', a do prefixo "./") nao tem segmento-pai nenhum pra esmaecer
 * nem drill-down nenhum pra destacar -- o texto inteiro usa a cor normal de
 * corpo (RATIMOS_COLOR_TEXT).
 */
static void format_breadcrumb(lv_obj_t * label, const char * path)
{
    lv_label_set_recolor(label, true);

    const char * safe_path = path ? path : "";

    size_t slash_count = 0;
    for (const char * p = safe_path; *p; p++) {
        if (*p == '/') {
            slash_count++;
        }
    }

    /* lv_color_to_u32() devolve 0xAARRGGBB (alpha sempre 0xff) -- mascara o
     * byte de alpha pra sobrar so' os 6 digitos hex que o markup de recolor
     * do LVGL espera. Nunca hardcoda uma segunda copia dos valores hex das
     * macros de cor -- eles ficam com fonte unica em theme.h. */
    unsigned long muted_rgb  = (unsigned long) (lv_color_to_u32(RATIMOS_COLOR_TEXT_MUTED) & 0x00FFFFFFu);
    unsigned long accent_rgb = (unsigned long) (lv_color_to_u32(RATIMOS_COLOR_ACCENT) & 0x00FFFFFFu);
    unsigned long text_rgb   = (unsigned long) (lv_color_to_u32(RATIMOS_COLOR_TEXT) & 0x00FFFFFFu);

    char buf[RATIMOS_SECTIONBAR_PATH_BUF_LEN];
    const char * last_slash = strrchr(safe_path, '/');

    if (slash_count >= 2 && last_slash) {
        int muted_len = (int) (last_slash - safe_path) + 1; /* inclui a '/' final */
        lv_snprintf(buf, sizeof(buf), "#%06lx %.*s##%06lx %s#", muted_rgb, muted_len, safe_path,
                    accent_rgb, last_slash + 1);
    }
    else {
        lv_snprintf(buf, sizeof(buf), "#%06lx %s#", text_rgb, safe_path);
    }

    lv_label_set_text(label, buf);
}

void ratimos_sectionbar_create(lv_obj_t * parent, const char * path)
{
    lv_obj_t * row = bar_row_create(parent, 24);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Unico filho do row: o label do caminho recolorido -- o antigo glifo
     * de checkmark (fonte padrao) foi removido (nao fazia parte do design
     * validado, ver header-navegacao.md "Sectionbar: caminho crescente").
     * Fonte no tier Body (D-08) -- o tier Heading/pixel arriscava estourar
     * a largura da sectionbar com um caminho de 3 segmentos. */
    lv_obj_t * title = lv_label_create(row);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    format_breadcrumb(title, path);
}

void ratimos_sectionbar_set_path(lv_obj_t * title_label, const char * path)
{
    format_breadcrumb(title_label, path);
}

void ratimos_bottombar_create(lv_obj_t * parent, const char * left_text, lv_event_cb_t left_cb,
                               const char * right_text)
{
    lv_obj_t * row = bar_row_create(parent, 22);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    if (left_cb) {
        lv_obj_t * btn = lv_button_create(row);
        lv_obj_remove_style_all(btn);
        lv_obj_add_event_cb(btn, left_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t * lbl = lv_label_create(btn);
        char buf[48];
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT " %s", left_text);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_ACCENT, 0);
    }
    else {
        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, left_text);
        lv_obj_set_style_text_color(lbl, RATIMOS_COLOR_TEXT_MUTED, 0);
    }

    lv_obj_t * right = lv_label_create(row);
    lv_label_set_text(right, right_text);
    lv_obj_set_style_text_color(right, RATIMOS_COLOR_TEXT_MUTED, 0);
}
