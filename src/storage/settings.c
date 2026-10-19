/*
 * Storage/Content API — dominio `settings` (config).
 *
 * Dois grupos de dados:
 *
 * 1) Placeholders REAIS por D-09 (brilho, volume, versao, armazenamento):
 *    nao sao valores hardcoded direto em config_app.c (isso seria repetir o
 *    anti-padrao que esta fase elimina) -- sao valores reais, nao-zero,
 *    servidos por este dominio da Storage API, exatamente como qualquer
 *    outro dominio de conteudo. Somente leitura, so' em memoria.
 *
 * 2) PERSISTIDOS (plano 02.1-16, D-10): `theme_id` e `boot_style`.
 *
 *    >>> NVS, NUNCA O SD <<<
 *    Este arquivo (assets/save/settings.bin) e' o SUBSTITUTO DO SIMULADOR
 *    para o NVS. No ESP32-S3 a Fase de hardware troca estas funcoes por duas
 *    chaves de `Preferences` (namespace "ratimos", chaves "theme_id" e
 *    "boot_style"), mantendo as assinaturas de content_api.h. O SD e'
 *    removivel e nao e' lugar de configuracao do aparelho (restricao de
 *    seguranca do CLAUDE.md: estado do dispositivo nunca no SD). Por isso
 *    get/set NAO dependem de ratimos_storage_mount() (o mount do SD): o boot
 *    le o tema ANTES do primeiro passo.
 *
 * Disciplina V5: o arquivo lido e' ENTRADA NAO CONFIAVEL (truncado, adulterado,
 * de uma versao futura). Registro invalido (tamanho, magic, versao, checksum)
 * da os 2 padroes. Registro valido mas com UM campo fora do intervalo da o
 * padrao SO' daquele campo e preserva o outro. Disciplina V12: o caminho e'
 * uma constante fixa.
 */
#include <stdio.h>
#include <string.h>

#include "content_api.h"
#include "save_io.h"

#define SETTINGS_MAGIC          0x54534d52u /* "RMST" little-endian */
#define SETTINGS_FORMAT_VERSION 1u

static const char * const s_settings_path = RATIMOS_SAVE_DIR "/settings.bin";

/*
 * Registro em disco (16 bytes, gravado como bytes crus):
 *   0  magic            u32  "RMST"
 *   4  format_version   u16
 *   6  theme_id         u8
 *   7  boot_style       u8
 *   8  reserved         u32  (sempre 0)
 *  12  checksum         u32  soma aditiva dos bytes 4..11
 */
typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint8_t theme_id;
    uint8_t boot_style;
    uint32_t reserved;
    uint32_t checksum;
} settings_record_t;

_Static_assert(sizeof(settings_record_t) == 16, "settings_record_t ganhou padding inesperado");

static ratimos_settings_t s_settings;

static uint32_t record_checksum(const settings_record_t * rec)
{
    const uint8_t * p = (const uint8_t *) rec;
    uint32_t sum = 0;
    for (size_t i = offsetof(settings_record_t, format_version); i < offsetof(settings_record_t, checksum); i++) {
        sum += p[i];
    }
    return sum;
}

static bool theme_in_range(uint8_t v)
{
    return v < (uint8_t) RATIMOS_THEME_COUNT;
}

static bool style_in_range(uint8_t v)
{
    return v < (uint8_t) RATIMOS_BOOT_STYLE_COUNT;
}

/* Le o registro do "NVS". false = ausente OU invalido (chamador usa padroes). */
static bool load_record(settings_record_t * rec)
{
    memset(rec, 0, sizeof(*rec));
    if (!ratimos_save_read_exact(s_settings_path, rec, sizeof(*rec), NULL)) {
        return false;
    }
    if (rec->magic != SETTINGS_MAGIC || rec->format_version != SETTINGS_FORMAT_VERSION) {
        return false;
    }
    if (rec->checksum != record_checksum(rec)) {
        return false;
    }
    return true;
}

static void load_values(ratimos_theme_id_t * theme, ratimos_boot_style_t * style)
{
    settings_record_t rec;
    *theme = RATIMOS_THEME_DEFAULT;
    *style = RATIMOS_BOOT_STYLE_DEFAULT;
    if (!load_record(&rec)) {
        return;
    }
    /* Cada campo e' validado SOZINHO: um fora do intervalo nao derruba o outro. */
    if (theme_in_range(rec.theme_id)) {
        *theme = (ratimos_theme_id_t) rec.theme_id;
    }
    if (style_in_range(rec.boot_style)) {
        *style = (ratimos_boot_style_t) rec.boot_style;
    }
}

static bool store_values(ratimos_theme_id_t theme, ratimos_boot_style_t style)
{
    settings_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = SETTINGS_MAGIC;
    rec.format_version = SETTINGS_FORMAT_VERSION;
    rec.theme_id = (uint8_t) theme;
    rec.boot_style = (uint8_t) style;
    rec.checksum = record_checksum(&rec);

    ratimos_save_ensure_dir();
    return ratimos_save_write_atomic(s_settings_path, &rec, sizeof(rec));
}

ratimos_theme_id_t ratimos_storage_get_theme_id(void)
{
    ratimos_theme_id_t theme;
    ratimos_boot_style_t style;
    load_values(&theme, &style);
    return theme;
}

ratimos_boot_style_t ratimos_storage_get_boot_style(void)
{
    ratimos_theme_id_t theme;
    ratimos_boot_style_t style;
    load_values(&theme, &style);
    return style;
}

bool ratimos_storage_set_theme_id(ratimos_theme_id_t id)
{
    if ((int) id < 0 || (int) id >= (int) RATIMOS_THEME_COUNT) {
        return false;
    }
    ratimos_theme_id_t theme;
    ratimos_boot_style_t style;
    load_values(&theme, &style); /* preserva o OUTRO campo */
    return store_values(id, style);
}

bool ratimos_storage_set_boot_style(ratimos_boot_style_t style_in)
{
    if ((int) style_in < 0 || (int) style_in >= (int) RATIMOS_BOOT_STYLE_COUNT) {
        return false;
    }
    ratimos_theme_id_t theme;
    ratimos_boot_style_t style;
    load_values(&theme, &style);
    return store_values(theme, style_in);
}

void ratimos_storage_index_settings(void)
{
    s_settings.brightness_pct = 80;
    s_settings.volume_pct = 50;
    snprintf(s_settings.firmware_version, sizeof(s_settings.firmware_version), "0.1.0-sim");
    snprintf(s_settings.storage_used_label, sizeof(s_settings.storage_used_label), "12 MB / 512 MB (simulado)");
    load_values(&s_settings.theme_id, &s_settings.boot_style);
}

ratimos_settings_t ratimos_storage_get_settings(void)
{
    /* Os 2 campos persistidos refletem o armazenamento no momento da
     * chamada (um set depois de index_settings aparece aqui). */
    load_values(&s_settings.theme_id, &s_settings.boot_style);
    return s_settings;
}
