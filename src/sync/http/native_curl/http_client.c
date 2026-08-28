/*
 * Transporte HTTPS -- implementacao native_curl (simulador de PC via libcurl).
 * Todo o codigo libcurl do projeto vive exclusivamente aqui -- nenhum outro
 * arquivo deste repositorio deve incluir o header do libcurl (mesma
 * convencao de isolamento de native_sdl/board.c para o header da SDL2).
 */
#include <curl/curl.h>
#include <string.h>
#include "../http_client.h"

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    ratimos_http_response_t *resp = (ratimos_http_response_t *) userdata;
    size_t n = size * nmemb;
    size_t space = sizeof(resp->body) - resp->body_len - 1;
    if (n > space) {
        n = space;
    }
    memcpy(resp->body + resp->body_len, ptr, n);
    resp->body_len += n;
    resp->body[resp->body_len] = '\0';
    return size * nmemb; /* diz a libcurl que o chunk inteiro foi "tratado", mesmo truncado */
}

static int url_is_empty(const char *url)
{
    return (url == NULL) || (url[0] == '\0');
}

int ratimos_sync_http_get(const char *url, const char *bearer_token,
                           ratimos_http_response_t *out)
{
    memset(out, 0, sizeof(*out));
    if (url_is_empty(url)) {
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    struct curl_slist *headers = NULL;
    char auth_header[512];
    if (bearer_token) {
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", bearer_token);
        headers = curl_slist_append(headers, auth_header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);               /* deve ser https:// -- SEC-02 */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);      /* ja e o default; explicito p/ auditoria */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        out->status_code = (int) code;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK) ? 0 : -1;
}

int ratimos_sync_http_post_json(const char *url, const char *header_name,
                                 const char *header_value, const char *json_body,
                                 ratimos_http_response_t *out)
{
    memset(out, 0, sizeof(*out));
    if (url_is_empty(url)) {
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    struct curl_slist *headers = NULL;
    char custom_header[512];
    if (json_body) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    if (header_name && header_value) {
        snprintf(custom_header, sizeof(custom_header), "%s: %s", header_name, header_value);
        headers = curl_slist_append(headers, custom_header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);               /* deve ser https:// -- SEC-02 */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);      /* ja e o default; explicito p/ auditoria */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if (json_body) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        out->status_code = (int) code;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK) ? 0 : -1;
}
