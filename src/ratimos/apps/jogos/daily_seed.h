#ifndef RATIMOS_JOGOS_DAILY_SEED_H
#define RATIMOS_JOGOS_DAILY_SEED_H

#include <stdint.h>

/*
 * Semente diaria compartilhada por todos os jogos com modo diario.
 *
 * ATENCAO (assuncao A5 do RESEARCH): no `native_sim` nao existe RTC — "hoje"
 * vem do relogio do PC hospedeiro. A Fase 4 liga o RTC PCF85063 real e passa
 * a alimentar estas duas funcoes; ate la, um relogio de PC errado produz o
 * quebra-cabeca do dia errado. Impacto restrito ao desenvolvimento.
 */

/* Dias inteiros desde a epoch Unix. Muda exatamente uma vez por dia (UTC). */
uint32_t ratimos_daily_index(void);

/* Semente por jogo: o mesmo dia gera conteudo diferente em cada jogo (e,
 * depois, em cada modo do termo) porque o `stream` separa os fluxos. */
uint32_t ratimos_daily_seed(uint8_t stream);

#endif
