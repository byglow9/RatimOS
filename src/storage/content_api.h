#ifndef RATIMOS_STORAGE_CONTENT_API_H
#define RATIMOS_STORAGE_CONTENT_API_H

#include <stddef.h>

/*
 * Storage/Content API do RatimOS (D-09/D-10/D-11).
 *
 * Contrato unico, sincrono e sem alocacao dinamica, compartilhado pelos 5
 * dominios de conteudo do dispositivo. Cada app le exclusivamente atraves
 * destas funcoes — nunca faz I/O de arquivo diretamente.
 *
 * Nesta fase (01-01) apenas o dominio `letters` esta realmente implementado
 * (src/storage/letters.c); os demais tem apenas a assinatura declarada aqui,
 * sem arquivo .c correspondente ainda (job do plano 01-03).
 */

typedef struct {
    char id[16];
    char title[64];
} ratimos_photo_t;

typedef struct {
    char id[16];
    char title[64];
} ratimos_track_t;

typedef struct {
    char id[16];
    char title[64];
} ratimos_letter_t;

typedef struct {
    char id[16];
    char title[64];
} ratimos_game_t;

typedef struct {
    int brightness_pct;
    int volume_pct;
    char firmware_version[16];
    char storage_used_label[32];
} ratimos_settings_t;

/* Monta o backend de storage (mock/native nesta fase). Deve ser chamada uma
 * unica vez, no primeiro passo do splash, antes de qualquer index_*(). */
void ratimos_storage_mount(void);

/* Passos de indexacao — cada um le as fixtures do seu dominio do disco uma
 * unica vez e popula um cache estatico em memoria (cache-on-index, nunca
 * read-on-get — ver Pitfall 1 do RESEARCH.md). Chamados pelo splash. */
void ratimos_storage_index_photos(void);
void ratimos_storage_index_tracks(void);
void ratimos_storage_index_letters(void);
void ratimos_storage_index_games(void);
void ratimos_storage_index_settings(void);

/* Getters — apenas copiam do cache populado pelo index_*() correspondente,
 * nunca voltam a tocar disco. Retornam a quantidade de itens copiados para
 * `out` (sempre <= max_count). */
size_t ratimos_storage_list_photos(ratimos_photo_t * out, size_t max_count);
size_t ratimos_storage_list_tracks(ratimos_track_t * out, size_t max_count);
size_t ratimos_storage_list_letters(ratimos_letter_t * out, size_t max_count);
size_t ratimos_storage_list_games(ratimos_game_t * out, size_t max_count);
ratimos_settings_t ratimos_storage_get_settings(void);

#endif
