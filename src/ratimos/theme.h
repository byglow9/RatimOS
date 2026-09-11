#ifndef RATIMOS_THEME_H
#define RATIMOS_THEME_H

#include "lvgl.h"

/*
 * Paleta RatimOS — amostrada diretamente do logo oficial (logo/RatimOS.png,
 * decisão D-17): fundo quase-preto dominante, painéis violeta escuro,
 * acento vermelho-rook, textos com leve matiz violeta. Substitui a paleta
 * placeholder da Fase 0 (indigo/teal), aplicada em toda a UI (home, splash
 * e as 5 telas de app), não apenas na splash.
 */
#define RATIMOS_COLOR_BG            lv_color_hex(0x000000)
#define RATIMOS_COLOR_PANEL         lv_color_hex(0x2a123f)
#define RATIMOS_COLOR_PANEL_ACTIVE  lv_color_hex(0x4e2277)
#define RATIMOS_COLOR_ACCENT        lv_color_hex(0xe6010f)
#define RATIMOS_COLOR_TEXT          lv_color_hex(0xf5f2f8)
#define RATIMOS_COLOR_TEXT_MUTED    lv_color_hex(0xa997ba)

/*
 * Cores semanticas de JOGO (novas nesta fase, D-17 intacto).
 *
 * D-17 travou a paleta de MARCA/CHROME (as 6 macros acima) — nao havia jogo
 * nenhum quando ela foi definida, entao ela nao previu feedback de acerto/erro.
 * Estas 7 macros existem SOMENTE para feedback de jogabilidade dentro do
 * tabuleiro (letra certa/presente/ausente no termo, faixa de categoria
 * resolvida no conexo). Nunca use nenhuma delas como cor de chrome, de botao
 * primario ou de CTA — esse papel continua sendo exclusivo de
 * RATIMOS_COLOR_ACCENT.
 */
#define RATIMOS_COLOR_GAME_CORRECT   lv_color_hex(0x2f8f4e)
#define RATIMOS_COLOR_GAME_PRESENT   lv_color_hex(0xd99a1b)
#define RATIMOS_COLOR_GAME_ABSENT    lv_color_hex(0x3a3a42)

#define RATIMOS_COLOR_CONEXO_YELLOW  lv_color_hex(0xd9a520)
#define RATIMOS_COLOR_CONEXO_GREEN   lv_color_hex(0x3f8f4e)
#define RATIMOS_COLOR_CONEXO_BLUE    lv_color_hex(0x2f6fa8)
#define RATIMOS_COLOR_CONEXO_PURPLE  lv_color_hex(0x8a3fbe)

#define RATIMOS_SCREEN_W 320
#define RATIMOS_SCREEN_H 480

void ratimos_theme_apply_screen(lv_obj_t * scr);
lv_obj_t * ratimos_panel_create(lv_obj_t * parent);

/*
 * Cria o icone solto usado pelos launchers/linhas de lista -- SEM nenhum
 * container/badge por baixo (a bola vermelha circular foi removida, G-02.1-1/
 * G-02.1-2: ela era um lv_obj_create() clicavel por padrao que interceptava o
 * toque destinado a linha/tile pai, ver icones.md). `icon_id` e' resolvido
 * via ratimos_icon_by_id() (src/ratimos/icons.h): numa correspondencia,
 * retorna o icone de pixel art project-authored (VISUAL-01/D-07) criado
 * diretamente via lv_image_create() -- essa e' a UNICA coisa visivel no
 * slot, sem recorte de circulo, sem preenchimento de fundo. Em NULL ou id
 * sem correspondencia, cai para um lv_label_create() com o texto bruto do
 * id (icon_id e' entao tratado como o texto a exibir). Em qualquer um dos
 * dois casos o objeto retornado nunca e' LV_OBJ_FLAG_CLICKABLE (garantido
 * pelo proprio construtor do LVGL para lv_image/lv_label) e e' sempre
 * exatamente UM objeto -- nunca desreferencia um icon_id NULL, nunca
 * envolve o icone/rotulo num container extra (preserva o contrato de
 * indice de filho badge=0/text_col=1 de ratimos_row_create()).
 */
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * icon_id);

#endif
