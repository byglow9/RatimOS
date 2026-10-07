#ifndef RATIMOS_HEAP_PROBE_H
#define RATIMOS_HEAP_PROBE_H

/*
 * Medidor do heap do LVGL (plano 02.1-15) -- o mesmo numero que
 * lv_mem_monitor() reporta, num formato pronto pra teste e diagnostico.
 *
 * So' vale com o alocador builtin do LVGL (LV_USE_STDLIB_MALLOC ==
 * LV_STDLIB_BUILTIN, lv_conf.h): com malloc do sistema, lv_mem_monitor()
 * devolve zeros e estas funcoes tambem.
 *
 * Uso tipico num teste (ver test/test_navigation_memory):
 *
 *     ratimos_home_screen_show(NULL);           // tela base, em cache
 *     size_t base = ratimos_heap_used();
 *     ratimos_sudoku_show(NULL);
 *     size_t cost = ratimos_heap_used() - base; // custo da tela de jogo
 *     ratimos_home_screen_show(NULL);           // sai: a tela e' deletada
 *     // ratimos_heap_used() volta pra ~base
 */
#include <stddef.h>
#include <stdint.h>

/* Bytes em uso agora (total - livre, inclui overhead do TLSF). */
size_t ratimos_heap_used(void);

/* Tamanho total do heap (LV_MEM_SIZE + pools extras adicionados). */
size_t ratimos_heap_total(void);

/* Maior uso ja visto desde lv_init() (pico). */
size_t ratimos_heap_peak(void);

/* Uso agora, em % do total (0-100). */
uint8_t ratimos_heap_used_pct(void);

#endif
