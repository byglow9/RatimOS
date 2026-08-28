#ifndef RATIMOS_SYNC_CLIENT_H
#define RATIMOS_SYNC_CLIENT_H

#include <stddef.h>

/*
 * Sync Client do RatimOS (D-04/D-05/D-09/D-10).
 *
 * Contrato unico, sincrono e sem alocacao dinamica para ler o conteudo
 * pendente de um device, espelhando o estilo de content_api.h. Compoe com
 * src/sync/http/http_client.h (D-05) -- este arquivo nunca inclui cJSON.h
 * nem conhece detalhes de parsing JSON, isso fica confinado a sync_client.c.
 *
 * Esta fase (02-03) so declara o contrato de leitura em andamento
 * (whats-new, D-10) -- o fluxo de registro de device (register-device)
 * continua sendo ferramenta de operador/teste, exercitado diretamente via
 * http_client.h (Planos 1/4), fora do contrato publico deste header.
 */

typedef struct {
    char id[40];
    char title[64];
    char type[16];
    char content_date[16];
    char url[128];
} ratimos_content_item_t;

/* Busca o conteudo pendente do device no endpoint whats-new. `base_url` e a
 * URL do projeto Supabase (ex.: https://bhqscupdrgfuwitbtlui.supabase.co) --
 * esta funcao monta o caminho completo /functions/v1/whats-new internamente.
 * `out_status` sempre recebe o codigo HTTP retornado, independente do
 * resultado do parsing, permitindo ao chamador distinguir um 401 (0 itens,
 * out_status=401) de um 200 com genuinamente 0 itens pendentes. O retorno e
 * sempre a quantidade de itens efetivamente copiados para `out`, nunca maior
 * que `max_count`. */
size_t ratimos_sync_whats_new(const char *base_url, const char *device_token,
                               ratimos_content_item_t *out, size_t max_count,
                               int *out_status);

#endif
