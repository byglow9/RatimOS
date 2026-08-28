/*
 * Suite de integracao (RESEARCH.md Pitfall 5) -- exige acesso de rede real ao
 * projeto Supabase hospedado. Le ADMIN_REGISTRATION_SECRET do ambiente via
 * getenv() em tempo de execucao (nunca hardcoded aqui) -- veja .env na raiz
 * do repo (gitignored, D-03). Separada de test_sync_unit/ porque o runner
 * Unity nativo do PlatformIO linka todos os .c de um mesmo diretorio
 * test/<nome>/ num unico binario com um unico main(), e cada suite Unity
 * define seu proprio main() (correcao estrutural registrada no plano).
 */
#include <string.h>
#include <stdlib.h>
#include <unity.h>
#include "sync/http/http_client.h"
#include "sync/sync_client.h"

#define REGISTER_DEVICE_URL "https://bhqscupdrgfuwitbtlui.supabase.co/functions/v1/register-device"
#define WHATS_NEW_URL "https://bhqscupdrgfuwitbtlui.supabase.co/functions/v1/whats-new"
#define SUPABASE_BASE_URL "https://bhqscupdrgfuwitbtlui.supabase.co"

/* Token sintaticamente valido (formato UUID) mas nunca registrado via
 * register-device -- exercita o mesmo caminho de codigo (token_hash nao
 * encontrado -> 401) que um "token de outro device" exerceria, per o
 * assumption_delta deste plano (single-device project, D-12). */
#define NEVER_REGISTERED_TOKEN "00000000-dead-beef-cafe-000000000000"

void setUp(void) {}
void tearDown(void) {}

void test_register_device_missing_secret_rejected(void)
{
    ratimos_http_response_t out;
    int rc = ratimos_sync_http_post_json(REGISTER_DEVICE_URL, NULL, NULL,
                                          "{\"label\":\"ci-test\"}", &out);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(401, out.status_code);
}

void test_register_device_valid_secret_creates_token(void)
{
    const char *admin_secret = getenv("ADMIN_REGISTRATION_SECRET");
    TEST_ASSERT_NOT_NULL_MESSAGE(admin_secret,
        "ADMIN_REGISTRATION_SECRET must be set in the environment (see .env) to run this test");

    ratimos_http_response_t out;
    int rc = ratimos_sync_http_post_json(REGISTER_DEVICE_URL, "X-Admin-Secret", admin_secret,
                                          "{\"label\":\"ci-test\"}", &out);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(201, out.status_code);
    TEST_ASSERT_NOT_NULL(strstr(out.body, "token"));
}

void test_whats_new_valid_token_returns_pending_items(void)
{
    const char *device_token = getenv("TEST_DEVICE_TOKEN");
    TEST_ASSERT_NOT_NULL_MESSAGE(device_token,
        "TEST_DEVICE_TOKEN must be set in the environment (see .env, bootstrapped in Plan 2 Task 3) to run this test");

    ratimos_content_item_t items[8];
    int status = 0;
    size_t n = ratimos_sync_whats_new(SUPABASE_BASE_URL, device_token, items, 8, &status);

    TEST_ASSERT_EQUAL_INT(200, status);
    TEST_ASSERT_EQUAL_UINT(3, n);

    int found_seeded_title = 0;
    for (size_t i = 0; i < n; i++) {
        if (strcmp(items[i].title, "Carta de teste 1") == 0 ||
            strcmp(items[i].title, "Foto de teste 1") == 0 ||
            strcmp(items[i].title, "Faixa de teste 1") == 0) {
            found_seeded_title = 1;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found_seeded_title,
        "expected at least one of the 3 seeded content_items titles from Plan 2's seed script");
}

void test_whats_new_no_token_rejected(void)
{
    ratimos_content_item_t items[8];
    int status = 0;
    size_t n = ratimos_sync_whats_new(SUPABASE_BASE_URL, NULL, items, 8, &status);

    TEST_ASSERT_EQUAL_INT(401, status);
    TEST_ASSERT_EQUAL_UINT(0, n);
}

void test_whats_new_wrong_token_rejected(void)
{
    ratimos_content_item_t items[8];
    int status = 0;
    size_t n = ratimos_sync_whats_new(SUPABASE_BASE_URL, NEVER_REGISTERED_TOKEN, items, 8, &status);

    TEST_ASSERT_EQUAL_INT(401, status);
    TEST_ASSERT_EQUAL_UINT(0, n);
}

void test_https_transport_succeeds(void)
{
    const char *device_token = getenv("TEST_DEVICE_TOKEN");
    TEST_ASSERT_NOT_NULL_MESSAGE(device_token,
        "TEST_DEVICE_TOKEN must be set in the environment (see .env) to run this test");

    /* Chama ratimos_sync_http_get diretamente (nao via sync_client.h) contra
     * a URL real do whats-new -- prova que a verificacao de certificado TLS
     * padrao do libcurl realmente funciona em tempo de execucao contra o
     * certificado real da Supabase, nao apenas que as opcoes corretas estao
     * setadas no codigo-fonte (SEC-02). */
    ratimos_http_response_t out;
    int rc = ratimos_sync_http_get(WHATS_NEW_URL, device_token, &out);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_NOT_EQUAL(0, out.status_code);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_register_device_missing_secret_rejected);
    RUN_TEST(test_register_device_valid_secret_creates_token);
    RUN_TEST(test_whats_new_valid_token_returns_pending_items);
    RUN_TEST(test_whats_new_no_token_rejected);
    RUN_TEST(test_whats_new_wrong_token_rejected);
    RUN_TEST(test_https_transport_succeeds);
    return UNITY_END();
}
