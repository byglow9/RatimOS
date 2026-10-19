#ifndef RATIMOS_APPS_CONFIG_PICKER_H
#define RATIMOS_APPS_CONFIG_PICKER_H

#include "lvgl.h"

/*
 * Seletores de TEMA e CARREGAMENTO de Configuracoes (plano 02.1-16, sketch
 * 010 "drawPicker"). Duas telas de segundo nivel com a mesma implementacao:
 *   ./home/config/tema          4 temas, miniatura = fundo da home do tema
 *   ./home/config/carregamento  4 carregamentos, miniatura = fundo de boot do
 *                               tema EM USO com a miniatura do carregamento
 *
 * Cada lista mostra "padrao" e "em uso", tem "previa" (boot e depois home,
 * SEM salvar) e "aplicar" (salva pela Storage API, toca boot e home e volta
 * com o toast "aplicado!"). Tocar numa linha so' a destaca. "voltar" leva a
 * ./home/config.
 *
 * Segue o ciclo de vida do 02.1-15: construida a cada visita, deletada ao
 * sair (ratimos_screen_load), com LV_EVENT_DELETE zerando os estaticos.
 */
void ratimos_config_tema_show(lv_event_t * e);
void ratimos_config_carregamento_show(lv_event_t * e);

/*
 * Seta de pixel das linhas de config que abrem outra tela (a `arrow()` do
 * 010): 5 colunas crescentes, ponta a 13px da borda direita de `row_area`,
 * centralizada na altura. Desenhada num LV_EVENT_DRAW_POST -- nenhum objeto
 * filho, entao o contrato de indice de filho badge=0/text_col=1 de
 * ratimos_row_create() continua valendo.
 */
void ratimos_config_draw_arrow(lv_layer_t * layer, const lv_area_t * row_area);

#endif
