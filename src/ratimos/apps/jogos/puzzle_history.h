#ifndef RATIMOS_JOGOS_PUZZLE_HISTORY_H
#define RATIMOS_JOGOS_PUZZLE_HISTORY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Buffer circular de quebra-cabecas recentes, compartilhado por todo jogo com
 * banco de conteudo compilado (conexo agora; cruzadinha depois).
 *
 * Serializavel: vive dentro do estado salvo de cada jogo, entao a memoria do
 * que ja saiu sobrevive a um reboot.
 */
#define RATIMOS_RECENT_PUZZLES_TRACKED 10

typedef struct {
    /* Guarda indice+1: 0 significa "slot vazio", para que uma struct zerada
     * (jogo novo) nao pareca ja ter visto o quebra-cabeca 0. */
    uint16_t recent[RATIMOS_RECENT_PUZZLES_TRACKED];
    uint8_t write_pos;
} ratimos_puzzle_history_t;

/*
 * Sorteia um indice em [0, pool_count) evitando os recentes.
 *
 * Retorna 0 quando `pool_count` e 0 — o chamador DEVE tratar um banco vazio
 * como o caso "indisponivel" antes de usar o retorno.
 *
 * Limite de escala honesto (nao e bug): com no maximo 16 tentativas, um banco
 * menor ou proximo da janela rastreada acaba permitindo repeticao. Com 5
 * quebra-cabecas e janela de 10 isso acontece rapido; e o comportamento
 * aceito ate o banco crescer.
 */
size_t ratimos_puzzle_pick(const ratimos_puzzle_history_t * history, size_t pool_count, uint32_t seed);

/* Registra `index` como recem-usado e avanca o cursor do anel. */
void ratimos_puzzle_history_push(ratimos_puzzle_history_t * history, uint16_t index);

#endif
