#include "boot_styles.h"
#include "logo_image.h"
#include "splash.h"
#include "theme.h"

#include <stdio.h>

/*
 * Rotinas dos carregamentos (plano 02.1-16). Geometria FINAL do sketch 010
 * (`BOOTS.*.draw`), que substitui a do 008 onde diferem. Todas desenham so'
 * assets com alfa (ratimos_logo_alpha_desc e a arte gerada) por cima do
 * fundo do tema que a splash ja pintou.
 *
 * Cada rotina redesenha a composicao INTEIRA a cada chamada: o clip do layer
 * (a area invalidada) decide o que de fato gasta CPU. Quem decide quanto
 * invalidar e' ratimos_boot_style_dirty_area().
 */

#define LOGO_X 32
#define LOGO_W 256
#define LOGO_H 171

/* --------------------------------------------------------------------- *
 * Primitivas
 * --------------------------------------------------------------------- */

/*
 * Desenha um RETANGULO de `dsc` (origem sx,sy; tamanho w x h) em (dx,dy),
 * com opacidade `opa`. A imagem INTEIRA e' posta em (dx-sx, dy-sy) e o clip
 * do layer e' reduzido ao destino enquanto a draw task e' criada -- o LVGL
 * guarda o clip na task, entao restaurar logo em seguida e' seguro. Sem
 * buffer intermediario, sem heap por quadro.
 */
static void blit(lv_layer_t * layer, const lv_image_dsc_t * dsc, int32_t sx, int32_t sy, int32_t w,
                 int32_t h, int32_t dx, int32_t dy, lv_opa_t opa)
{
    if (opa == LV_OPA_TRANSP || w <= 0 || h <= 0) {
        return;
    }
    lv_area_t dest = { dx, dy, dx + w - 1, dy + h - 1 };
    lv_area_t saved = layer->_clip_area;
    lv_area_t clip;
    if (!lv_area_intersect(&clip, &saved, &dest)) {
        return; /* fora da area suja: nada a fazer */
    }

    lv_draw_image_dsc_t d;
    lv_draw_image_dsc_init(&d);
    d.src = dsc;
    d.opa = opa;
    lv_area_t img = { dx - sx, dy - sy, dx - sx + (int32_t) dsc->header.w - 1,
                      dy - sy + (int32_t) dsc->header.h - 1 };

    layer->_clip_area = clip;
    lv_draw_image(layer, &d, &img);
    layer->_clip_area = saved;
}

static void fill_rect(lv_layer_t * layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t c,
                      lv_opa_t opa)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = c;
    d.bg_opa = opa;
    d.border_width = 0;
    lv_area_t a = { x1, y1, x2, y2 };
    lv_draw_rect(layer, &d, &a);
}

/* Cor RGB565 do pixel (x,y) do logo, expandida para 8 bits. A cor do mastro
 * vem do PROPRIO array do logo -- nenhum hex no codigo. */
static lv_color_t logo_pixel(int32_t x, int32_t y)
{
    const uint8_t * base = ratimos_logo_alpha_desc.data;
    const uint8_t * p = base + (size_t) y * ratimos_logo_alpha_desc.header.stride + (size_t) x * 2;
    uint16_t v = (uint16_t) (p[0] | (p[1] << 8));
    uint8_t r = (uint8_t) (((v >> 11) & 0x1f) << 3);
    uint8_t g = (uint8_t) (((v >> 5) & 0x3f) << 2);
    uint8_t b = (uint8_t) ((v & 0x1f) << 3);
    return lv_color_make((uint8_t) (r | (r >> 5)), (uint8_t) (g | (g >> 6)), (uint8_t) (b | (b >> 5)));
}

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* --------------------------------------------------------------------- *
 * BANDEIRA ("hastear a bandeira", padrao) -- BOOTS.bandeira do 010
 * --------------------------------------------------------------------- */

#define B_TOP        86  /* LOGO_Y(110) - 24 */
#define B_MAST_X     125
#define B_MAST_W     3
#define B_MAST_SLICE 40  /* linha do logo que o mastro estica */
#define B_FLAG_X     129
#define B_FLAG_Y     31
#define B_FLAG_W     29
#define B_FLAG_H     16
#define B_BASE       47
#define B_EXT        64

int32_t ratimos_boot_flag_rise_px(int32_t units)
{
    units = clamp32(units, 0, RATIMOS_SPLASH_UNITS);
    return (units * (B_EXT / 2) / RATIMOS_SPLASH_UNITS) * 2;
}

/* A bandeira do logo, linha a linha: cada grupo de 3 linhas desloca 1px,
 * alternando a cada onda (`phase`). */
static void draw_flag_rows(lv_layer_t * layer, int32_t dx, int32_t dy, uint8_t phase, lv_opa_t opa)
{
    for (int32_t r = 0; r < B_FLAG_H; r++) {
        int32_t o = ((r / 3) + phase) % 2;
        blit(layer, &ratimos_logo_alpha_desc, B_FLAG_X, B_FLAG_Y + r, B_FLAG_W, 1, dx + o, dy + r, opa);
    }
}

