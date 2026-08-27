/*
 * Storage/Content API — dominio `photos` (album).
 *
 * Metadata-only nesta fase (Pitfall 3 do RESEARCH.md): sem decode de
 * imagem/JPEG, apenas titulo/id lidos de fixtures de texto simples. Mesma
 * disciplina do dominio `letters` (01-01): manifesto fixo e hardcoded (nunca
 * dirent.h/directory scanning), cache-on-index, safe-default-title.
 */
#include <stdio.h>
#include <string.h>
#include "content_api.h"

#define RATIMOS_PHOTO_COUNT 3

static const char * const s_photo_files[RATIMOS_PHOTO_COUNT] = {
    "assets/mock/photos/foto1.txt",
    "assets/mock/photos/foto2.txt",
    "assets/mock/photos/foto3.txt",
};

static ratimos_photo_t s_photos[RATIMOS_PHOTO_COUNT];
static size_t s_photo_count = 0;

/* Le a primeira linha de `path` em `out`, com fallback seguro "sem titulo"
 * caso o arquivo nao exista, nao possa ser lido, ou a primeira linha esteja
 * em branco — mesma disciplina defensiva de letters.c. */
static void read_title_or_default(const char * path, char * out, size_t out_size)
{
    FILE * f = fopen(path, "r");
    if (!f) {
        snprintf(out, out_size, "sem titulo");
        return;
    }

    char line[128];
    if (!fgets(line, sizeof(line), f)) {
        snprintf(out, out_size, "sem titulo");
        fclose(f);
        return;
    }
    fclose(f);

    line[strcspn(line, "\r\n")] = '\0';

    if (line[0] == '\0') {
        snprintf(out, out_size, "sem titulo");
    } else {
        snprintf(out, out_size, "%s", line);
    }
}

void ratimos_storage_index_photos(void)
{
    s_photo_count = RATIMOS_PHOTO_COUNT;

    for (size_t i = 0; i < RATIMOS_PHOTO_COUNT; i++) {
        snprintf(s_photos[i].id, sizeof(s_photos[i].id), "photo%zu", i + 1);
        read_title_or_default(s_photo_files[i], s_photos[i].title, sizeof(s_photos[i].title));
    }
}

size_t ratimos_storage_list_photos(ratimos_photo_t * out, size_t max_count)
{
    size_t n = s_photo_count < max_count ? s_photo_count : max_count;
    memcpy(out, s_photos, n * sizeof(ratimos_photo_t));
    return n;
}
