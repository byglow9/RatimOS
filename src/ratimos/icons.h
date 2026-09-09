/*
 * GERADO por tools/convert_images.py a partir de assets/icons/manifest.json -- nao
 * editar a mao. Reexecute o script se a arte fonte mudar.
 *
 * Formato de imagem indexado de 4 bits/pixel do LVGL (paleta de ate 16
 * cores) -- ver "Icon & Title Font Asset Pipeline" em 02.1-UI-SPEC.md.
 */
#ifndef RATIMOS_ICONS_H
#define RATIMOS_ICONS_H

#include "lvgl.h"
#include <stddef.h>

extern const lv_image_dsc_t ratimos_icon_home_jogos_desc;
extern const lv_image_dsc_t ratimos_icon_home_musica_desc;
extern const lv_image_dsc_t ratimos_icon_home_album_desc;
extern const lv_image_dsc_t ratimos_icon_home_cartas_desc;
extern const lv_image_dsc_t ratimos_icon_home_config_desc;
extern const lv_image_dsc_t ratimos_icon_home_castelo_desc;
extern const lv_image_dsc_t ratimos_icon_game_sudoku_desc;
extern const lv_image_dsc_t ratimos_icon_game_paciencia_desc;
extern const lv_image_dsc_t ratimos_icon_game_termo_desc;
extern const lv_image_dsc_t ratimos_icon_game_cruzadinha_desc;
extern const lv_image_dsc_t ratimos_icon_game_conexo_desc;

const lv_image_dsc_t * ratimos_icon_by_id(const char * id);

#endif
