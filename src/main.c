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
#include "ratimos/home_screen.h"
#include "storage/content_api.h"

int main(void)
{
    lv_init();
    board_display_init();
    board_input_init();

    /* Storage indexing normally runs as the splash's staged-progress steps
     * (D-02/D-03, Task 2 of this plan) -- ratimos_splash_show() will replace
     * this direct call site and drive ratimos_storage_mount()/index_letters()
     * itself, one step per lv_timer tick. Until the splash lands, call them
     * directly here so this tracer task's own claim -- "cartas renders real
     * fixture-backed letters through the Storage API" -- is actually true of
     * the running binary, not just of the (correct, but otherwise unreachable)
     * storage code. */
    ratimos_storage_mount();
    ratimos_storage_index_letters();

    ratimos_home_screen_show(NULL);

    while (1) {
        uint32_t idle = lv_timer_handler();
        board_tick(idle);
    }

    return 0;
}
