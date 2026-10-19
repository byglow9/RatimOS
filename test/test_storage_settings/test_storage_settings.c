/*
 * Suite do dominio `settings` persistido (plano 02.1-16, Task 1): theme_id e
 * boot_style. Sem LVGL/SDL -- so' o caminho real de fopen/fwrite/rename de
 * src/storage/settings.c + save_io.c.
 *
 * O arquivo e' o substituto do simulador para o NVS do dispositivo (ver o
 * cabecalho de settings.c). main() faz mkdtemp + chdir ANTES de qualquer
 * chamada, entao nada aqui toca o assets/save/ real do desenvolvedor.
 *
 * As mutacoes diretas de arquivo (fopen local) existem so' para simular um
 * registro corrompido/adulterado: espelham o layout PRIVADO de
 * settings_record_t (16 bytes, checksum = soma dos bytes 4..11).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unity.h>

#include "storage/content_api.h"

static const char * const PATH = "assets/save/settings.bin";

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint8_t theme_id;
    uint8_t boot_style;
    uint32_t reserved;
    uint32_t checksum;
} test_record_t;

_Static_assert(sizeof(test_record_t) == 16, "layout de settings_record_t mudou");

static uint32_t checksum_of(const test_record_t * r)
{
    const uint8_t * p = (const uint8_t *) r;
    uint32_t sum = 0;
    for (size_t i = 4; i < 12; i++) {
        sum += p[i];
    }
    return sum;
}

static void write_raw(const void * data, size_t size)
{
    FILE * f = fopen(PATH, "wb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(size, fwrite(data, 1, size, f));
    fclose(f);
}

static test_record_t valid_record(uint8_t theme, uint8_t style)
{
    test_record_t r;
    memset(&r, 0, sizeof(r));
    r.magic = 0x54534d52u;
    r.format_version = 1;
    r.theme_id = theme;
    r.boot_style = style;
    r.checksum = checksum_of(&r);
    return r;
}

static bool file_exists(void)
{
    FILE * f = fopen(PATH, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

void setUp(void)
{
    ratimos_storage_index_game_state(); /* so' cria assets/save/ no tmpdir */
    remove(PATH);
    ratimos_storage_index_settings();
}

void tearDown(void) {}

void test_defaults_without_file_are_classico_and_bandeira(void)
{
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_DEFAULT, RATIMOS_THEME_CLASSICO);
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_DEFAULT, RATIMOS_BOOT_STYLE_BANDEIRA);
}

void test_persisted_ids_never_renumbered(void)
{
    /* Valores gravados em NVS: fixos para sempre. */
    TEST_ASSERT_EQUAL_INT(0, RATIMOS_THEME_CLASSICO);
    TEST_ASSERT_EQUAL_INT(1, RATIMOS_THEME_NOITE);
    TEST_ASSERT_EQUAL_INT(2, RATIMOS_THEME_CASTELO);
    TEST_ASSERT_EQUAL_INT(3, RATIMOS_THEME_XADREZ);
    TEST_ASSERT_EQUAL_INT(4, RATIMOS_THEME_COUNT);
    TEST_ASSERT_EQUAL_INT(0, RATIMOS_BOOT_STYLE_BANDEIRA);
    TEST_ASSERT_EQUAL_INT(1, RATIMOS_BOOT_STYLE_AMEIAS);
    TEST_ASSERT_EQUAL_INT(2, RATIMOS_BOOT_STYLE_LETREIRO);
    TEST_ASSERT_EQUAL_INT(3, RATIMOS_BOOT_STYLE_TORRES);
    TEST_ASSERT_EQUAL_INT(4, RATIMOS_BOOT_STYLE_COUNT);
}

void test_getters_work_before_mount_and_index(void)
{
    /* setUp() so' chama index_settings; mount() nunca e' chamado nesta suite. */
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_NOITE));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_NOITE, ratimos_storage_get_theme_id());
}

void test_set_theme_does_not_change_boot_style(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_boot_style(RATIMOS_BOOT_STYLE_TORRES));
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_XADREZ));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_TORRES, ratimos_storage_get_boot_style());
}

void test_set_boot_style_does_not_change_theme(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_CASTELO));
    TEST_ASSERT_TRUE(ratimos_storage_set_boot_style(RATIMOS_BOOT_STYLE_AMEIAS));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CASTELO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_AMEIAS, ratimos_storage_get_boot_style());
}

void test_both_survive_a_new_index_settings_and_show_in_settings_struct(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_XADREZ));
    TEST_ASSERT_TRUE(ratimos_storage_set_boot_style(RATIMOS_BOOT_STYLE_LETREIRO));

    ratimos_storage_index_settings(); /* simula reboot */

    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_LETREIRO, ratimos_storage_get_boot_style());
    ratimos_settings_t s = ratimos_storage_get_settings();
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_XADREZ, s.theme_id);
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_LETREIRO, s.boot_style);
    /* os placeholders continuam la */
    TEST_ASSERT_EQUAL_INT(80, s.brightness_pct);
}

