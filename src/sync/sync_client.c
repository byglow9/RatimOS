/*
 * Sync Client -- implementacao (D-09/D-10). Chama http_client.h's GET
 * contract (Plano 1) e faz o parsing do corpo JSON via cJSON -- unico
 * arquivo do projeto que inclui cJSON.h, sync_client.h nunca conhece esse
 * tipo (RESEARCH.md Pitfall 1).
 */
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "sync/sync_client.h"
#include "sync/http/http_client.h"

/* Copia um campo string de um objeto cJSON para um buffer de tamanho fixo,
 * com strncpy + terminacao nula explicita. Nunca confia no tamanho da
 * string de origem. Se o campo estiver ausente ou nao for string, deixa o
 * buffer de destino vazio (nao falha, nao crasha). */
static void copy_string_field(const cJSON *item, const char *key, char *out, size_t out_size)
{
    out[0] = '\0';
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(item, key);
    if (field == NULL || !cJSON_IsString(field) || field->valuestring == NULL) {
        return;
    }
    strncpy(out, field->valuestring, out_size - 1);
    out[out_size - 1] = '\0';
}

/* Helper interno (nao faz parte do contrato publico de sync_client.h) --
 * extern-linkable para ser testado diretamente por test_sync_unit.c, sem
 * rede. Sempre chama cJSON_Delete antes de retornar, em todo caminho de
 * codigo, para nunca vazar a arvore de parsing. */
size_t ratimos_sync_parse_items(const char *json_body, ratimos_content_item_t *out, size_t max_count)
{
    cJSON *root = cJSON_Parse(json_body);
    if (root == NULL) {
        return 0;
    }

    const cJSON *items = cJSON_GetObjectItemCaseSensitive(root, "items");
    if (!cJSON_IsArray(items)) {
        cJSON_Delete(root);
        return 0;
    }

    size_t count = 0;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, items)
    {
        if (count >= max_count) {
            break;
        }
        copy_string_field(item, "id", out[count].id, sizeof(out[count].id));
        copy_string_field(item, "title", out[count].title, sizeof(out[count].title));
        copy_string_field(item, "type", out[count].type, sizeof(out[count].type));
        copy_string_field(item, "content_date", out[count].content_date, sizeof(out[count].content_date));
        copy_string_field(item, "url", out[count].url, sizeof(out[count].url));
        count++;
    }

    cJSON_Delete(root);
    return count;
}

size_t ratimos_sync_whats_new(const char *base_url, const char *device_token,
                               ratimos_content_item_t *out, size_t max_count,
                               int *out_status)
{
    char url[256];
    snprintf(url, sizeof(url), "%s/functions/v1/whats-new", base_url);

    ratimos_http_response_t response;
    int rc = ratimos_sync_http_get(url, device_token, &response);

    if (rc != 0) {
        *out_status = response.status_code;
        return 0;
    }

    *out_status = response.status_code;
    if (response.status_code != 200) {
        return 0;
    }

    return ratimos_sync_parse_items(response.body, out, max_count);
}
