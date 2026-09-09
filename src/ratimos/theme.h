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
 * Cria o selo redondo de 28x28px usado pelos launchers/linhas de lista.
 * `icon_id` e' resolvido via ratimos_icon_by_id() (src/ratimos/icons.h):
 * numa correspondencia, renderiza o icone de pixel art project-authored
 * (VISUAL-01/D-07) centrado, com o container recortado em circulo. Em
 * NULL ou id sem correspondencia, cai para a renderizacao original de
 * circulo+letra (icon_id e' entao tratado como o texto a exibir) --
 * nunca desreferencia um icon_id NULL, sempre retorna um objeto valido.
 */
lv_obj_t * ratimos_badge_create(lv_obj_t * parent, const char * icon_id);

#endif
