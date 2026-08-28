#ifndef RATIMOS_SYNC_HTTP_CLIENT_H
#define RATIMOS_SYNC_HTTP_CLIENT_H

#include <stddef.h>

/*
 * Contrato de transporte HTTPS do RatimOS (D-05).
 *
 * Cada variante de transporte (native_curl hoje, esp32 na Fase 7) implementa
 * exatamente estas 2 funcoes. O resto de src/sync/ so conhece este contrato,
 * nunca os detalhes de libcurl/HTTPClient por baixo.
 */

typedef struct {
    int status_code;    /* 0 = a propria camada de transporte nunca obteve resposta */
    char body[4096];
    size_t body_len;
} ratimos_http_response_t;

/* Executa um HTTPS GET, com bearer token opcional (NULL omite o header
 * Authorization). Retorna 0 em sucesso de transporte (out preenchido,
 * independente do status_code), nao-zero se o proprio transporte falhou.
 * Nunca tenta a requisicao se url for NULL ou vazia -- out->status_code
 * permanece 0 e um codigo de falha de transporte e retornado antes de
 * qualquer chamada a libcurl. */
int ratimos_sync_http_get(const char *url, const char *bearer_token,
                           ratimos_http_response_t *out);

/* Executa um HTTPS POST com corpo JSON e um header customizado opcional
 * (header_name/header_value; header_name NULL omite o header). Sempre
 * define Content-Type: application/json quando json_body nao e NULL.
 * Mesmas garantias de retorno e validacao de url que ratimos_sync_http_get. */
int ratimos_sync_http_post_json(const char *url, const char *header_name,
                                 const char *header_value, const char *json_body,
                                 ratimos_http_response_t *out);

#endif
