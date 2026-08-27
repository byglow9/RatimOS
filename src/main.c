/*
 * RatimOS — ponto de entrada generico (D-08).
 *
 * Nao conhece SDL2 nem nenhum detalhe de placa especifico — so chama o
 * contrato de src/board/board.h. A variante linkada (native_sdl hoje,
 * waveshare_s3_35 na Fase 3) e decidida em tempo de build por
 * platformio.ini's build_src_filter.
 */
#include "lvgl.h"
#include "board/board.h"
#include "ratimos/splash.h"

int main(void)
{
    lv_init();
    board_display_init();
    board_input_init();

    ratimos_splash_show();

    while (1) {
        uint32_t idle = lv_timer_handler();
        board_tick(idle);
    }

    return 0;
}
