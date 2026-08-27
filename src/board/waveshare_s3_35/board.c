/*
 * Board/HAL — stub waveshare_s3_35 (D-07).
 *
 * Ainda nao existe ambiente PlatformIO `esp32s3` nem toolchain ESP-IDF neste
 * projeto — este arquivo existe apenas para provar que o contrato de
 * src/board/board.h "compila como uma unidade de traducao" com um gcc puro
 * (`gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c`).
 * Por isso NAO inclui nenhum header ESP-IDF/Arduino/esp_lcd — apenas
 * "../board.h". A implementacao real (ST7796 + FT6336) e trabalho da Fase 3.
 *
 * Excluido do build de native_sim via build_src_filter em platformio.ini,
 * para nunca colidir (duplicate symbol) com src/board/native_sdl/board.c.
 */
#include "../board.h"

void board_display_init(void)
{
    /* TODO (Fase 3): inicializar painel ST7796 via QSPI/esp_lcd */
}

void board_input_init(void)
{
    /* TODO (Fase 3): inicializar touch FT6336 via I2C */
}

void board_tick(uint32_t idle_time_ms)
{
    (void) idle_time_ms;
    /* TODO (Fase 3): pausa compativel com FreeRTOS (vTaskDelay) */
}
