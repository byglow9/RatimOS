/*
 * Storage/Content API — mount do backend (D-03).
 *
 * Backend nativo/mock desta fase nao precisa de nenhuma montagem real de
 * filesystem (le fixtures via fopen() direto, ver letters.c) — esta funcao
 * so marca "a camada de storage esta pronta", representando o primeiro
 * passo de progresso real do splash.
 */
#include "content_api.h"

static int s_mounted = 0;

void ratimos_storage_mount(void)
{
    s_mounted = 1;
}
