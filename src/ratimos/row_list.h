#ifndef RATIMOS_ROW_LIST_H
#define RATIMOS_ROW_LIST_H

#include "lvgl.h"
#include "theme.h"

/*
 * Linha padrão de lista usada por vários apps (jogos, config, cartas,
 * música): ícone solto (sem badge/container por baixo) + título + legenda,
 * ocupando a largura toda. `click_cb` pode ser NULL para uma linha
 * não-clicável.
 *
 * `letter` é, apesar do nome herdado, um icon id resolvido primeiro via
 * ratimos_icon_by_id() (src/ratimos/icons.h) — numa correspondência, exibe
 * o ícone de pixel art (VISUAL-01/D-07) solto, sem nenhum wrapper; em NULL
 * ou sem correspondência, cai para um rótulo de texto puro (`letter` é
 * então tratado como o texto a exibir). Mesmo contrato de
 * ratimos_badge_create() (theme.h), que este helper delega diretamente —
 * inclusive o não-clicável garantido, que fecha o bug de hitbox G-02.1-1.
 */
/*
 * Espaco vertical entre linhas numa lista (gap do `.mock-content` do sketch
 * 003-C). Os apps de lista aplicam no `content` do app_shell.
 */
#define RATIMOS_ROW_LIST_GAP RATIMOS_LIST_GAP /* theme.h */

/*
 * Anatomia 003-C (plano 02.1-14): moldura bevel com padding 9, icone 26px
 * com sombra em pixel, titulo em Press Start 2P 8px e descricao em
 * JetBrains Mono 10px esmaecida -- a linha mede ~48px. Filhos: badge=0,
 * text_col=1 (titulo=0, subtitulo=1), contrato lido por jogos_app.c.
 */
lv_obj_t * ratimos_row_create(lv_obj_t * parent,
                               const char * letter,
                               const char * title,
                               const char * subtitle,
                               lv_event_cb_t click_cb);

#endif
