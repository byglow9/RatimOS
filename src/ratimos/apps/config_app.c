#include <stdio.h>
#include "config_app.h"
#include "../app_shell.h"
#include "../row_list.h"
#include "../../storage/content_api.h"

/*
 * Cache-once (same rationale as jogos_app.c / musica_app.c / album_app.c /
 * cartas_app.c / 01-01) -- avoids rebuilding a new lv_obj_t screen on every
 * visit, which exhausts LVGL's builtin heap after a handful of visits.
 */
static lv_obj_t * s_config_screen = NULL;

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
    ratimos_row_create(shell.content, "cfg_firmware", "firmware", s.firmware_version, NULL);
    ratimos_row_create(shell.content, "cfg_armazenamento", "armazenamento", s.storage_used_label, NULL);

    return shell.screen;
}

void ratimos_config_show(lv_event_t * e)
{
    (void) e;
    if (!s_config_screen) {
        s_config_screen = build_config_screen();
    }
    lv_screen_load(s_config_screen);
}
