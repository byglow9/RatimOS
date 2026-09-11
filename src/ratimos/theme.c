#include "theme.h"
#include "icons.h"
#include "bg_images.h"

/*
 * Fundo de tela: gradiente ditherizado pre-gerado (Task 3, plano 02.1-09 --
 * ver fundo-e-ambiente.md), decodificado da fonte 80x120 e esticado pro
 * frame 320x480 em tempo de desenho (nunca um gradiente computado via
 * estilo de gradiente em runtime do LVGL -- essa variante foi
 * explicitamente rejeitada na sessao de sketch). LV_OBJ_FLAG_FLOATING
 * mantem o bitmap fora do layout
 * flex-column que toda tela usa pra topbar/sectionbar/content/bottombar;
 * lv_image_set_antialias(false) mantem o upscale nitido/pixelado em vez de
 * borrado pelo smoothing padrao do LVGL, preservando o visual retro-pixel.
 * RATIMOS_COLOR_BG solido continua desenhado por baixo (linha acima) como
 * fallback de resiliencia caso o source da imagem alguma vez fique NULL.
 *
 * FORMATO: ratimos_bg_dither_desc e' RGB565 (bg_images.c/tools/
 * convert_bg_dither.py), de proposito NAO o formato indexado I4 que
 * icons.c/progress_images.c usam. Verificacao real no simulador mostrou a
 * combinacao LV_COLOR_FORMAT_I4 + LV_IMAGE_ALIGN_STRETCH renderizando
 * ruido/estatico -- rastreado ate o decoder binario vendorizado do LVGL
 * (lv_bin_decoder.c): com LV_BIN_DECODER_RAM_LOAD desligado (padrao deste
 * projeto), uma imagem indexada LV_IMAGE_SRC_VARIABLE nunca ganha um
 * buffer decodificado completo, caindo no caminho de decode "em pedacos"
 * (lv_image_decoder_get_area()) que nao e' compativel com o caminho de
 * desenho TRANSFORMADO que LV_IMAGE_ALIGN_STRETCH aciona (seta
 * scale_x/scale_y != LV_SCALE_NONE). RGB565 nao passa por decode indexado
 * nenhum -- mesmo formato ja comprovado em producao pelo logo da splash
 * (logo_image.c). NUNCA reverter pra I4 aqui sem resolver esse bug de
 * decode primeiro.
 */
void ratimos_theme_apply_screen(lv_obj_t * scr)
{
    lv_obj_set_style_bg_color(scr, RATIMOS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);

    lv_obj_t * bg = lv_image_create(scr);
    lv_image_set_src(bg, &ratimos_bg_dither_desc);
    lv_obj_set_size(bg, RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
    lv_image_set_inner_align(bg, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_antialias(bg, false);
}

/*
 * Card/painel compartilhado (row_list.c, home_screen.c, todo painel/pill/
 * overlay de jogo). Bevel retro de cantos retos (D-17 revision escopada,
 * plano 02.1-09 -- variante C vencedora de cards-superficies.md): fundo
 * translucido liso (LV_OPA_70, calibrado contra o fundo ditherizado
 * aplicado por ratimos_theme_apply_screen()), borda externa escura de 2px
 * (RATIMOS_COLOR_BEVEL_DARK, nao mais o acento). Sem friso interno claro
 * (cards-superficies.md's "aceitar uma unica borda mais grossa como
 * fallback mais simples") -- NUNCA adicionar um segundo lv_obj_t filho
 * aqui: varios chamadores (row_list.c, jogos_app.c) leem filhos do valor
 * retornado por indice posicional, e um filho extra shiftaria esses
 * indices silenciosamente.
 */
lv_obj_t * ratimos_panel_create(lv_obj_t * parent)
{
    lv_obj_t * panel = lv_obj_create(parent);
    lv_obj_set_style_bg_color(panel, RATIMOS_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_70, 0);
    lv_obj_set_style_border_color(panel, RATIMOS_COLOR_BEVEL_DARK, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_style_text_color(panel, RATIMOS_COLOR_TEXT, 0);
    lv_obj_set_style_shadow_width(panel, 0, 0);
    return panel;
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
