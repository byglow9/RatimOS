#ifndef RATIMOS_SPLASH_H
#define RATIMOS_SPLASH_H

#include "lvgl.h"

/* Tela de boot (D-01 a D-04): logo com fade-in + barra de progresso real,
 * ligada aos passos de inicializacao da Storage/Content API (D-02/D-03).
 * Nao aceita toque (D-04) -- sempre roda ate o fim, sem pular. */
void ratimos_splash_show(void);

#endif
