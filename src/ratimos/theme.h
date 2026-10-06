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
 * Borda externa escura do bevel retrô dos cards (D-17 revision escopada,
 * plano 02.1-09 -- ver cards-superficies.md). NAO reabre a paleta travada:
 * as 6 macros de marca/chrome acima continuam intactas, este e' um token
 * novo introduzido pelo sketch especificamente para a borda externa de
 * ratimos_panel_create(), nunca usado como cor de acento/CTA.
 */
#define RATIMOS_COLOR_BEVEL_DARK    lv_color_hex(0x0d0515)

/*
 * Friso interno claro do bevel 003-C (plano 02.1-13): branco usado a ~15-20%
 * de opacidade (RATIMOS_BEVEL_LIGHT_OPA), 1px logo dentro da borda externa
 * -- o `inset 0 0 0 1px rgba(255,255,255,0.15)` do CSS do sketch. Seguido de
 * uma faixa preta de 2px a ~30% (RATIMOS_BEVEL_SHADE_OPA), o
 * `inset 0 0 0 3px rgba(0,0,0,0.35)`. Mesma regra do BEVEL_DARK: token de
 * moldura, nunca cor de acento/CTA/texto.
 */
#define RATIMOS_COLOR_BEVEL_LIGHT   lv_color_hex(0xffffff)
#define RATIMOS_BEVEL_LIGHT_OPA     LV_OPA_20
#define RATIMOS_BEVEL_SHADE_OPA     LV_OPA_30

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

/*
 * Orcamento vertical de toda tela de app (plano 02.1-14, fix de checkpoint):
 * barras fixas + content com padding/gap fixos. A tela (flex column) nao tem
 * gap nenhum entre as barras (ratimos_theme_apply_screen zera pad_row --
 * antes o tema default do LVGL punha 10px acima e abaixo da sectionbar, que
 * parecia ter ~44px). Jogos dimensionam tabuleiro + teclado contra
 * RATIMOS_CONTENT_INNER_H para caber sem rolagem.
 */
#define RATIMOS_TOPBAR_H        26
#define RATIMOS_SECTIONBAR_H    20
#define RATIMOS_BOTTOMBAR_H     28
#define RATIMOS_CONTENT_PAD     10
#define RATIMOS_CONTENT_GAP     8
#define RATIMOS_CONTENT_INNER_H (RATIMOS_SCREEN_H - RATIMOS_TOPBAR_H - RATIMOS_SECTIONBAR_H \
                                 - RATIMOS_BOTTOMBAR_H - 2 * RATIMOS_CONTENT_PAD)
/* Pilulas de modo/dificuldade e botoes de acao de uma linha nos jogos. */
#define RATIMOS_PILL_H          26

/*
 * Ritmo vertical dos cards (plano 02.1-14, fix de checkpoint): cards do
 * mesmo tipo tem a MESMA altura e o mesmo gap em todo menu.
 * - Home: os 6 tiles (jogos largo, grade 2x2, castelo largo) = 4 fileiras
 *   de RATIMOS_HOME_TILE_H com RATIMOS_HOME_GAP (4*80 + 3*10 = 350 <= 386).
 *   Antes: jogos 70, grade 80, castelo 100.
 * - Listas (jogos/musica/cartas/config/estado vazio): linhas de
 *   RATIMOS_LIST_ROW_H (moldura 2+2, padding 9+9, icone 26 -- sketch 003-C)
 *   com RATIMOS_LIST_GAP entre elas, com ou sem subtitulo.
 */
#define RATIMOS_HOME_TILE_H     80
#define RATIMOS_HOME_GAP        10
#define RATIMOS_LIST_ROW_H      48
#define RATIMOS_LIST_GAP        14

void ratimos_theme_apply_screen(lv_obj_t * scr);
lv_obj_t * ratimos_panel_create(lv_obj_t * parent);

