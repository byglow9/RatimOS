#ifndef RATIMOS_BOOT_ART_H
#define RATIMOS_BOOT_ART_H

/* GERADO por tools/generate_boot_art.py -- nao editar a mao. */
#include "lvgl.h"

/* Mastro da variante ameias (sketch 010). */
#define RATIMOS_BOOT_MAST_HEX 0xd9d0e6
/* Linha do logo onde o sprite do brilho do letreiro comeca (o sprite so' tem as linhas com conteudo). */
#define RATIMOS_BOOT_GLOW_Y0 25
#define RATIMOS_BOOT_GLOW_X0 34

extern const lv_image_dsc_t ratimos_boot_flag_desc;
extern const lv_image_dsc_t ratimos_boot_wall_lit_desc;
extern const lv_image_dsc_t ratimos_boot_wall_unlit_desc;
extern const lv_image_dsc_t ratimos_boot_wordmark_glow_desc;
extern const lv_image_dsc_t ratimos_boot_rook_empty_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_0_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_1_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_2_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_3_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_4_desc;
extern const lv_image_dsc_t ratimos_boot_rook_full_5_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_0_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_1_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_2_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_3_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_4_desc;
extern const lv_image_dsc_t ratimos_boot_rook_glint_5_desc;
extern const lv_image_dsc_t ratimos_boot_thumb_bandeira_desc;
extern const lv_image_dsc_t ratimos_boot_thumb_ameias_desc;
extern const lv_image_dsc_t ratimos_boot_thumb_letreiro_desc;
extern const lv_image_dsc_t ratimos_boot_thumb_torres_desc;

#endif
