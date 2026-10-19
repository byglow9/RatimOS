#ifndef RATIMOS_THEME_CATALOG_H
#define RATIMOS_THEME_CATALOG_H

#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"
#include "../storage/content_api.h"

/*
 * Catalogo de TEMAS (plano 02.1-16, sketch 010 "Modelo (revisao 2)").
 *
 * Tema = fundo do BOOT + fundo do SISTEMA (home e telas de app), cada um com
 * seu proprio escurecimento. O CARREGAMENTO (animacao de boot) e' outra
 * escolha, independente (boot_styles.h): qualquer tema combina com qualquer
 * carregamento. (theme.h/theme.c continuam sendo paleta + bevel; este arquivo
 * so' cuida do "qual fundo".)
 *
 * Fundos da home/sistema: imagens ESTATICAS 320x480 RGB565 em flash (nunca
 * indexadas: imagem indexada de tela cheia passa pelo decoder do LVGL e
 * aloca buffer no heap -- motivo registrado no 02.1-14).
 *
 * Fundos do boot: desenhados direto no "stage" da splash por
 * ratimos_theme_boot_bg_draw() -- NUNCA criam objeto LVGL nem usam heap LVGL
 * por quadro; os animados so' invalidam a area que mudou
 * (ratimos_theme_boot_bg_tick()).
 */

typedef enum {
    RATIMOS_BOOT_BG_BLACK = 0, /* preto liso: defensivo e testes */
    RATIMOS_BOOT_BG_DITHER,    /* bg_dither.png 1:1 */
    RATIMOS_BOOT_BG_STARS,     /* Noite estrelada: faixas + 70 estrelas piscando + lua */
    RATIMOS_BOOT_BG_SUNSET,    /* Castelo ao entardecer: imagem + janelas piscando */
    RATIMOS_BOOT_BG_CHESS      /* Xadrez: tabuleiro em perspectiva por pixel, rolando */
} ratimos_boot_bg_source_t;

typedef struct {
    ratimos_theme_id_t id;
    const char * name;
    struct {
        ratimos_boot_bg_source_t source;
        uint8_t dim_pct; /* retangulo preto translucido por cima do fundo (rgba(0,0,0,dim) do sketch) */
    } boot_bg;
    struct {
        const lv_image_dsc_t * image; /* 320x480 RGB565 estatica */
        uint8_t dim_pct;              /* aplicado em tempo de execucao (image_recolor preto) */
    } system_bg;
    const lv_image_dsc_t * home_thumb;    /* 40x60, seletor de tema */
    const lv_image_dsc_t * boot_bg_thumb; /* 40x60, seletor de carregamento */
} ratimos_theme_t;

/* Tema do id; id fora do intervalo OU linha sem preencher cai no Classico
 * (caminho defensivo permanente -- valor vindo de disco nunca indexa fora). */
const ratimos_theme_t * ratimos_theme_get(ratimos_theme_id_t id);

/* Tema APLICADO (o da Storage API; carregado na 1a chamada). */
const ratimos_theme_t * ratimos_theme_applied(void);
/* Tema EXIBIDO: o aplicado, exceto durante uma previa (preview_begin). */
const ratimos_theme_t * ratimos_theme_displayed(void);

/* Torna `id` o tema aplicado em memoria e aumenta a geracao. NAO grava (quem
 * grava e' ratimos_storage_set_theme_id, chamado pelo seletor). */
void ratimos_theme_apply(ratimos_theme_id_t id);

/* Previa: troca o tema EXIBIDO sem tocar no aplicado nem no storage. */
void ratimos_theme_preview_begin(ratimos_theme_id_t id);
void ratimos_theme_preview_end(void);

/* Contador que sobe sempre que o tema exibido pode ter mudado
 * (apply/preview_begin/preview_end). A home guarda a geracao com que foi
 * construida e se reconstroi quando ela muda. */
uint32_t ratimos_theme_generation(void);

/* Desenha o fundo do boot do tema em `layer` (so' dentro do clip atual):
 * fonte + escurecimento. `area` = o stage (320x480). */
void ratimos_theme_boot_bg_draw(const ratimos_theme_t * theme, lv_layer_t * layer,
                                const lv_area_t * area, uint32_t now_ms);

/* A fonte do fundo de boot anima? (decide se o timer de fundo existe.) */
bool ratimos_theme_boot_bg_is_animated(const ratimos_theme_t * theme);

/* Chamado pelo timer de animacao de fundo (50ms): invalida em `stage` so' o
 * que mudou entre prev_ms e now_ms. No-op para fontes estaticas. */
void ratimos_theme_boot_bg_tick(const ratimos_theme_t * theme, uint32_t prev_ms, uint32_t now_ms,
                                lv_obj_t * stage);

/* Seam so' para teste: substitui a tabela de temas (NULL restaura). */
void ratimos_theme_catalog_override_for_test(const ratimos_theme_t * themes, size_t count);

#endif
