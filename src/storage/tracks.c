/*
 * Storage/Content API — dominio `tracks` (musica).
 *
 * Mesma estrutura de letters.c/photos.c: manifesto fixo e hardcoded (nunca
 * dirent.h/directory scanning), cache-on-index, safe-default-title.
 *
 * `ratimos_storage_tracks_read_title_or_default()` tem linkage externa
 * (nao-static) apenas para permitir a Test 5 do plano 01-03 exercitar o
 * caminho defensivo (fallback "sem titulo") sem precisar corromper nenhuma
 * das 3 fixtures reais usadas pelo UAT da fase — nao faz parte do contrato
 * publico declarado em content_api.h.
 */
#include <stdio.h>
#include <string.h>
#include "content_api.h"

#define RATIMOS_TRACK_COUNT 3

static const char * const s_track_files[RATIMOS_TRACK_COUNT] = {
    "assets/mock/tracks/faixa1.txt",
    "assets/mock/tracks/faixa2.txt",
    "assets/mock/tracks/faixa3.txt",
};

static ratimos_track_t s_tracks[RATIMOS_TRACK_COUNT];
static size_t s_track_count = 0;

void ratimos_storage_tracks_read_title_or_default(const char * path, char * out, size_t out_size)
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

void ratimos_storage_index_tracks(void)
{
    s_track_count = RATIMOS_TRACK_COUNT;

    for (size_t i = 0; i < RATIMOS_TRACK_COUNT; i++) {
        snprintf(s_tracks[i].id, sizeof(s_tracks[i].id), "track%zu", i + 1);
        ratimos_storage_tracks_read_title_or_default(s_track_files[i], s_tracks[i].title, sizeof(s_tracks[i].title));
    }
}

size_t ratimos_storage_list_tracks(ratimos_track_t * out, size_t max_count)
{
    size_t n = s_track_count < max_count ? s_track_count : max_count;
    memcpy(out, s_tracks, n * sizeof(ratimos_track_t));
    return n;
}