/*
 * Aplica a moldura bevel 003-C completa (cards-superficies.md, variante C)
 * a um objeto ja existente: radius 0, fundo RATIMOS_COLOR_PANEL a LV_OPA_70,
 * borda externa 2px RATIMOS_COLOR_BEVEL_DARK, sem sombra -- mais o friso
 * interno (1px claro + 2px escuro) desenhado num callback LV_EVENT_DRAW_POST.
 * NUNCA cria objeto filho (contrato de indice de filho de row_list.c,
 * jogos_app.c, termo.c). Idempotente: chamar duas vezes no mesmo objeto
 * registra o callback de desenho uma unica vez.
 */
void ratimos_bevel_apply(lv_obj_t * obj);

/*
 * Botao clicavel padrao do RatimOS (plano 02.1-13): painel bevel 003-C
 * (ratimos_panel_create) + clickable + nao-scrollable + UM label centralizado
 * nao-clicavel -- arvore painel -> 1 label, a mesma dos make_pill() locais
 * que substitui (leitores estruturais por indice continuam valendo:
 * lv_obj_get_child(btn, 0) e' o label). `cb` (opcional, pode ser NULL)
 * e' registrado em LV_EVENT_CLICKED com user_data NULL. Dimensao fixa ->
 * pad 0 nesse eixo; LV_SIZE_CONTENT -> pad 8 nesse eixo, pra o texto nao
 * encostar na moldura. Estado LV_STATE_PRESSED troca so' o fundo para
 * RATIMOS_COLOR_PANEL_ACTIVE -- moldura e friso intactos.
 */
lv_obj_t * ratimos_button_create(lv_obj_t * parent, const char * text, lv_event_cb_t cb,
                                 lv_coord_t width, lv_coord_t height);

/*
 * Estado selecionado de um botao criado por ratimos_button_create (pills de
 * modo/dificuldade): selecionado = fundo PANEL_ACTIVE + texto ACCENT; nao
 * selecionado = fundo PANEL + texto TEXT_MUTED. NUNCA mexe em borda, raio
 * ou friso -- a moldura bevel e' constante em qualquer estado.
 */
void ratimos_button_set_selected(lv_obj_t * btn, bool selected);

/*
 * Aplica o bevel 003-C as teclas (LV_PART_ITEMS) de um lv_buttonmatrix:
 * container transparente sem borda; teclas radius 0, fundo PANEL a OPA_70,
 * borda 2px BEVEL_DARK, pressed/checked com fundo PANEL_ACTIVE; friso
 * interno de cada tecla desenhado via LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS +
 * LV_EVENT_DRAW_TASK_ADDED, por cima do fill -- a cor de fundo da tecla
 * continua vindo do estilo/jogo (cores semanticas preservadas).
 */
void ratimos_bevel_style_buttonmatrix(lv_obj_t * m);

/*
 * Dialogo modal centralizado na TELA (plano 02.1-14, fix de checkpoint):
 * cria um scrim de tela cheia (320x480, preto translucido, clicavel -- o
 * toque atras do dialogo nao chega no jogo) com LV_OBJ_FLAG_FLOATING, filho
 * de `screen`, e dentro dele um painel bevel w x h centralizado (flex
 * coluna, itens centralizados, pad_row 12). Retorna o PAINEL -- o chamador
 * monta o conteudo nele. Nasce escondido; mostrar/esconder SEMPRE via
 * ratimos_modal_show()/ratimos_modal_hide() (que agem no scrim).
 *
 * Por que FLOATING: toda tela de app e' um flex-column (topbar/sectionbar/
 * content/bottombar). Um painel filho direto da tela SEM o flag vira o 6o
 * item do flex quando aparece -- o lv_obj_align(CENTER) e' ignorado e o
 * dialogo cai no canto inferior esquerdo, cortado, encolhendo o content.
 */
lv_obj_t * ratimos_modal_create(lv_obj_t * screen, lv_coord_t w, lv_coord_t h);
void ratimos_modal_show(lv_obj_t * modal_panel);
void ratimos_modal_hide(lv_obj_t * modal_panel);
bool ratimos_modal_is_visible(lv_obj_t * modal_panel);

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
