#ifndef RATIMOS_SPLASH_H
#define RATIMOS_SPLASH_H

#include "lvgl.h"

/* Tela de boot (D-01 a D-04): logo com fade-in + barra de progresso real,
 * ligada aos passos de inicializacao da Storage/Content API (D-02/D-03).
 * Nao aceita toque (D-04) -- sempre roda ate o fim, sem pular. */
void ratimos_splash_show(void);

/* Barra de progresso (02.1-14): RATIMOS_SPLASH_BLOCKS blocos; `lit` acesos
 * agora (anda 1 por vez) e `target` = blocos justificados pelos passos
 * reais ja concluidos. Invariante: lit <= target. Expostos pro teste. */
#define RATIMOS_SPLASH_BLOCKS 25
uint8_t ratimos_splash_blocks_lit(void);
uint8_t ratimos_splash_blocks_target(void);

#endif
