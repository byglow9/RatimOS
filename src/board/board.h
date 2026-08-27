#ifndef RATIMOS_BOARD_H
#define RATIMOS_BOARD_H

#include <stdint.h>

/*
 * Contrato de board/HAL do RatimOS (D-05).
 *
 * Cada variante de placa (native_sdl hoje, waveshare_s3_35 na Fase 3)
 * implementa exatamente estas 3 funcoes. `main.c` so conhece este contrato,
 * nunca os detalhes de SDL2/ESP-IDF por baixo.
 */

/* Inicializa o display (janela SDL2 hoje, painel ST7796 na Fase 3). */
void board_display_init(void);

/* Inicializa o input (mouse SDL2 hoje, touch FT6336 na Fase 3). */
void board_input_init(void);

/* Chamada a cada volta do loop principal com o tempo ocioso (ms) retornado
 * por `lv_timer_handler()` — cada placa decide como "dormir" esse tempo
 * (delay da SDL hoje, vTaskDelay/FreeRTOS na Fase 3). */
void board_tick(uint32_t idle_time_ms);

#endif
