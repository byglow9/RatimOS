#include <stdio.h>
#include "config_app.h"
#include "config_picker.h"
#include "../app_shell.h"
#include "../boot_styles.h"
#include "../row_list.h"
#include "../theme.h"
#include "../theme_catalog.h"
#include "../../storage/content_api.h"

/*
 * Construida a cada visita e deletada ao sair (ratimos_screen_load, plano
 * 02.1-15): o ponteiro so' vale enquanto a tela existe -- o LV_EVENT_DELETE
 * zera ele pro proximo ratimos_config_show() reconstruir.
 */
static lv_obj_t * s_config_screen = NULL;

static void config_screen_deleted_cb(lv_event_t * e)
{
    (void) e;
    s_config_screen = NULL;
}

/*
 * Linha que abre outra tela (tema, carregamento -- plano 02.1-16, sketch 010
 * `CFG_ROWS`): mesma anatomia e altura (RATIMOS_LIST_ROW_H) das informativas,
 * mas com fundo mais claro e a seta de pixel a direita, desenhada num
 * LV_EVENT_DRAW_POST (nenhum filho extra: badge=0/text_col=1 continua valendo).
 */
static void tappable_arrow_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    lv_area_t a;
    lv_obj_get_coords(row, &a);
    ratimos_config_draw_arrow(lv_event_get_layer(e), &a);
}

static lv_obj_t * tappable_row_create(lv_obj_t * parent, const char * icon, const char * title,
                                      const char * subtitle, lv_event_cb_t cb)
{
    lv_obj_t * row = ratimos_row_create(parent, icon, title, subtitle, cb);
    lv_obj_set_style_bg_color(row, RATIMOS_COLOR_PANEL_TAPPABLE, 0);
    lv_obj_set_style_bg_opa(row, (lv_opa_t) 217 /* ~85% */, 0);
    lv_obj_add_event_cb(row, tappable_arrow_cb, LV_EVENT_DRAW_POST, NULL);
    return row;
}

static lv_obj_t * build_config_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("./home/config", "fase 0 local");
    lv_obj_set_style_pad_row(shell.content, RATIMOS_ROW_LIST_GAP, 0);

    ratimos_settings_t s = ratimos_storage_get_settings();

    char brightness[8];
    char volume[8];
    snprintf(brightness, sizeof(brightness), "%d%%", s.brightness_pct);
    snprintf(volume, sizeof(volume), "%d%%", s.volume_pct);

    ratimos_row_create(shell.content, "cfg_brilho", "brilho", brightness, NULL);
    ratimos_row_create(shell.content, "cfg_volume", "volume", volume, NULL);
    /* tema e carregamento: dois seletores independentes (sketch 010) */
    tappable_row_create(shell.content, "cfg_tema", "tema", ratimos_theme_applied()->name,
                        ratimos_config_tema_show);
    tappable_row_create(shell.content, "cfg_carregamento", "carregamento",
                        ratimos_boot_style_name(ratimos_storage_get_boot_style()),
                        ratimos_config_carregamento_show);
    ratimos_row_create(shell.content, "cfg_firmware", "firmware", s.firmware_version, NULL);
    ratimos_row_create(shell.content, "cfg_armazenamento", "armazenamento", s.storage_used_label, NULL);

    return shell.screen;
}

void ratimos_config_show(lv_event_t * e)
{
    (void) e;
    if (!s_config_screen) {
        s_config_screen = build_config_screen();
        lv_obj_add_event_cb(s_config_screen, config_screen_deleted_cb, LV_EVENT_DELETE, NULL);
    }
    ratimos_screen_load(s_config_screen);
}
