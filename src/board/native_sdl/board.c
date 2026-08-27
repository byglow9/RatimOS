/*
 * Board/HAL — implementacao native_sdl (simulador de PC via SDL2).
 * Todo o codigo SDL2 do projeto vive exclusivamente aqui (extraido de
 * src/main.c) — nenhum outro arquivo deve incluir <SDL2/SDL.h>.
 */
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "../board.h"
#include "../../ratimos/theme.h"

void board_display_init(void)
{
    lv_display_t * disp = lv_sdl_window_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    (void) disp;
    lv_tick_set_cb(SDL_GetTicks);
}

void board_input_init(void)
{
    lv_indev_t * mouse = lv_sdl_mouse_create();
    (void) mouse;
}

void board_tick(uint32_t idle_time_ms)
{
    SDL_Delay(idle_time_ms > 0 ? idle_time_ms : 5);
}
