#ifndef RATIMOS_STORAGE_CONTENT_API_H
#define RATIMOS_STORAGE_CONTENT_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

/* ------------------------------------------------------------------------
 * Dominio `game_state` — o PRIMEIRO dominio de escrita da Storage API (D-10).
 *
 * Os 5 dominios acima (letters/photos/tracks/games/settings) sao somente
 * leitura: indexam fixtures uma vez e nunca escrevem nada. Este dominio
 * quebra essa simetria de proposito — o progresso de um jogo precisa
 * sobreviver a um reboot, e o cache de tela em memoria (build-once) nao
 * sobrevive. Continua valendo a regra central: nenhum arquivo em
 * src/ratimos/apps/ toca disco; tudo passa por aqui.
 * ------------------------------------------------------------------------ */

/* Ordem canonica dos jogos — usada como indice em varios arrays de tamanho
 * fixo (tabela de caminhos de save, flags de desbloqueio). Qualquer valor
 * vindo de disco DEVE ser checado contra RATIMOS_GAME_COUNT antes do uso. */
typedef enum {
    RATIMOS_GAME_SUDOKU = 0,
    RATIMOS_GAME_PACIENCIA,
    RATIMOS_GAME_TERMO,
    RATIMOS_GAME_CRUZADINHA,
    RATIMOS_GAME_CONEXO,
    RATIMOS_GAME_COUNT
} ratimos_game_kind_t;

/* Blob opaco de estado: a Storage API nao sabe (nem quer saber) o que cada
 * jogo guarda aqui dentro — so garante que os `used` primeiros bytes voltam
 * identicos e na mesma ordem. Tamanho fixo, sem alocacao dinamica. */
#define RATIMOS_GAME_STATE_BLOB_SIZE 512

typedef struct {
    uint8_t bytes[RATIMOS_GAME_STATE_BLOB_SIZE];
    size_t used;
} ratimos_game_state_t;

typedef enum {
    RATIMOS_GAME_STATE_OK = 0,      /* save valido carregado */
    RATIMOS_GAME_STATE_ABSENT,      /* nunca houve save — comece um jogo novo */
    RATIMOS_GAME_STATE_INVALID      /* save corrompido/truncado — jogo novo + aviso */
} ratimos_game_state_status_t;

/* Teto do contador compartilhado (jardim/castelo, D-05). Existe para que um
 * arquivo adulterado nao consiga injetar um numero absurdo na UI. */
#define RATIMOS_PROGRESSION_MAX_COMPLETIONS 9999

/* Sentinela "nunca creditado" para last_win_day_index[] -- precisa ser um
 * valor que ratimos_daily_index() nunca produz na pratica (dias desde a
 * epoch Unix; um progression state novo/zerado nao pode ser confundido com
 * "ja venceu no dia 0"). UINT32_MAX corresponde a ~11.7 milhoes de anos no
 * futuro a partir da epoch, entao nunca colide com um dia real. */
#define RATIMOS_NO_WIN_DAY 0xFFFFFFFFu

typedef struct {
    uint16_t shared_completions;                        /* dias concluidos, somados entre todos os jogos */
    uint8_t game_exclusive_unlocked[RATIMOS_GAME_COUNT]; /* 0 ou 1 por jogo */
    /* CR-01: dia (ratimos_daily_index()) em que este jogo creditou o castelo
     * pela ultima vez. Guard de idempotencia POR DIA que sobrevive a um
     * reset client-side de `daily_win_recorded` -- ver
     * ratimos_storage_record_daily_win(). RATIMOS_NO_WIN_DAY = nunca
     * creditado. */
    uint32_t last_win_day_index[RATIMOS_GAME_COUNT];
} ratimos_progression_state_t;

/* Passo de boot: garante que o diretorio de saves existe. Nao le nada. */
void ratimos_storage_index_game_state(void);

/* Le o save de um jogo. Em QUALQUER caminho de falha `*out` sai zerado, para
 * que um chamador que ignore o status nunca leia bytes velhos. */
ratimos_game_state_status_t ratimos_storage_get_game_state(ratimos_game_kind_t game,
                                                           ratimos_game_state_t * out);

/* Responde apenas "existe progresso retomavel para este jogo" (o launcher
 * usa isto para decidir entre os rotulos "jogar"/"continuar"), sem devolver
 * o blob. Reaproveita a MESMA validacao de ratimos_storage_get_game_state —
 * probe e getter nunca podem discordar: um save corrompido ou vazio (0
 * bytes, residuo de escrita interrompida) responde que nao ha progresso. */
bool ratimos_storage_has_game_state(ratimos_game_kind_t game);

/* Grava o save de um jogo de forma atomica (arquivo temporario + rename). */
bool ratimos_storage_save_game_state(ratimos_game_kind_t game, const ratimos_game_state_t * state);

/* Apaga APENAS o tabuleiro salvo desse jogo. Nunca toca na progressao. */
bool ratimos_storage_clear_game_state(ratimos_game_kind_t game);

bool ratimos_storage_get_progression(ratimos_progression_state_t * out);
bool ratimos_storage_save_progression(const ratimos_progression_state_t * state);

/* Vitoria no modo diario: soma 1 no contador compartilhado e marca o
 * desbloqueio exclusivo do jogo. Nunca decrementa nem limpa nada.
 *
 * `day_index` (tipicamente ratimos_daily_index() do chamador) e comparado
 * contra o `last_win_day_index` ja persistido para este jogo (CR-01): se
 * este jogo ja foi creditado no MESMO dia, a chamada e um no-op idempotente
 * (retorna true sem somar de novo) -- protege o contador mesmo que o
 * chamador tenha perdido/zerado seu proprio flag `daily_win_recorded` em
 * memoria (ex.: um "novo jogo"/troca-de-modo que reconstroi o estado do
 * zero a partir da MESMA semente diaria). */
bool ratimos_storage_record_daily_win(ratimos_game_kind_t game, uint32_t day_index);


#endif
