/*
 * I/O de arquivo de save compartilhado (plano 02.1-16). Extraido sem mudar
 * comportamento de game_state.c -- ver save_io.h.
 */
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include "save_io.h"

void ratimos_save_ensure_dir(void)
{
    if (mkdir("assets", 0777) != 0 && errno != EEXIST) {
        /* sem diretorio, os saves simplesmente falham de forma silenciosa e
         * o jogador cai no caminho ABSENT -- nunca um crash. */
    }
    if (mkdir(RATIMOS_SAVE_DIR, 0777) != 0 && errno != EEXIST) {
        /* idem */
    }
}

/*
 * Escrita atomica: grava em "<path>.tmp", fecha, e so entao renomeia por cima
 * do caminho final. rename() e atomico dentro do mesmo diretorio, entao um
 * processo morto no meio da escrita deixa no maximo um .tmp orfao -- o save
 * anterior, completo, continua intacto (ameaca T-02.1-04).
 */
bool ratimos_save_write_atomic(const char * path, const void * data, size_t size)
{
    char tmp[RATIMOS_SAVE_PATH_MAX + 8];

    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int) sizeof(tmp)) {
        return false;
    }

    FILE * f = fopen(tmp, "wb");
    if (!f) {
        return false;
    }

    bool ok = (fwrite(data, 1, size, f) == size) && (fflush(f) == 0);

    if (fclose(f) != 0) {
        ok = false;
    }

    if (!ok || rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }

    return true;
}

bool ratimos_save_read_exact(const char * path, void * out, size_t size, bool * out_missing)
{
    if (out_missing) {
        *out_missing = false;
    }

    FILE * f = fopen(path, "rb");
    if (!f) {
        if (out_missing) {
            *out_missing = true;
        }
        return false;
    }

    size_t got = fread(out, 1, size, f);
    /* fgetc extra: se ainda houver byte depois de `size`, o arquivo e maior
     * que o registro esperado e portanto invalido. */
    bool has_trailing = (fgetc(f) != EOF);
    fclose(f);

    return got == size && !has_trailing;
}
