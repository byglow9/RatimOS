#ifndef RATIMOS_STATUS_BAR_H
#define RATIMOS_STATUS_BAR_H

#include "lvgl.h"

/*
 * Barra superior (marca "RatimOS" + relógio + bateria).
 * Relógio/bateria são placeholders estáticos por enquanto — ficam
 * "vivos" na Fase 2 do plano, quando o firmware tiver RTC e PMIC reais.
 */
void ratimos_topbar_create(lv_obj_t * parent);

/*
 * Barra de seção: renderiza `path` como um caminho estilo explorador de
 * arquivos (ex: "./home/jogos/conexo"), com os segmentos-pai em
 * RATIMOS_COLOR_TEXT_MUTED e o segmento atual em destaque
 * (RATIMOS_COLOR_ACCENT), como um único lv_label_t recolorido — sem cursor
 * piscando. O row criado tem exatamente um filho (o label do caminho).
 */
void ratimos_sectionbar_create(lv_obj_t * parent, const char * path);

/*
 * Atualiza um label de sectionbar já criado por ratimos_sectionbar_create()
 * (o mesmo ponteiro de label retornado/armazenado por ela) para exibir um
 * novo `path`, reaplicando a mesma formatação muted/atual -- para telas
 * cujo título muda depois da sectionbar já construída (ex: termo.c
 * trocando entre os modos termo/dueto/quarteto).
 */
void ratimos_sectionbar_set_path(lv_obj_t * title_label, const char * path);

/*
 * Barra inferior. `left_text` + `left_cb` (opcional): se left_cb não for
 * NULL, o texto vira um botão clicável (ex: "voltar" -> Home). `right_text`
 * é sempre uma dica não-interativa.
 */
void ratimos_bottombar_create(lv_obj_t * parent, const char * left_text, lv_event_cb_t left_cb,
                               const char * right_text);

#endif
