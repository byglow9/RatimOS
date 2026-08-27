/*
 * Wave 0 test scaffold (RESEARCH.md Validation Architecture) -- covers the
 * Storage/Content API's letters domain against its real assets/mock/letters/
 * fixtures. Runs via PlatformIO's native Unity test runner: no LVGL/SDL
 * dependency, pure C, exercises the exact mount()/index_letters()/
 * list_letters() path the splash and cartas_app.c already use in production.
 */
#include <unity.h>
#include "storage/content_api.h"

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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_letters_list_returns_fixture_titles);
    RUN_TEST(test_letters_list_respects_max_count);
    return UNITY_END();
}
