#ifndef RATIMOS_BOOT_STYLES_H
#define RATIMOS_BOOT_STYLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"
#include "../storage/content_api.h"

/*
 * CARREGAMENTOS (animacoes da tela de boot) -- plano 02.1-16, sketch 008 +
 * rotinas finais do sketch 010 (`BOOTS.*.draw`). Independentes do tema: cada
 * rotina desenha SO' o logo + a animacao (assets com alfa) sobre o que
 * splash_draw_background() ja pintou, entao funciona sobre qualquer fundo.
 *
 * Nada aqui cria objeto LVGL: as rotinas desenham no `layer` do stage da
 * splash durante LV_EVENT_DRAW_MAIN_END, respeitando o clip atual.
 */

/* O que a splash entrega a uma rotina de carregamento a cada desenho. */
typedef struct {
    int32_t units;       /* progresso exibido, 0..units_total */
    int32_t units_total; /* RATIMOS_SPLASH_UNITS */
    bool done;           /* units == units_total ("pronto!") */
    uint8_t wave_phase;  /* 0/1, alterna a cada 260ms (tremular da bandeira) */
    lv_opa_t opa;        /* fade-in do boot (~400ms); aplicado a TODOS os draws do carregamento */
    /* Etapa de fundo trocavel (splash_draw_background). Qualquer apagamento
     * futuro usa ISTO, nunca uma cor fixa. */
    void (*draw_background)(lv_layer_t * layer, const lv_area_t * area);
} ratimos_boot_frame_t;

/* Desenha o carregamento. `style` fora do intervalo (ou sem rotina) cai em
 * BANDEIRA. */
void ratimos_boot_style_draw(ratimos_boot_style_t style, lv_layer_t * layer,
                             const ratimos_boot_frame_t * frame);

/* Area (absoluta, tela 320x480) que muda quando o progresso vai de
 * `units_before` a `units_after`. Devolve false (e nao toca em `out`) quando
 * nada visivel mudou -- a splash so' invalida isso. */
bool ratimos_boot_style_dirty_area(ratimos_boot_style_t style, int32_t units_before,
                                   int32_t units_after, lv_area_t * out);

/* Area invalidada a cada onda de 260ms. false = este estilo nao tremula agora
 * (`done` = progresso completo). */
bool ratimos_boot_style_wave_area(ratimos_boot_style_t style, bool done, lv_area_t * out);

/* Y (absoluto) da linha do percentual; a linha do passo fica 16px abaixo. */
int32_t ratimos_boot_style_text_y(ratimos_boot_style_t style);

/* Texto do percentual: prefixo do estilo + "%03d%%", pct = round(units*100/RATIMOS_SPLASH_UNITS)
 * (como o b.pct() do 010). Prefixos: "" (bandeira), "construindo " (ameias),
 * "carregando " (letreiro e torres). */
void ratimos_boot_style_format_pct(ratimos_boot_style_t style, int32_t units, char * buf, size_t len);

/* "hastear a bandeira", "ameias do castelo", "letreiro que acende", "fileira de torres". */
const char * ratimos_boot_style_name(ratimos_boot_style_t style);

/* Miniatura 40x60 com alfa do carregamento (o quadro a 60% do progresso, sem
 * texto), para os seletores de config. Em runtime fica por cima da miniatura de
 * fundo de boot do tema em uso. */
const lv_image_dsc_t * ratimos_boot_style_thumb(ratimos_boot_style_t style);

/* Subida da bandeira da variante BANDEIRA, em px: floor(units*64/UNITS/2)*2
 * (par, 0..64). Funcao pura do progresso. */
int32_t ratimos_boot_flag_rise_px(int32_t units);

#endif
