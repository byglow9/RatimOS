#ifndef RATIMOS_STATUS_BAR_H
#define RATIMOS_STATUS_BAR_H

#include "lvgl.h"

/*
 * Barra superior (marca "RatimOS" + relógio + bateria).
 * Relógio/bateria são placeholders estáticos por enquanto — ficam
 * "vivos" na Fase 2 do plano, quando o firmware tiver RTC e PMIC reais.
 */
void ratimos_topbar_create(lv_obj_t * parent);

/* Barra de seção: apenas o nome da tela atual (sem ação). */
void ratimos_sectionbar_create(lv_obj_t * parent, const char * label);

/*
 * Barra inferior. `left_text` + `left_cb` (opcional): se left_cb não for
 * NULL, o texto vira um botão clicável (ex: "voltar" -> Home). `right_text`
 * é sempre uma dica não-interativa.
 */
void ratimos_bottombar_create(lv_obj_t * parent, const char * left_text, lv_event_cb_t left_cb,
                               const char * right_text);

#endif
