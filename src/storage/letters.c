/*
 * Storage/Content API — dominio `letters` (cartas).
 *
 * Unico dominio totalmente real nesta fase (01-01): le fixtures de disco
 * (D-12) a partir de um manifesto fixo e hardcoded (nunca dirent.h/directory
 * scanning — disciplina V12 de path-traversal do RESEARCH.md), com
 * cache-on-index (Pitfall 1): index_letters() faz o I/O uma unica vez e
 * popula um cache estatico; list_letters() so faz memcpy do cache.
 */
#include <stdio.h>
#include <string.h>
#include "content_api.h"

#define RATIMOS_LETTER_COUNT 3

static const char * const s_letter_files[RATIMOS_LETTER_COUNT] = {
    "assets/mock/letters/carta1.txt",
    "assets/mock/letters/carta2.txt",
    "assets/mock/letters/carta3.txt",
};

static ratimos_letter_t s_letters[RATIMOS_LETTER_COUNT];
static size_t s_letter_count = 0;

void ratimos_storage_index_letters(void)
{
    s_letter_count = RATIMOS_LETTER_COUNT;

    for (size_t i = 0; i < RATIMOS_LETTER_COUNT; i++) {
        snprintf(s_letters[i].id, sizeof(s_letters[i].id), "letter%zu", i + 1);

        FILE * f = fopen(s_letter_files[i], "r");
        if (!f) {
            snprintf(s_letters[i].title, sizeof(s_letters[i].title), "sem titulo");
            continue;
        }

        char line[128];
        if (!fgets(line, sizeof(line), f)) {
            snprintf(s_letters[i].title, sizeof(s_letters[i].title), "sem titulo");
            fclose(f);
            continue;
        }
        fclose(f);

        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0') {
            snprintf(s_letters[i].title, sizeof(s_letters[i].title), "sem titulo");
        } else {
            snprintf(s_letters[i].title, sizeof(s_letters[i].title), "%s", line);
        }
    }
}

size_t ratimos_storage_list_letters(ratimos_letter_t * out, size_t max_count)
{
    size_t n = s_letter_count < max_count ? s_letter_count : max_count;
    memcpy(out, s_letters, n * sizeof(ratimos_letter_t));
    return n;
}
