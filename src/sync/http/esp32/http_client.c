/*
 * Transporte HTTPS -- stub esp32 (D-05, RESEARCH.md Open Question 1).
 *
 * Ainda nao existe ambiente PlatformIO `esp32s3` nem toolchain ESP-IDF neste
 * projeto -- este arquivo existe apenas para provar que o contrato de
 * src/sync/http/http_client.h "compila como uma unidade de traducao" com um
 * gcc puro (`gcc -fsyntax-only -std=gnu11 -Isrc src/sync/http/esp32/http_client.c`).
 * Por isso NAO inclui nenhum header ESP-IDF/Arduino -- apenas "../http_client.h".
 * A implementacao real (HTTPClient/NetworkClientSecure) e trabalho da Fase 7.
 *
 * Excluido do build de native_sim via build_src_filter em platformio.ini,
 * para nunca colidir (duplicate symbol) com src/sync/http/native_curl/http_client.c.
 */
#include "../http_client.h"

int ratimos_sync_http_get(const char *url, const char *bearer_token,
                           ratimos_http_response_t *out)
{
    (void) url;
    (void) bearer_token;
    /* TODO (Fase 7): implementar via HTTPClient/NetworkClientSecure */
    out->status_code = 0;
    return -1;
}

int ratimos_sync_http_post_json(const char *url, const char *header_name,
                                 const char *header_value, const char *json_body,
                                 ratimos_http_response_t *out)
{
    (void) url;
    (void) header_name;
    (void) header_value;
    (void) json_body;
    /* TODO (Fase 7): implementar via HTTPClient/NetworkClientSecure */
    out->status_code = 0;
    return -1;
}
