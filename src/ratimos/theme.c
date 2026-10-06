#include "theme.h"
#include "icons.h"
#include "bg_images.h"

/*
 * Fundo de tela: gradiente ditherizado pre-gerado (fundo-e-ambiente.md,
 * sketch 001-C), 320x480 RGB565, desenhado 1:1 -- exatamente o tamanho do
 * frame, sem stretch nem escala (plano 02.1-14). A versao anterior era uma
 * fonte 80x120 esticada 4x via LV_IMAGE_ALIGN_STRETCH, o que engordava
 * cada scanline de 1px numa faixa de 4px e deixava a granulacao mais
 * pesada que a do sketch. O limite de 80x120 so' existia por medo de um
 * buffer de decode de ~300KB no heap: isso vale pra imagem INDEXADA (I4);
 * RGB565 cru num array const e' lido direto da flash, sem decode e sem
 * heap (medido com lv_mem_monitor no SUMMARY do 02.1-14). Ver o docstring
 * de tools/convert_bg_dither.py antes de mudar o formato.
 *
 * LV_OBJ_FLAG_FLOATING mantem o bitmap fora do layout flex-column que
 * toda tela usa pra topbar/sectionbar/content/bottombar. RATIMOS_COLOR_BG
 * solido continua desenhado por baixo como fallback de resiliencia.
 *
 * A splash NAO usa esta funcao: o boot tem fundo preto liso (splash.c).
 */
void ratimos_theme_apply_screen(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    /* Sem gap entre topbar/sectionbar/content/bottombar: o tema default do
     * LVGL dava pad_row 10 a tela, e a sectionbar de 24px ocupava ~44px. */
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);

    lv_obj_t * bg = lv_image_create(scr);
    lv_image_set_src(bg, &ratimos_bg_dither_desc);
    lv_obj_set_size(bg, RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);
}

/*
 * Friso interno do bevel 003-C desenhado dentro de `area` (as coordenadas
 * EXTERNAS do objeto/tecla), logo depois de uma borda externa de
 * `border_w` px: 1px branco translucido (o `inset 0 0 0 1px` claro do
 * sketch) seguido de 2px pretos translucidos (o `inset 0 0 0 3px` escuro).
 * Desenha so' bordas (bg_opa TRANSP) -- nunca cobre conteudo no miolo.
 * Compartilhado pelo callback DRAW_POST dos paineis e pelo callback
 * DRAW_TASK_ADDED das teclas de lv_buttonmatrix.
 */
static void bevel_draw_frieze(lv_layer_t * layer, const lv_area_t * area, int32_t border_w)
{
    if (layer == NULL || area == NULL) {
        return;
    }
    /* Area pequena demais pra caber borda + friso + faixa: desenha nada. */
    if (lv_area_get_width(area) <= 2 * (border_w + 3) ||
        lv_area_get_height(area) <= 2 * (border_w + 3)) {
        return;
    }

    lv_draw_rect_dsc_t dsc;

    lv_area_t light = *area;
    lv_area_increase(&light, -border_w, -border_w);
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.radius = 0;
    dsc.border_width = 1;
    dsc.border_color = RATIMOS_COLOR_BEVEL_LIGHT;
    dsc.border_opa = RATIMOS_BEVEL_LIGHT_OPA;
    lv_draw_rect(layer, &dsc, &light);

    lv_area_t shade = light;
    lv_area_increase(&shade, -1, -1);
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.radius = 0;
    dsc.border_width = 2;
    dsc.border_color = lv_color_hex(0x000000);
    dsc.border_opa = RATIMOS_BEVEL_SHADE_OPA;
    lv_draw_rect(layer, &dsc, &shade);
}

/*
 * LV_EVENT_DRAW_POST: roda depois do fundo, da borda e dos filhos do
 * objeto, na mesma layer -- o friso fica por cima da borda interna, como o
 * `box-shadow: inset` do CSS. Acompanha a borda REAL do objeto naquele
 * instante (alguns chamadores trocam a borda em runtime, ex. tiles do
 * conexo 1px/2px): o friso sempre encosta por dentro dela. Borda 0 =
 * objeto saiu da moldura (ex. faixas resolvidas do conexo) -> sem friso.
 */
static void bevel_draw_post_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t border_w = lv_obj_get_style_border_width(obj, LV_PART_MAIN);
    if (border_w <= 0) {
        return;
    }
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    bevel_draw_frieze(lv_event_get_layer(e), &coords, border_w);
}

static bool obj_has_event_cb(lv_obj_t * obj, lv_event_cb_t cb)
{
    uint32_t n = lv_obj_get_event_count(obj);
    for (uint32_t i = 0; i < n; i++) {
        lv_event_dsc_t * d = lv_obj_get_event_dsc(obj, i);
        if (d && lv_event_dsc_get_cb(d) == cb) {
            return true;
        }
    }
    return false;
}

