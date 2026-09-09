/*
 * GERADO por tools/convert_images.py a partir de assets/progress/manifest.json -- nao
 * editar a mao. Reexecute o script se a arte fonte mudar.
 *
 * Formato de imagem indexado de 4 bits/pixel do LVGL (paleta de ate 16
 * cores) -- ver "Icon & Title Font Asset Pipeline" em 02.1-UI-SPEC.md.
 */
#ifndef RATIMOS_PROGRESS_IMAGES_H
#define RATIMOS_PROGRESS_IMAGES_H

#include "lvgl.h"
#include <stddef.h>

extern const lv_image_dsc_t ratimos_progress_castle_stage_00_terreno_vazio_desc;
extern const lv_image_dsc_t ratimos_progress_castle_stage_01_alicerce_desc;
extern const lv_image_dsc_t ratimos_progress_castle_stage_02_muros_canteiro_desc;
extern const lv_image_dsc_t ratimos_progress_castle_stage_03_torres_florindo_desc;
extern const lv_image_dsc_t ratimos_progress_castle_stage_04_bandeira_jardim_cheio_desc;
extern const lv_image_dsc_t ratimos_progress_castle_stage_05_completo_desc;
extern const lv_image_dsc_t ratimos_progress_unlock_sudoku_roseira_desc;
extern const lv_image_dsc_t ratimos_progress_unlock_paciencia_bandeira_desc;
extern const lv_image_dsc_t ratimos_progress_unlock_termo_arvore_desc;
extern const lv_image_dsc_t ratimos_progress_unlock_cruzadinha_fonte_desc;
extern const lv_image_dsc_t ratimos_progress_unlock_conexo_portao_desc;

const lv_image_dsc_t * ratimos_progress_by_id(const char * id);

#endif
