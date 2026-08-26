/*
 * RatimOS — ponto de entrada do simulador de PC (Fase 0 do plano).
 * Roda a UI inteira numa janela SDL2, sem precisar de nenhuma placa.
 */
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ratimos/theme.h"
#include "ratimos/home_screen.h"

int main(void)
{
    lv_init();
    lv_tick_set_cb(SDL_GetTicks);

    lv_display_t * disp = lv_sdl_window_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    lv_indev_t * mouse = lv_sdl_mouse_create();
    (void) disp;
    (void) mouse;

    ratimos_home_screen_show(NULL);

    while (1) {
        uint32_t idle = lv_timer_handler();
        SDL_Delay(idle > 0 ? idle : 5);
    }

    return 0;
}
