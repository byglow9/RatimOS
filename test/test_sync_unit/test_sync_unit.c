/*
 * Suite offline (RESEARCH.md Pitfall 5) -- cobre exclusivamente a logica de
 * parsing JSON de sync_client.c com strings JSON fixas em memoria, sem
 * nenhuma chamada de rede/http_client.h. Separada de test_sync_integration/
 * porque o runner Unity nativo do PlatformIO linka todos os .c de um mesmo
 * diretorio test/<nome>/ num unico binario com um unico main().
 */
#include <string.h>
#include <unity.h>
#include "sync/sync_client.h"

/* Helper interno (nao faz parte do contrato publico de sync_client.h) --
 * declarado aqui via extern, mirroring test_storage.c's extern-helper
 * convention, para testar o parsing JSON diretamente sem rede. */
extern size_t ratimos_sync_parse_items(const char *json_body, ratimos_content_item_t *out, size_t max_count);

void setUp(void) {}
void tearDown(void) {}

void test_sync_parse_items_returns_all_fields(void)
{
    const char *body =
        "{\"items\":[{\"id\":\"11111111-1111-1111-1111-111111111111\","
        "\"title\":\"Carta de teste 1\",\"type\":\"letter\","
        "\"content_date\":\"2026-08-01\",\"url\":\"\"}]}";

    ratimos_content_item_t out[4];
    size_t n = ratimos_sync_parse_items(body, out, 4);

    TEST_ASSERT_EQUAL_UINT(1, n);
    TEST_ASSERT_EQUAL_STRING("Carta de teste 1", out[0].title);
    TEST_ASSERT_EQUAL_STRING("11111111-1111-1111-1111-111111111111", out[0].id);
    TEST_ASSERT_EQUAL_STRING("letter", out[0].type);
    TEST_ASSERT_EQUAL_STRING("2026-08-01", out[0].content_date);
}

void test_sync_parse_items_empty_array_returns_zero(void)
{
    const char *body = "{\"items\":[]}";

    ratimos_content_item_t out[4];
    size_t n = ratimos_sync_parse_items(body, out, 4);

    TEST_ASSERT_EQUAL_UINT(0, n);
}

void test_sync_parse_items_malformed_json_returns_zero(void)
{
    const char *body = "this is not json at all {{{";

    ratimos_content_item_t out[4];
    size_t n = ratimos_sync_parse_items(body, out, 4);

    TEST_ASSERT_EQUAL_UINT(0, n);
}

void test_sync_parse_items_respects_max_count(void)
{
    const char *body =
        "{\"items\":["
        "{\"id\":\"1\",\"title\":\"Item 1\",\"type\":\"letter\",\"content_date\":\"2026-08-01\",\"url\":\"\"},"
        "{\"id\":\"2\",\"title\":\"Item 2\",\"type\":\"photo\",\"content_date\":\"2026-08-02\",\"url\":\"\"},"
        "{\"id\":\"3\",\"title\":\"Item 3\",\"type\":\"music\",\"content_date\":\"2026-08-03\",\"url\":\"\"}"
        "]}";

    ratimos_content_item_t out[4];
    size_t n = ratimos_sync_parse_items(body, out, 1);

    TEST_ASSERT_EQUAL_UINT(1, n);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sync_parse_items_returns_all_fields);
    RUN_TEST(test_sync_parse_items_empty_array_returns_zero);
    RUN_TEST(test_sync_parse_items_malformed_json_returns_zero);
    RUN_TEST(test_sync_parse_items_respects_max_count);
    return UNITY_END();
}
