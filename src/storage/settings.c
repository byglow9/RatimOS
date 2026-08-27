/*
 * Storage/Content API — dominio `settings` (config).
 *
 * Placeholders REAIS por D-09: nao sao valores hardcoded direto em
 * config_app.c (isso seria repetir o anti-padrao que esta fase elimina) —
 * sao valores reais, nao-zero, servidos por este dominio da Storage API,
 * exatamente como qualquer outro dominio de conteudo.
 */
#include <stdio.h>
#include "content_api.h"

static ratimos_settings_t s_settings;

void ratimos_storage_index_settings(void)
{
    s_settings.brightness_pct = 80;
    s_settings.volume_pct = 50;
    snprintf(s_settings.firmware_version, sizeof(s_settings.firmware_version), "0.1.0-sim");
    snprintf(s_settings.storage_used_label, sizeof(s_settings.storage_used_label), "12 MB / 512 MB (simulado)");
}

ratimos_settings_t ratimos_storage_get_settings(void)
{
    return s_settings;
}
