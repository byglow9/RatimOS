#ifndef RATIMOS_STORAGE_SAVE_IO_H
#define RATIMOS_STORAGE_SAVE_IO_H

/*
 * I/O de arquivo de save COMPARTILHADO entre os dominios de escrita da
 * Storage API (game_state.c e settings.c). Header INTERNO de src/storage/:
 * nenhum app em src/ratimos/ inclui isto -- apps so' enxergam content_api.h.
 *
 * Extraido de game_state.c no plano 02.1-16 (comportamento identico) para que
 * settings.c reuse a mesma escrita atomica em vez de duplica-la.
 */
#include <stdbool.h>
#include <stddef.h>

/* Diretorio dos saves do simulador (no dispositivo, saves de jogo vao pro SD
 * e settings vai pra NVS -- ver settings.c). */
#define RATIMOS_SAVE_DIR      "assets/save"
#define RATIMOS_SAVE_PATH_MAX 64

/* Cria assets/ e assets/save/ se ainda nao existirem. EEXIST e' o caso
 * normal (segundo boot em diante); qualquer outra falha e' silenciosa -- os
 * saves entao falham de forma graciosa em vez de derrubar o boot. */
void ratimos_save_ensure_dir(void);

/* Escrita atomica: grava "<path>.tmp", fecha e so' entao renomeia por cima do
 * caminho final. Um processo morto no meio da escrita deixa no maximo um
 * .tmp orfao; o arquivo anterior continua intacto. */
bool ratimos_save_write_atomic(const char * path, const void * data, size_t size);

/* Le exatamente `size` bytes. Arquivo MAIOR ou MENOR que isso e' recusado
 * (truncamento e lixo extra sao os dois lados da mesma falha). `out_missing`
 * (opcional) fica true quando o arquivo simplesmente nao existe. */
bool ratimos_save_read_exact(const char * path, void * out, size_t size, bool * out_missing);

#endif
