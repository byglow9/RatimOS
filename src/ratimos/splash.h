#ifndef RATIMOS_SPLASH_H
#define RATIMOS_SPLASH_H

#include <stdint.h>

#include "lvgl.h"
#include "../storage/content_api.h"

/*
 * Tela de boot (Fase 1 D-01..D-04, refeita no plano 02.1-16): fundo do TEMA
 * (etapa separada e trocavel) + carregamento escolhido (animacao) + percentual
 * e rotulo do passo REAL embaixo. O progresso e' ligado aos passos de
 * inicializacao da Storage/Content API (D-02/D-03). Nao aceita toque (D-04):
 * sempre roda ate o fim, sem pular.
 *
 * Tema e carregamento sao lidos da Storage API ANTES do primeiro passo (nao
 * dependem do mount do SD).
 */
void ratimos_splash_show(void);

/*
 * Previa: toca o boot UMA vez com `theme` e `style`, SEM executar nenhum
 * passo real da Storage API e SEM gravar nada. Termina em "pronto!" e chama
 * `on_done` (que normalmente carrega outra tela por ratimos_screen_load(),
 * deletando a tela da splash). Chamado pelos seletores de config.
 */
void ratimos_splash_preview(ratimos_theme_id_t theme, ratimos_boot_style_t style, void (*on_done)(void));

/*
 * Modelo de progresso em UNIDADES (substitui os 25 blocos do 02.1-14):
 *   - alvo   = passos reais concluidos * UNITS / passos totais;
 *   - exibido anda NO MAXIMO 1 unidade por tick (16ms) e nunca passa do alvo.
 * Invariante: units_shown <= units_target. Expostos pro teste.
 */
#define RATIMOS_SPLASH_UNITS 96
uint8_t ratimos_splash_units_shown(void);
uint8_t ratimos_splash_units_target(void);

/* Tema e carregamento da splash atual/ultima (teste). */
ratimos_theme_id_t ratimos_splash_active_theme(void);
ratimos_boot_style_t ratimos_splash_active_style(void);

/* Seam de teste: substitui a etapa de fundo. NULL restaura o fundo do tema. */
typedef void (*ratimos_splash_bg_fn_t)(lv_layer_t * layer, const lv_area_t * area);
void ratimos_splash_set_background(ratimos_splash_bg_fn_t fn);

/* Seam de teste: duracao do fade-in (padrao 400ms; 0 = sem fade, opa cheia
 * desde o primeiro quadro). So' vale para a proxima splash. */
void ratimos_splash_set_fade_ms_for_test(uint32_t ms);

#endif
