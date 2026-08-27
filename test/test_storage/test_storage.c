/*
 * Wave 0 test scaffold (RESEARCH.md Validation Architecture) -- covers the
 * Storage/Content API's 5 domains against their real assets/mock/ fixtures
 * (letters from 01-01; photos/tracks/games/settings added in 01-03). Runs
 * via PlatformIO's native Unity test runner: no LVGL/SDL dependency, pure C,
 * exercises the exact mount()/index_*()/list_*() path the splash and app
 * screens already use in production.
 */
#include <unity.h>
#include "storage/content_api.h"

/* Internal helper (not part of content_api.h's public contract) — declared
 * here via a matching extern prototype so the safe-default-title defensive
 * behavior can be unit-tested directly, without corrupting any of the 3
 * real track fixtures the phase-gate UAT depends on seeing rendered intact. */
extern void ratimos_storage_tracks_read_title_or_default(const char * path, char * out, size_t out_size);

void setUp(void) {}
void tearDown(void) {}

void test_letters_list_returns_fixture_titles(void)
{
    ratimos_storage_mount();
    ratimos_storage_index_letters();

    ratimos_letter_t out[4];
    size_t n = ratimos_storage_list_letters(out, 4);

    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_STRING("Carta de teste 1", out[0].title);
}

void test_letters_list_respects_max_count(void)
{
    ratimos_storage_mount();
    ratimos_storage_index_letters();

    ratimos_letter_t out[4];
    size_t n = ratimos_storage_list_letters(out, 1);

    TEST_ASSERT_EQUAL_UINT(1, n);
}

void test_photos_list_returns_fixture_titles(void)
{
    ratimos_storage_mount();
    ratimos_storage_index_photos();

    ratimos_photo_t out[4];
    size_t n = ratimos_storage_list_photos(out, 4);

    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_STRING("Foto de teste 1", out[0].title);
}

void test_tracks_list_returns_fixture_titles(void)
{
    ratimos_storage_mount();
    ratimos_storage_index_tracks();

    ratimos_track_t out[4];
    size_t n = ratimos_storage_list_tracks(out, 4);

    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_STRING("Faixa de teste 1", out[0].title);
}

void test_games_list_returns_compiled_titles_in_order(void)
{
    ratimos_storage_index_games();

    ratimos_game_t out[4];
    size_t n = ratimos_storage_list_games(out, 4);

    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_STRING("sudoku", out[0].title);
    TEST_ASSERT_EQUAL_STRING("car jam", out[1].title);
    TEST_ASSERT_EQUAL_STRING("paciencia", out[2].title);
}

void test_settings_get_returns_nonzero_values(void)
{
    ratimos_storage_index_settings();

    ratimos_settings_t s = ratimos_storage_get_settings();

    TEST_ASSERT_GREATER_THAN(0, s.brightness_pct);
    TEST_ASSERT_GREATER_THAN(0, s.volume_pct);
    TEST_ASSERT_TRUE(s.firmware_version[0] != '\0');
}

void test_tracks_read_title_defaults_when_file_missing(void)
{
    char title[64];
    ratimos_storage_tracks_read_title_or_default("assets/mock/tracks/does_not_exist.txt", title, sizeof(title));

    TEST_ASSERT_EQUAL_STRING("sem titulo", title);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_letters_list_returns_fixture_titles);
    RUN_TEST(test_letters_list_respects_max_count);
    RUN_TEST(test_photos_list_returns_fixture_titles);
    RUN_TEST(test_tracks_list_returns_fixture_titles);
    RUN_TEST(test_games_list_returns_compiled_titles_in_order);
    RUN_TEST(test_settings_get_returns_nonzero_values);
    RUN_TEST(test_tracks_read_title_defaults_when_file_missing);
    return UNITY_END();
}