/*
 * Moldura bevel 003-C completa (cards-superficies.md, variante C vencedora,
 * confirmada pela usuaria em 2026-10-05: "e' exatamente o visual da C").
 * O friso interno -- pulado no 02.1-09 por medo de criar um segundo lv_obj
 * filho e deslocar indices estruturais -- agora e' desenhado direto na layer
 * pelo callback DRAW_POST acima: nenhum lv_obj_create aqui, o numero de
 * filhos do objeto nao muda.
 */
void ratimos_bevel_apply(lv_obj_t * obj)
{
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_70, 0);
    lv_obj_set_style_border_color(obj, RATIMOS_COLOR_BEVEL_DARK, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    if (!obj_has_event_cb(obj, bevel_draw_post_cb)) {
        lv_obj_add_event_cb(obj, bevel_draw_post_cb, LV_EVENT_DRAW_POST, NULL);
    }
}

/*
 * Card/painel compartilhado (row_list.c, home_screen.c, todo painel/pill/
 * overlay de jogo). Bevel retro 003-C completo via ratimos_bevel_apply():
 * cantos retos, fundo translucido liso (LV_OPA_70, calibrado contra o fundo
 * ditherizado de ratimos_theme_apply_screen()), borda externa escura 2px +
 * friso interno claro + faixa escura -- sem nenhum filho extra (varios
 * chamadores leem filhos do valor retornado por indice posicional).
 */
lv_obj_t * ratimos_panel_create(lv_obj_t * parent)
{
    lv_obj_t * panel = lv_obj_create(parent);
    ratimos_bevel_apply(panel);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_style_text_color(panel, RATIMOS_COLOR_TEXT, 0);
    return panel;
}

/*
 * Botao bevel compartilhado -- substitui os make_pill()/make_pill_w() locais
 * de conexo/sudoku/termo/cruzadinha/paciencia (que usavam radius de pilula
 * e trocavam a borda pra vermelho/0 ao selecionar, fora da variante 003-C).
 */
lv_obj_t * ratimos_button_create(lv_obj_t * parent, const char * text, lv_event_cb_t cb,
                                 lv_coord_t width, lv_coord_t height)
{
    lv_obj_t * btn = ratimos_panel_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_pad_all(btn, 0, 0);
    if (width == LV_SIZE_CONTENT) {
        lv_obj_set_style_pad_hor(btn, 8, 0);
    }
    if (height == LV_SIZE_CONTENT) {
        lv_obj_set_style_pad_ver(btn, 8, 0);
    }
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(btn, RATIMOS_COLOR_PANEL_ACTIVE, LV_STATE_PRESSED);
    if (cb) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    }

    /* Licao do 02.1-09 (hitbox): nenhum filho clicavel por cima do botao --
     * lv_label_create ja nasce sem CLICKABLE, mas o clear e' explicito pra
     * nao depender do default do construtor. */
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text ? text : "");
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(label);

    return btn;
}

void ratimos_button_set_selected(lv_obj_t * btn, bool selected)
{
    if (btn == NULL) {
        return;
    }
    lv_obj_set_style_bg_color(btn, selected ? RATIMOS_COLOR_PANEL_ACTIVE : RATIMOS_COLOR_PANEL, 0);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    if (label) {
        lv_obj_set_style_text_color(label, selected ? RATIMOS_COLOR_ACCENT : RATIMOS_COLOR_TEXT_MUTED, 0);
    }
}

/*
 * LV_EVENT_DRAW_TASK_ADDED do buttonmatrix: para cada tarefa de FILL de uma
 * tecla (LV_PART_ITEMS) acrescenta o friso 003-C dentro da area da tecla.
 * Teclas com radius 0 nao tem o fill encolhido pelo lv_draw_rect, entao a
 * area da tarefa e' exatamente a area externa da tecla. Novas tarefas
 * criadas aqui nao disparam DRAW_TASK_ADDED de novo (lv_draw.c guarda
 * task_running), sem recursao.
 */
static void bevel_buttonmatrix_draw_task_cb(lv_event_t * e)
{
    lv_draw_task_t * t = lv_event_get_draw_task(e);
    if (t == NULL || lv_draw_task_get_type(t) != LV_DRAW_TASK_TYPE_FILL) {
        return;
    }
    lv_draw_dsc_base_t * base = lv_draw_task_get_draw_dsc(t);
    if (base == NULL || base->part != LV_PART_ITEMS) {
        return;
    }
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t border_w = lv_obj_get_style_border_width(obj, LV_PART_ITEMS);
    if (border_w <= 0) {
        return;
    }
    lv_area_t area;
    lv_draw_task_get_area(t, &area);
    bevel_draw_frieze(base->layer, &area, border_w);
}