static void bandeira_draw(lv_layer_t * layer, const ratimos_boot_frame_t * f)
{
    const lv_image_dsc_t * logo = &ratimos_logo_alpha_desc;
    int32_t rise = ratimos_boot_flag_rise_px(f->units);

    /* (1) do topo ate a bandeira */
    blit(layer, logo, 0, 0, LOGO_W, B_FLAG_Y, LOGO_X, B_TOP, f->opa);
    /* (2) o inicio do mastro (2 linhas) */
    blit(layer, logo, B_MAST_X, B_FLAG_Y, B_MAST_W, 2, LOGO_X + B_MAST_X, B_TOP + B_FLAG_Y, f->opa);
    /* (3) o mastro esticado: a linha 40 repetida, remontada sem emenda. Uma
     * coluna por retangulo, cada uma com a cor lida do array do logo. */
    int32_t mast_y1 = B_TOP + B_FLAG_Y + 2;
    int32_t mast_y2 = mast_y1 + (B_BASE - B_FLAG_Y - 2 + B_EXT) - 1;
    for (int32_t c = 0; c < B_MAST_W; c++) {
        int32_t x = LOGO_X + B_MAST_X + c;
        fill_rect(layer, x, mast_y1, x, mast_y2, logo_pixel(B_MAST_X + c, B_MAST_SLICE), f->opa);
    }
    /* (4) o resto do logo (torre, letreiro) deslocado B_EXT para baixo */
    blit(layer, logo, 0, B_BASE, LOGO_W, LOGO_H - B_BASE, LOGO_X, B_TOP + B_BASE + B_EXT, f->opa);
    /* (5) a bandeira, subindo e tremulando. Nada ao lado do mastro. */
    draw_flag_rows(layer, LOGO_X + B_FLAG_X, B_TOP + B_FLAG_Y + B_EXT - rise, f->wave_phase, f->opa);
}

static bool bandeira_dirty(int32_t before, int32_t after, lv_area_t * out)
{
    if (ratimos_boot_flag_rise_px(before) == ratimos_boot_flag_rise_px(after)) {
        return false;
    }
    /* x 161..191 (bandeira + 1px de tremulo), y 117 (topo final) .. 196 (base inicial) */
    lv_area_set(out, LOGO_X + B_FLAG_X, B_TOP + B_FLAG_Y, LOGO_X + B_FLAG_X + B_FLAG_W + 1,
                B_TOP + B_FLAG_Y + B_EXT + B_FLAG_H - 1);
    return true;
}

static bool bandeira_wave(bool done, lv_area_t * out)
{
    (void) done;
    lv_area_set(out, LOGO_X + B_FLAG_X, B_TOP + B_FLAG_Y, LOGO_X + B_FLAG_X + B_FLAG_W + 1,
                B_TOP + B_FLAG_Y + B_EXT + B_FLAG_H - 1);
    return true;
}

/* --------------------------------------------------------------------- *
 * Tabela / despacho
 * --------------------------------------------------------------------- */

typedef struct {
    const char * name;
    const char * pct_prefix;
    int32_t text_y;
    void (*draw)(lv_layer_t * layer, const ratimos_boot_frame_t * f);
    bool (*dirty)(int32_t before, int32_t after, lv_area_t * out);
    bool (*wave)(bool done, lv_area_t * out);
} style_def_t;

/* Indexada pelo enum. Entrada sem `draw` cai em BANDEIRA (defensivo). */
static const style_def_t s_styles[RATIMOS_BOOT_STYLE_COUNT] = {
    [RATIMOS_BOOT_STYLE_BANDEIRA] = {
        .name = "hastear a bandeira",
        .pct_prefix = "",
        .text_y = B_TOP + LOGO_H + B_EXT + 18, /* 339 */
        .draw = bandeira_draw,
        .dirty = bandeira_dirty,
        .wave = bandeira_wave,
    },
};

static const style_def_t * resolve(ratimos_boot_style_t style)
{
    if ((int) style >= 0 && (int) style < (int) RATIMOS_BOOT_STYLE_COUNT && s_styles[style].draw != NULL) {
        return &s_styles[style];
    }
    return &s_styles[RATIMOS_BOOT_STYLE_BANDEIRA];
}

void ratimos_boot_style_draw(ratimos_boot_style_t style, lv_layer_t * layer,
                             const ratimos_boot_frame_t * frame)
{
    if (layer == NULL || frame == NULL) {
        return;
    }
    resolve(style)->draw(layer, frame);
}

bool ratimos_boot_style_dirty_area(ratimos_boot_style_t style, int32_t units_before,
                                   int32_t units_after, lv_area_t * out)
{
    if (out == NULL) {
        return false;
    }
    const style_def_t * def = resolve(style);
    return def->dirty != NULL && def->dirty(units_before, units_after, out);
}

bool ratimos_boot_style_wave_area(ratimos_boot_style_t style, bool done, lv_area_t * out)
{
    if (out == NULL) {
        return false;
    }
    const style_def_t * def = resolve(style);
    return def->wave != NULL && def->wave(done, out);
}

int32_t ratimos_boot_style_text_y(ratimos_boot_style_t style)
{
    return resolve(style)->text_y;
}

void ratimos_boot_style_format_pct(ratimos_boot_style_t style, int32_t units, char * buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }
    units = clamp32(units, 0, RATIMOS_SPLASH_UNITS);
    int32_t pct = (units * 100 + RATIMOS_SPLASH_UNITS / 2) / RATIMOS_SPLASH_UNITS;
    snprintf(buf, len, "%s%03d%%", resolve(style)->pct_prefix, (int) pct);
}

const char * ratimos_boot_style_name(ratimos_boot_style_t style)
{
    return resolve(style)->name;
}
