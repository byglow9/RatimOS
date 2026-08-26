#ifndef RATIMOS_APP_SHELL_H
#define RATIMOS_APP_SHELL_H

#include "lvgl.h"

typedef struct {
    lv_obj_t * screen;   /* tela cheia — carregue com lv_screen_load() */
    lv_obj_t * content;  /* área onde o app específico deve montar sua UI */
} ratimos_app_shell_t;

/*
 * Monta o "chrome" padrão de qualquer app do RatimOS: barra superior
 * (marca/relógio/bateria), barra de seção (título), área de conteúdo
 * e rodapé com "voltar" (sempre leva pra Home) + uma dica à direita.
 */
ratimos_app_shell_t ratimos_app_shell_create(const char * section_label,
                                              const char * bottom_right_hint);

#endif