void ratimos_bevel_style_buttonmatrix(lv_obj_t * m)
{
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_shadow_width(m, 0, 0);
    lv_obj_set_style_radius(m, 0, 0);
    /* Sem padding externo: o tema default do LVGL poe ~10px em volta da
     * matriz, o que achatava as teclas (pad_row/pad_column do chamador nao
     * sao tocados por pad_all). */
    lv_obj_set_style_pad_all(m, 0, 0);

    lv_obj_set_style_radius(m, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(m, RATIMOS_COLOR_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(m, LV_OPA_70, LV_PART_ITEMS);
    lv_obj_set_style_border_color(m, RATIMOS_COLOR_BEVEL_DARK, LV_PART_ITEMS);
    lv_obj_set_style_border_width(m, 2, LV_PART_ITEMS);
    /* Borda externa opaca (LV_OPA_100 == LV_OPA_COVER); o FUNDO da tecla e'
     * que nunca e' opaco -- fica no LV_OPA_70 acima. */
    lv_obj_set_style_border_opa(m, LV_OPA_100, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(m, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(m, RATIMOS_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(m, RATIMOS_COLOR_PANEL_ACTIVE, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(m, RATIMOS_COLOR_PANEL_ACTIVE, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_add_flag(m, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    if (!obj_has_event_cb(m, bevel_buttonmatrix_draw_task_cb)) {
        lv_obj_add_event_cb(m, bevel_buttonmatrix_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    }
}

lv_obj_t * ratimos_modal_create(lv_obj_t * screen, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * scrim = lv_obj_create(screen);
    lv_obj_remove_style_all(scrim);
    lv_obj_add_flag(scrim, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_pos(scrim, 0, 0);
    lv_obj_set_size(scrim, RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_obj_set_style_bg_color(scrim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scrim, LV_OPA_60, 0);
    lv_obj_add_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(scrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(scrim, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * panel = ratimos_panel_create(scrim);
    lv_obj_set_size(panel, w, h);
    lv_obj_center(panel);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(panel, 12, 0);
    /* Painel opaco por cima do scrim: o texto do dialogo nunca disputa
     * contraste com o jogo por tras. */
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    return panel;
}

void ratimos_modal_show(lv_obj_t * modal_panel)
{
    lv_obj_t * scrim = modal_panel ? lv_obj_get_parent(modal_panel) : NULL;
    if (scrim == NULL) {
        return;
    }
    lv_obj_clear_flag(scrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(scrim);
}

void ratimos_modal_hide(lv_obj_t * modal_panel)
{
    lv_obj_t * scrim = modal_panel ? lv_obj_get_parent(modal_panel) : NULL;
    if (scrim) {
        lv_obj_add_flag(scrim, LV_OBJ_FLAG_HIDDEN);
    }
}

bool ratimos_modal_is_visible(lv_obj_t * modal_panel)
{
    lv_obj_t * scrim = modal_panel ? lv_obj_get_parent(modal_panel) : NULL;
    return scrim && !lv_obj_has_flag(scrim, LV_OBJ_FLAG_HIDDEN);
}

/*
 * Icone solto usado pelos launchers/linhas de lista (row_list.c,
 * home_screen.c) -- SEM nenhum container/badge por baixo (G-02.1-1/
 * G-02.1-2: a bola vermelha circular era um lv_obj_create() clicavel por
 * padrao que interceptava o toque destinado a linha/tile pai, ver
 * icones.md). `icon_id` e' resolvido primeiro via ratimos_icon_by_id() --
 * numa correspondencia, cria o icone de pixel art project-authored
 * (VISUAL-01/D-07) diretamente via lv_image_create(); em NULL ou sem
 * correspondencia, cai para um lv_label_create() com o texto bruto do id.
 * lv_image_create()/lv_label_create() ja removem LV_OBJ_FLAG_CLICKABLE no
 * proprio construtor do LVGL (confirmado em lv_image.c/lv_label.c
 * vendorizados) -- nenhuma manipulacao de flag extra e' necessaria aqui.
 * Retorna sempre exatamente UM objeto, nunca envolto num container extra,
 * preservando o contrato de indice de filho badge=0/text_col=1 de
 * ratimos_row_create().
 */
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * icon_id)
{
    const lv_image_dsc_t * icon = ratimos_icon_by_id(icon_id);
    if (icon) {
        lv_obj_t * img = lv_image_create(parent);
        lv_image_set_src(img, icon);
        return img;
    }

    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, icon_id ? icon_id : "?");
    lv_obj_set_style_text_color(label, RATIMOS_COLOR_TEXT, 0);
    return label;
}