void test_truncated_record_gives_both_defaults(void)
{
    test_record_t r = valid_record(RATIMOS_THEME_XADREZ, RATIMOS_BOOT_STYLE_TORRES);
    write_raw(&r, sizeof(r) - 3);
    ratimos_storage_index_settings();
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

void test_record_with_trailing_garbage_gives_both_defaults(void)
{
    uint8_t buf[sizeof(test_record_t) + 4];
    test_record_t r = valid_record(RATIMOS_THEME_XADREZ, RATIMOS_BOOT_STYLE_TORRES);
    memset(buf, 0xAB, sizeof(buf));
    memcpy(buf, &r, sizeof(r));
    write_raw(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

void test_wrong_magic_gives_both_defaults(void)
{
    test_record_t r = valid_record(RATIMOS_THEME_XADREZ, RATIMOS_BOOT_STYLE_TORRES);
    r.magic = 0xDEADBEEFu;
    r.checksum = checksum_of(&r);
    write_raw(&r, sizeof(r));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

void test_wrong_checksum_gives_both_defaults(void)
{
    test_record_t r = valid_record(RATIMOS_THEME_XADREZ, RATIMOS_BOOT_STYLE_TORRES);
    r.checksum += 1;
    write_raw(&r, sizeof(r));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

void test_out_of_range_theme_falls_back_only_for_theme(void)
{
    test_record_t r = valid_record(200, RATIMOS_BOOT_STYLE_TORRES); /* checksum VALIDO */
    write_raw(&r, sizeof(r));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CLASSICO, ratimos_storage_get_theme_id());
    TEST_ASSERT_EQUAL_INT_MESSAGE(RATIMOS_BOOT_STYLE_TORRES, ratimos_storage_get_boot_style(),
                                  "a valid boot_style must survive an invalid theme_id");
}

void test_out_of_range_boot_style_falls_back_only_for_style(void)
{
    test_record_t r = valid_record(RATIMOS_THEME_NOITE, 99);
    write_raw(&r, sizeof(r));
    TEST_ASSERT_EQUAL_INT_MESSAGE(RATIMOS_THEME_NOITE, ratimos_storage_get_theme_id(),
                                  "a valid theme_id must survive an invalid boot_style");
    TEST_ASSERT_EQUAL_INT(RATIMOS_BOOT_STYLE_BANDEIRA, ratimos_storage_get_boot_style());
}

void test_set_with_out_of_range_value_returns_false_and_does_not_touch_file(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_CASTELO));
    uint8_t before[sizeof(test_record_t)];
    FILE * f = fopen(PATH, "rb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(sizeof(before), fread(before, 1, sizeof(before), f));
    fclose(f);

    TEST_ASSERT_FALSE(ratimos_storage_set_theme_id((ratimos_theme_id_t) RATIMOS_THEME_COUNT));
    TEST_ASSERT_FALSE(ratimos_storage_set_theme_id((ratimos_theme_id_t) -1));
    TEST_ASSERT_FALSE(ratimos_storage_set_boot_style((ratimos_boot_style_t) RATIMOS_BOOT_STYLE_COUNT));
    TEST_ASSERT_FALSE(ratimos_storage_set_boot_style((ratimos_boot_style_t) 77));

    uint8_t after[sizeof(test_record_t)];
    f = fopen(PATH, "rb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT(sizeof(after), fread(after, 1, sizeof(after), f));
    fclose(f);
    TEST_ASSERT_EQUAL_MEMORY(before, after, sizeof(before));
    TEST_ASSERT_EQUAL_INT(RATIMOS_THEME_CASTELO, ratimos_storage_get_theme_id());
}

void test_set_with_out_of_range_value_does_not_create_the_file(void)
{
    TEST_ASSERT_FALSE(ratimos_storage_set_theme_id((ratimos_theme_id_t) 50));
    TEST_ASSERT_FALSE(file_exists());
}

void test_write_leaves_no_stray_tmp_file(void)
{
    TEST_ASSERT_TRUE(ratimos_storage_set_theme_id(RATIMOS_THEME_NOITE));
    FILE * f = fopen("assets/save/settings.bin.tmp", "rb");
    TEST_ASSERT_NULL(f);
}

int main(void)
{
    /* Diretorio temporario ANTES de qualquer chamada da Storage API. */
    char tmpl[] = "/tmp/ratimos-settings-XXXXXX";
    if (mkdtemp(tmpl) == NULL || chdir(tmpl) != 0) {
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_defaults_without_file_are_classico_and_bandeira);
    RUN_TEST(test_persisted_ids_never_renumbered);
    RUN_TEST(test_getters_work_before_mount_and_index);
    RUN_TEST(test_set_theme_does_not_change_boot_style);
    RUN_TEST(test_set_boot_style_does_not_change_theme);
    RUN_TEST(test_both_survive_a_new_index_settings_and_show_in_settings_struct);
    RUN_TEST(test_truncated_record_gives_both_defaults);
    RUN_TEST(test_record_with_trailing_garbage_gives_both_defaults);
    RUN_TEST(test_wrong_magic_gives_both_defaults);
    RUN_TEST(test_wrong_checksum_gives_both_defaults);
    RUN_TEST(test_out_of_range_theme_falls_back_only_for_theme);
    RUN_TEST(test_out_of_range_boot_style_falls_back_only_for_style);
    RUN_TEST(test_set_with_out_of_range_value_returns_false_and_does_not_touch_file);
    RUN_TEST(test_set_with_out_of_range_value_does_not_create_the_file);
    RUN_TEST(test_write_leaves_no_stray_tmp_file);
    return UNITY_END();
}
