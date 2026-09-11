#include "status_bar.h"
#include "theme.h"

#include <string.h>

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

void ratimos_topbar_create(lv_obj_t * parent)
{
    lv_obj_t * row = bar_row_create(parent, 26);

    lv_obj_t * brand = lv_label_create(row);
    lv_label_set_text(brand, LV_SYMBOL_HOME " RatimOS");
    lv_obj_set_style_text_color(brand, RATIMOS_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);

    lv_obj_t * right = lv_obj_create(row);
    lv_obj_remove_style_all(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 6, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * clock = lv_label_create(right);
    lv_label_set_text(clock, "--:--");
    lv_obj_set_style_text_color(clock, RATIMOS_COLOR_TEXT, 0);

    lv_obj_t * batt = lv_label_create(right);
    lv_label_set_text(batt, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(batt, RATIMOS_COLOR_TEXT, 0);
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
