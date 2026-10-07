#ifndef RATIMOS_APP_SHELL_H
#define RATIMOS_APP_SHELL_H

#include "lvgl.h"

typedef struct {
    lv_obj_t * screen;   /* tela cheia — carregue com ratimos_screen_load() */
    lv_obj_t * content;  /* área onde o app específico deve montar sua UI */
} ratimos_app_shell_t;

/*
 * Monta o "chrome" padrão de qualquer app do RatimOS: barra superior
 * (marca/relógio/bateria), barra de seção (título), área de conteúdo
 * e rodapé com "voltar" (sempre leva pra Home) + uma dica opcional à
 * direita (`bottom_right_hint` NULL = sem dica).
 */
ratimos_app_shell_t ratimos_app_shell_create(const char * section_label,
                                              const char * bottom_right_hint);

/*
 * Mesmo chrome, mas o "voltar" chama `back_cb` em vez da Home -- telas de
 * segundo nivel voltam pro pai do breadcrumb (plano 02.1-14: um jogo em
 * ./home/jogos/<jogo> volta pra ./home/jogos, nao pra ./home).
 */
ratimos_app_shell_t ratimos_app_shell_create_with_back(const char * section_label,
                                                        const char * bottom_right_hint,
                                                        lv_event_cb_t back_cb);

/*
 * Navegacao com delete-on-navigate (plano 02.1-15, deferred-items #1).
 *
 * Carrega `scr` como tela ativa e DELETA a tela anterior -- a menos que ela
 * tenha sido marcada com ratimos_screen_set_persistent() (so' a home hoje).
 * Telas de app/jogo nao ficam mais em cache: cada *_show() reconstroi a
 * tela a partir do estado em storage, e a tela some do heap do LVGL assim
 * que a jogadora navega pra outra.
 *
 * A delecao e' imediata (lv_screen_load_anim com auto_del, sem animacao):
 * o LVGL 9 trata a delecao de uma tela de dentro do callback de clique de
 * um filho dela (ex.: o proprio botao "voltar"). Quem guarda ponteiros
 * estaticos de widgets precisa registrar um LV_EVENT_DELETE na tela pra
 * zera-los (e deletar seus lv_timer_t) -- ver os *_app.c e jogos/*.c.
 */
void ratimos_screen_load(lv_obj_t * scr);

/* Marca `scr` como persistente: ratimos_screen_load() nunca a deleta. */
void ratimos_screen_set_persistent(lv_obj_t * scr);

#endif
