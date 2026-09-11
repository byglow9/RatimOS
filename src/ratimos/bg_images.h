/*
 * GERADO por tools/convert_images.py a partir de assets/backgrounds/manifest.json -- nao
 * editar a mao. Reexecute o script se a arte fonte mudar.
 *
 * Formato de imagem indexado de 4 bits/pixel do LVGL (paleta de ate 16
 * cores) -- ver "Icon & Title Font Asset Pipeline" em 02.1-UI-SPEC.md.
 */
#ifndef RATIMOS_BG_IMAGES_H
#define RATIMOS_BG_IMAGES_H

#include "lvgl.h"
#include <stddef.h>

extern const lv_image_dsc_t ratimos_bg_dither_desc;

const lv_image_dsc_t * ratimos_bg_by_id(const char * id);

#endif
