#include "boot_styles.h"
#include "boot_art.h"
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

/* A bandeira do logo (sprite com alfa gerado), linha a linha: cada grupo de 3
 * linhas desloca 1px, alternando a cada onda (`phase`). Compartilhada por B e A. */
static void draw_flag_rows(lv_layer_t * layer, int32_t dx, int32_t dy, uint8_t phase, lv_opa_t opa)
{
    for (int32_t r = 0; r < B_FLAG_H; r++) {
        int32_t o = ((r / 3) + phase) % 2;
        blit(layer, &ratimos_boot_flag_desc, 0, r, B_FLAG_W, 1, dx + o, dy + r, opa);
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
 * AMEIAS ("ameias do castelo") -- BOOTS.ameias do 010
 *
 * Uma muralha da largura do letreiro abaixo do logo, preenchida tijolo a
 * tijolo (24 colunas de 8px) com o degrade vertical do "RatimOS". No fim, o
 * mastro e a bandeira ondulando na ultima ameia.
 * --------------------------------------------------------------------- */

#define A_LOGO_Y   110
#define A_COLS     24
#define A_CW       8
#define A_WALL_X   65  /* contorno de 1px; a muralha comeca em x0 = 66 */
#define A_WALL_Y   302 /* y0 = 303 */
#define A_WALL_W   194
#define A_WALL_H   22
#define A_MAST_X   250 /* x0 + W - MW */
#define A_MAST_Y   285 /* y0 - FLAG.h - 2 */
#define A_MAST_W   2
#define A_MAST_H   18

static int32_t ameias_lit(int32_t units)
{
    return clamp32(units, 0, RATIMOS_SPLASH_UNITS) * A_COLS / RATIMOS_SPLASH_UNITS;
}

static void ameias_draw(lv_layer_t * layer, const ratimos_boot_frame_t * f)
{
    blit(layer, &ratimos_logo_alpha_desc, 0, 0, LOGO_W, LOGO_H, LOGO_X, A_LOGO_Y, f->opa);

    /* Aceso nas colunas de tijolo < lit (+ o contorno da esquerda); apagado no resto. */
    int32_t cut = ameias_lit(f->units) * A_CW + 1;
    blit(layer, &ratimos_boot_wall_lit_desc, 0, 0, cut, A_WALL_H, A_WALL_X, A_WALL_Y, f->opa);
    blit(layer, &ratimos_boot_wall_unlit_desc, cut, 0, A_WALL_W - cut, A_WALL_H, A_WALL_X + cut, A_WALL_Y, f->opa);

    if (f->done) {
        /* Bandeira fincada na ultima ameia: primeiro o mastro, DEPOIS a bandeira ondulando. */
        fill_rect(layer, A_MAST_X, A_MAST_Y, A_MAST_X + A_MAST_W - 1, A_MAST_Y + A_MAST_H - 1,
                  lv_color_hex(RATIMOS_BOOT_MAST_HEX), f->opa);
        draw_flag_rows(layer, A_MAST_X + A_MAST_W, A_MAST_Y, f->wave_phase, f->opa);
    }
}

static bool ameias_dirty(int32_t before, int32_t after, lv_area_t * out)
{
    bool wall = ameias_lit(before) != ameias_lit(after);
    bool flag = after >= RATIMOS_SPLASH_UNITS && before < RATIMOS_SPLASH_UNITS;
    if (!wall && !flag) {
        return false;
    }
    lv_area_set(out, A_WALL_X, flag ? A_MAST_Y : A_WALL_Y, A_MAST_X + A_MAST_W + B_FLAG_W,
                A_WALL_Y + A_WALL_H - 1);
    return true;
}

static bool ameias_wave(bool done, lv_area_t * out)
{
    if (!done) {
        return false; /* a bandeira so' existe (e so' ondula) no fim */
    }
    lv_area_set(out, A_MAST_X + A_MAST_W, A_MAST_Y, A_MAST_X + A_MAST_W + B_FLAG_W, A_MAST_Y + B_FLAG_H - 1);
    return true;
}

/* --------------------------------------------------------------------- *
 * LETREIRO ("letreiro que acende") -- BOOTS.letreiro do 010
 *
 * O logo comeca APAGADO (alfa a 17%, bandeira inclusa, parada). Uma varredura
 * acende da esquerda pra direita a coluna INTEIRA do logo (letreiro + torre +
 * bandeira) em passos de 4px, com um friso de brilho de 2 colunas na borda.
 * No fim: o logo inteiro aceso, sem friso, sem tremular.
 *
 * O "apagado" e' o MESMO sprite com alfa desenhado a opa 17%: fiel ao 010 sem
 * pixel por quadro, sem buffer em RAM e sem asset extra.
 * --------------------------------------------------------------------- */

#define C_LOGO_Y   110
#define C_WM_X0    34
#define C_WM_X1    226
#define C_STEP     4
#define C_DIM_OPA  43 /* 17% */

static int32_t letreiro_edge(int32_t units)
{
    units = clamp32(units, 0, RATIMOS_SPLASH_UNITS);
    return C_WM_X0 + ((units * (C_WM_X1 - C_WM_X0 + 1)) / (RATIMOS_SPLASH_UNITS * C_STEP)) * C_STEP;
}

static lv_opa_t scale_opa(lv_opa_t opa, int32_t part)
{
    return (lv_opa_t) ((int32_t) opa * part / 255);
}

static void letreiro_draw(lv_layer_t * layer, const ratimos_boot_frame_t * f)
{
    if (f->done) {
        blit(layer, &ratimos_logo_alpha_desc, 0, 0, LOGO_W, LOGO_H, LOGO_X, C_LOGO_Y, f->opa);
        return;
    }
    blit(layer, &ratimos_logo_alpha_desc, 0, 0, LOGO_W, LOGO_H, LOGO_X, C_LOGO_Y, scale_opa(f->opa, C_DIM_OPA));

    int32_t edge = letreiro_edge(f->units);
    if (edge > C_WM_X0) {
        blit(layer, &ratimos_logo_alpha_desc, 0, 0, edge, LOGO_H, LOGO_X, C_LOGO_Y, f->opa);
    }
    if (edge < C_WM_X1) {
        /* friso de 2 colunas na borda; o sprite so' tem as linhas com conteudo. */
        blit(layer, &ratimos_boot_wordmark_glow_desc, edge - RATIMOS_BOOT_GLOW_X0, 0, 2,
             (int32_t) ratimos_boot_wordmark_glow_desc.header.h, LOGO_X + edge, C_LOGO_Y + RATIMOS_BOOT_GLOW_Y0,
             f->opa);
    }
}

static bool letreiro_dirty(int32_t before, int32_t after, lv_area_t * out)
{
    int32_t e0 = letreiro_edge(before);
    int32_t e1 = letreiro_edge(after);
    bool done = after >= RATIMOS_SPLASH_UNITS && before < RATIMOS_SPLASH_UNITS;
    if (e0 == e1 && !done) {
        return false;
    }
    int32_t x1 = done ? LOGO_X : LOGO_X + e0;
    int32_t x2 = done ? LOGO_X + LOGO_W - 1 : LOGO_X + e1 + 1;
    lv_area_set(out, x1, C_LOGO_Y, x2, C_LOGO_Y + LOGO_H - 1);
    return true;
}

/* --------------------------------------------------------------------- *
 * TORRES ("fileira de torres") -- BOOTS.torres do 010
 *
 * 6 torrinhas do icone do sistema (2x) enchem de baixo pra cima, tingidas do
 * roxo ao laranja, com uma faixa de brilho na fronteira. A que acabou de
 * encher da um pulinho de 2px: funcao PURA do progresso, sem estado de tempo.
 * --------------------------------------------------------------------- */

#define D_LOGO_Y 110
#define D_N      6
#define D_RH     20 /* linhas de progresso por torre */
#define D_W      28
#define D_H      40
#define D_GAP    7
#define D_X0     58 /* 58.5 no 010, arredondado para baixo */
#define D_Y0     299

static const lv_image_dsc_t * const s_rook_full[D_N] = {
    &ratimos_boot_rook_full_0_desc, &ratimos_boot_rook_full_1_desc, &ratimos_boot_rook_full_2_desc,
    &ratimos_boot_rook_full_3_desc, &ratimos_boot_rook_full_4_desc, &ratimos_boot_rook_full_5_desc,
};
static const lv_image_dsc_t * const s_rook_glint[D_N] = {
    &ratimos_boot_rook_glint_0_desc, &ratimos_boot_rook_glint_1_desc, &ratimos_boot_rook_glint_2_desc,
    &ratimos_boot_rook_glint_3_desc, &ratimos_boot_rook_glint_4_desc, &ratimos_boot_rook_glint_5_desc,
};

static int32_t torres_rows(int32_t units)
{
    return clamp32(units, 0, RATIMOS_SPLASH_UNITS) * (D_N * D_RH) / RATIMOS_SPLASH_UNITS;
}

static void torres_draw(lv_layer_t * layer, const ratimos_boot_frame_t * f)
{
    blit(layer, &ratimos_logo_alpha_desc, 0, 0, LOGO_W, LOGO_H, LOGO_X, D_LOGO_Y, f->opa);

    int32_t rows = torres_rows(f->units);
    for (int32_t i = 0; i < D_N; i++) {
        int32_t fill = clamp32(rows - D_RH * i, 0, D_RH);
        /* pulinho do 010: acabou de encher e a seguinte ainda nao tem 4 linhas */
        int32_t hop = (fill == D_RH && rows - D_RH * (i + 1) < 4 && rows < D_N * D_RH) ? 2 : 0;
        int32_t x = D_X0 + i * (D_W + D_GAP);
        int32_t y = D_Y0 - hop;
        int32_t split = D_H - 2 * fill; /* acima: vazio; daqui pra baixo: cheio */

        if (split > 0) {
            blit(layer, &ratimos_boot_rook_empty_desc, 0, 0, D_W, split, x, y, f->opa);
        }
        if (fill > 0) {
            int32_t band = fill < D_RH ? 2 : 0; /* faixa de 2px de brilho na fronteira */
            if (band) {
                blit(layer, s_rook_glint[i], 0, split, D_W, band, x, y + split, f->opa);
            }
            blit(layer, s_rook_full[i], 0, split + band, D_W, D_H - split - band, x, y + split + band, f->opa);
        }
    }
}

static bool torres_dirty(int32_t before, int32_t after, lv_area_t * out)
{
    if (torres_rows(before) == torres_rows(after)) {
        return false;
    }
    lv_area_set(out, D_X0, D_Y0 - 2, D_X0 + D_N * D_W + (D_N - 1) * D_GAP - 1, D_Y0 + D_H - 1);
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
    const lv_image_dsc_t * thumb;
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
        .thumb = &ratimos_boot_thumb_bandeira_desc,
    },
    [RATIMOS_BOOT_STYLE_AMEIAS] = {
        .name = "ameias do castelo",
        .pct_prefix = "construindo ",
        .text_y = 339, /* y0 + H + 16 */
        .draw = ameias_draw,
        .dirty = ameias_dirty,
        .wave = ameias_wave,
        .thumb = &ratimos_boot_thumb_ameias_desc,
    },
    [RATIMOS_BOOT_STYLE_LETREIRO] = {
        .name = "letreiro que acende",
        .pct_prefix = "carregando ",
        .text_y = C_LOGO_Y + LOGO_H + 22, /* 303 */
        .draw = letreiro_draw,
        .dirty = letreiro_dirty,
        .wave = NULL, /* sem onda: no fim, parado */
        .thumb = &ratimos_boot_thumb_letreiro_desc,
    },
    [RATIMOS_BOOT_STYLE_TORRES] = {
        .name = "fileira de torres",
        .pct_prefix = "carregando ",
        .text_y = D_Y0 + D_H + 14, /* 353 */
        .draw = torres_draw,
        .dirty = torres_dirty,
        .wave = NULL,
        .thumb = &ratimos_boot_thumb_torres_desc,
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

const lv_image_dsc_t * ratimos_boot_style_thumb(ratimos_boot_style_t style)
{
    return resolve(style)->thumb;
}
