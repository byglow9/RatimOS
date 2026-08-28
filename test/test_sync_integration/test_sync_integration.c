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

#define REGISTER_DEVICE_URL "https://bhqscupdrgfuwitbtlui.supabase.co/functions/v1/register-device"

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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_register_device_missing_secret_rejected);
    RUN_TEST(test_register_device_valid_secret_creates_token);
    return UNITY_END();
}
