#include "theme_catalog.h"
#include "bg_images.h"
#include "theme.h"
#include "theme_art.h"

#include <math.h>
#include "src/misc/lv_area_private.h" /* lv_area_intersect (LVGL 9.5 o mantem nos headers privados) */
#include "src/misc/cache/instance/lv_image_cache.h" /* lv_image_cache_drop */

/*
 * Catalogo de temas (plano 02.1-16). Indexado pelo id (valores FIXOS de
 * ratimos_theme_id_t). Uma linha sem preencher (.name == NULL) e' tratada
 * como inexistente por ratimos_theme_get(), que cai no Classico.
 */
static const ratimos_theme_t s_builtin_themes[RATIMOS_THEME_COUNT] = {
    [RATIMOS_THEME_CLASSICO] = {
        .id = RATIMOS_THEME_CLASSICO,
        .name = "clássico",
        .boot_bg = { RATIMOS_BOOT_BG_DITHER, 55 },
        .system_bg = { &ratimos_bg_dither_desc, 0 },
        .home_thumb = &ratimos_thumb_home_classico_desc,
        .boot_bg_thumb = &ratimos_thumb_bootbg_classico_desc,
    },
    [RATIMOS_THEME_NOITE] = {
        .id = RATIMOS_THEME_NOITE,
        .name = "noite estrelada",
        .boot_bg = { RATIMOS_BOOT_BG_STARS, 0 },
        .system_bg = { &ratimos_home_bg_noite_desc, 0 },
        .home_thumb = &ratimos_thumb_home_noite_desc,
        .boot_bg_thumb = &ratimos_thumb_bootbg_noite_desc,
    },
    [RATIMOS_THEME_CASTELO] = {
        .id = RATIMOS_THEME_CASTELO,
        .name = "castelo ao entardecer",
        .boot_bg = { RATIMOS_BOOT_BG_SUNSET, 15 },
        .system_bg = { &ratimos_home_bg_castelo_desc, 20 },
        .home_thumb = &ratimos_thumb_home_castelo_desc,
        .boot_bg_thumb = &ratimos_thumb_bootbg_castelo_desc,
    },
    [RATIMOS_THEME_XADREZ] = {
        .id = RATIMOS_THEME_XADREZ,
        .name = "xadrez",
        .boot_bg = { RATIMOS_BOOT_BG_CHESS, 10 },
        .system_bg = { &ratimos_home_bg_xadrez_desc, 0 },
        .home_thumb = &ratimos_thumb_home_xadrez_desc,
        .boot_bg_thumb = &ratimos_thumb_bootbg_xadrez_desc,
    },
};

static const ratimos_theme_t * s_themes = s_builtin_themes;
static size_t s_theme_count = RATIMOS_THEME_COUNT;

static bool s_applied_loaded = false;
static ratimos_theme_id_t s_applied_id = RATIMOS_THEME_DEFAULT;
static bool s_previewing = false;
static ratimos_theme_id_t s_preview_id = RATIMOS_THEME_DEFAULT;
static uint32_t s_generation = 1;

void ratimos_theme_catalog_override_for_test(const ratimos_theme_t * themes, size_t count)
{
    if (themes == NULL) {
        s_themes = s_builtin_themes;
        s_theme_count = RATIMOS_THEME_COUNT;
    } else {
        s_themes = themes;
        s_theme_count = count;
    }
}

const ratimos_theme_t * ratimos_theme_get(ratimos_theme_id_t id)
{
    if ((int) id >= 0 && (size_t) id < s_theme_count && s_themes[id].name != NULL) {
        return &s_themes[id];
    }
    /* Defensivo permanente: o Classico sempre existe no catalogo embutido; no
     * catalogo de teste, a linha 0 e' o fallback. */
    return &s_themes[RATIMOS_THEME_CLASSICO];
}

static void load_applied_once(void)
{
    if (!s_applied_loaded) {
        s_applied_id = ratimos_storage_get_theme_id();
        s_applied_loaded = true;
    }
}

const ratimos_theme_t * ratimos_theme_applied(void)
{
    load_applied_once();
    return ratimos_theme_get(s_applied_id);
}

const ratimos_theme_t * ratimos_theme_displayed(void)
{
    if (s_previewing) {
        return ratimos_theme_get(s_preview_id);
    }
    return ratimos_theme_applied();
}

void ratimos_theme_apply(ratimos_theme_id_t id)
{
    if ((int) id < 0 || (int) id >= (int) RATIMOS_THEME_COUNT) {
        return; /* nunca aplica um id invalido */
    }
    s_applied_id = id;
    s_applied_loaded = true;
    s_generation++;
}

void ratimos_theme_preview_begin(ratimos_theme_id_t id)
{
    if ((int) id < 0 || (int) id >= (int) RATIMOS_THEME_COUNT) {
        return;
    }
    load_applied_once();
    s_previewing = true;
    s_preview_id = id;
    s_generation++;
}

void ratimos_theme_preview_end(void)
{
    if (s_previewing) {
        s_previewing = false;
        s_generation++;
    }
}

uint32_t ratimos_theme_generation(void)
{
    return s_generation;
}

/* ---------------------------------------------------------------------- *
 * Fundo do boot
 *
 * Nenhuma fonte cria objeto LVGL nem aloca heap LVGL por quadro: tudo e'
 * desenhado no stage da splash durante o draw (retangulos e blits de imagens
 * em flash) e o tick so' INVALIDA a area que mudou. O stage esta em (0,0), entao
 * coordenadas de tela == coordenadas do stage.
 * ---------------------------------------------------------------------- */

static void fill_black(lv_layer_t * layer, const lv_area_t * area, lv_opa_t opa)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = lv_color_black();
    d.bg_opa = opa;
    d.border_width = 0;
    lv_draw_rect(layer, &d, area);
}

/* O retangulo toca o clip atual? (evita criar draw task a toa: sao ~100
 * retangulos de estrela por quadro) */
static bool touches_clip(const lv_layer_t * layer, const lv_area_t * a)
{
    lv_area_t tmp;
    return lv_area_intersect(&tmp, a, &layer->_clip_area);
}

static void rect_px(lv_layer_t * layer, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t hex)
{
    lv_area_t a = { x, y, x + w - 1, y + h - 1 };
    if (!touches_clip(layer, &a)) {
        return;
    }
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = 0;
    d.bg_color = lv_color_hex(hex);
    d.bg_opa = LV_OPA_COVER;
    d.border_width = 0;
    lv_draw_rect(layer, &d, &a);
}

static void blit_at(lv_layer_t * layer, const lv_image_dsc_t * dsc, int32_t x, int32_t y)
{
    lv_area_t a = { x, y, x + (int32_t) dsc->header.w - 1, y + (int32_t) dsc->header.h - 1 };
    if (!touches_clip(layer, &a)) {
        return;
    }
    lv_draw_image_dsc_t d;
    lv_draw_image_dsc_init(&d);
    d.src = dsc;
    d.opa = LV_OPA_COVER;
    lv_draw_image(layer, &d, &a);
}

static void invalidate_rect(lv_obj_t * stage, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    lv_area_t a = { x1, y1, x2, y2 };
    lv_obj_invalidate_area(stage, &a);
}

/* ---- NOITE ESTRELADA ---------------------------------------------------- */

/* Fase da estrela i no instante now_ms: floor(now/period + i) % 4 (010). */
static uint32_t star_phase(int i, uint32_t now_ms)
{
    const ratimos_star_t * s = &ratimos_noite_stars[i];
    return (((uint32_t) ((float) now_ms / (float) s->period_ms)) + (uint32_t) i) % 4u;
}

static void stars_draw(lv_layer_t * layer, uint32_t now_ms)
{
    rect_px(layer, 0, 0, 320, 160, RATIMOS_NOITE_BAND0);
    rect_px(layer, 0, 160, 320, 160, RATIMOS_NOITE_BAND1);
    rect_px(layer, 0, 320, 320, 160, RATIMOS_NOITE_BAND2);

    for (int i = 0; i < RATIMOS_NOITE_STAR_COUNT; i++) {
        const ratimos_star_t * s = &ratimos_noite_stars[i];
        uint32_t ph = star_phase(i, now_ms);
        if (ph == 0) {
            continue;
        }
        rect_px(layer, s->x, s->y, 1, 1, ph == 3 ? RATIMOS_NOITE_STAR_PEAK : s->col);
        if (s->big && ph >= 2) {
            rect_px(layer, s->x - 1, s->y, 1, 1, RATIMOS_NOITE_BIG_CROSS);
            rect_px(layer, s->x + 1, s->y, 1, 1, RATIMOS_NOITE_BIG_CROSS);
            rect_px(layer, s->x, s->y - 1, 1, 1, RATIMOS_NOITE_BIG_CROSS);
            rect_px(layer, s->x, s->y + 1, 1, 1, RATIMOS_NOITE_BIG_CROSS);
        }
    }
    blit_at(layer, &ratimos_noite_moon_desc, RATIMOS_NOITE_MOON_X, RATIMOS_NOITE_MOON_Y);
}

static void stars_tick(uint32_t prev_ms, uint32_t now_ms, lv_obj_t * stage)
{
    for (int i = 0; i < RATIMOS_NOITE_STAR_COUNT; i++) {
        if (star_phase(i, prev_ms) != star_phase(i, now_ms)) {
            const ratimos_star_t * s = &ratimos_noite_stars[i];
            invalidate_rect(stage, s->x - 1, s->y - 1, s->x + 1, s->y + 1);
        }
    }
}

/* ---- CASTELO AO ENTARDECER ---------------------------------------------- */

static bool window_is_off(const ratimos_window_t * w, uint32_t now_ms)
{
    return ((now_ms / 800u) + w->i) % 5u == 0u;
}

static void sunset_draw(lv_layer_t * layer, uint32_t now_ms)
{
    blit_at(layer, &ratimos_boot_bg_castelo_desc, 0, 0); /* janelas ja pintadas ACESAS */
    for (int i = 0; i < RATIMOS_CASTELO_WINDOW_COUNT; i++) {
        const ratimos_window_t * w = &ratimos_castelo_windows[i];
        if (window_is_off(w, now_ms)) {
            rect_px(layer, w->x, w->y, RATIMOS_CASTELO_WIN_W, RATIMOS_CASTELO_WIN_H, RATIMOS_CASTELO_WIN_OFF);
            rect_px(layer, w->x, w->y, RATIMOS_CASTELO_WIN_W, 1, RATIMOS_CASTELO_WIN_TOP);
        }
    }
}

static void sunset_tick(uint32_t prev_ms, uint32_t now_ms, lv_obj_t * stage)
{
    if (prev_ms / 800u == now_ms / 800u) {
        return;
    }
    for (int i = 0; i < RATIMOS_CASTELO_WINDOW_COUNT; i++) {
        const ratimos_window_t * w = &ratimos_castelo_windows[i];
        invalidate_rect(stage, w->x, w->y, w->x + RATIMOS_CASTELO_WIN_W - 1, w->y + RATIMOS_CASTELO_WIN_H - 1);
    }
}

/* ---- XADREZ -------------------------------------------------------------- */
/*
 * Tabuleiro em perspectiva por pixel (chessRender do 010) num buffer RGB565
 * 320x480 ESTATICO: vive no BSS, fora do heap do LVGL. No dispositivo este
 * buffer (307200 bytes) vai para a PSRAM (EXT_RAM_BSS_ATTR) na fase de
 * hardware; no simulador e' memoria comum. O ceu (y < 140) e' renderizado uma
 * vez; a cada >= 66ms so' as linhas y >= 141 sao refeitas, com z, fog e as
 * duas cores de cada linha pre-calculados.
 */
#define CHESS_W  320
#define CHESS_H  480
#define CHESS_HY 140
#define CHESS_MIN_RENDER_MS 66

static uint16_t s_chess_buf[CHESS_W * CHESS_H];
static lv_image_dsc_t s_chess_dsc = {
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = CHESS_W,
    .header.h = CHESS_H,
    .header.stride = CHESS_W * 2,
    .data_size = sizeof(s_chess_buf),
    .data = (const uint8_t *) s_chess_buf,
};

static bool s_chess_ready;
static uint32_t s_chess_last_ms;
static float s_row_z[CHESS_H];        /* 65000 / dy */
static float s_row_u24[CHESS_H];      /* z / 24: u/24 = colfac * z / 24 */
static uint16_t s_row_a[CHESS_H];     /* cor da casa A com a nevoa da linha */
static uint16_t s_row_b[CHESS_H];     /* cor da casa B */
static float s_colfac[CHESS_W];       /* (x - 160) / 160 */

static uint16_t rgb565_of(int r, int g, int b)
{
    return (uint16_t) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static int clamp255(float v)
{
    int r = (int) lrintf(v);
    return r < 0 ? 0 : (r > 255 ? 255 : r);
}

static void chess_init_tables_and_sky(void)
{
    static const int A[3] = RATIMOS_XADREZ_A, B[3] = RATIMOS_XADREZ_B, M[3] = RATIMOS_XADREZ_M;
    static const int SKY0[3] = RATIMOS_XADREZ_SKY0, SKY1[3] = RATIMOS_XADREZ_SKY1;

    for (int x = 0; x < CHESS_W; x++) {
        s_colfac[x] = (float) (x - 160) / 160.0f;
    }
    for (int y = 0; y < CHESS_H; y++) {
        if (y < CHESS_HY) {
            float t = (float) (y - (CHESS_HY - 60)) / 60.0f;
            t = t < 0.0f ? 0.0f : t;
            uint16_t c = rgb565_of(clamp255(SKY0[0] + (SKY1[0] - SKY0[0]) * t * t),
                                   clamp255(SKY0[1] + (SKY1[1] - SKY0[1]) * t * t),
                                   clamp255(SKY0[2] + (SKY1[2] - SKY0[2]) * t * t));
            for (int x = 0; x < CHESS_W; x++) {
                s_chess_buf[y * CHESS_W + x] = c;
            }
            continue;
        }
        float dy = (float) (y - CHESS_HY) + 0.5f;
        float z = 65000.0f / dy;
        float fog = (dy - 4.0f) / 70.0f;
        fog = fog < 0.0f ? 0.0f : (fog > 1.0f ? 1.0f : fog);
        s_row_z[y] = z;
        s_row_u24[y] = z / 24.0f;
        s_row_a[y] = rgb565_of(clamp255(M[0] + (A[0] - M[0]) * fog), clamp255(M[1] + (A[1] - M[1]) * fog),
                               clamp255(M[2] + (A[2] - M[2]) * fog));
        s_row_b[y] = rgb565_of(clamp255(M[0] + (B[0] - M[0]) * fog), clamp255(M[1] + (B[1] - M[1]) * fog),
                               clamp255(M[2] + (B[2] - M[2]) * fog));
    }
}

/* Refaz as linhas [y0, CHESS_H) do tabuleiro para a rolagem do instante. */
static void chess_render_rows(int y0, uint32_t now_ms)
{
    float scroll = (float) now_ms / 100.0f; /* (now/1000)*10 px */
    for (int y = y0; y < CHESS_H; y++) {
        int vrow = (int) floorf((s_row_z[y] + scroll) / 24.0f);
        float uz = s_row_u24[y];
        uint16_t a = s_row_a[y], b = s_row_b[y];
        uint16_t * row = &s_chess_buf[y * CHESS_W];
        for (int x = 0; x < CHESS_W; x++) {
            int uc = (int) floorf(s_colfac[x] * uz);
            row[x] = ((uc + vrow) & 1) ? a : b;
        }
    }
}

static void chess_ensure_init(uint32_t now_ms)
{
    if (!s_chess_ready) {
        chess_init_tables_and_sky();
        chess_render_rows(CHESS_HY, now_ms);
        s_chess_last_ms = now_ms;
        s_chess_ready = true;
    }
}

static void chess_draw(lv_layer_t * layer, uint32_t now_ms)
{
    chess_ensure_init(now_ms);
    blit_at(layer, &s_chess_dsc, 0, 0);
    rect_px(layer, 0, CHESS_HY, 320, 1, RATIMOS_XADREZ_LINE);
}

static void chess_tick(uint32_t now_ms, lv_obj_t * stage)
{
    chess_ensure_init(now_ms);
    if (now_ms - s_chess_last_ms < CHESS_MIN_RENDER_MS) {
        return;
    }
    s_chess_last_ms = now_ms;
    chess_render_rows(CHESS_HY + 1, now_ms);
    lv_image_cache_drop(&s_chess_dsc); /* o conteudo do buffer mudou */
    invalidate_rect(stage, 0, CHESS_HY + 1, CHESS_W - 1, CHESS_H - 1);
}

/* ---- despacho -------------------------------------------------------------- */

void ratimos_theme_boot_bg_draw(const ratimos_theme_t * theme, lv_layer_t * layer,
                                const lv_area_t * area, uint32_t now_ms)
{
    if (theme == NULL || layer == NULL || area == NULL) {
        return;
    }

    switch (theme->boot_bg.source) {
    case RATIMOS_BOOT_BG_DITHER:
        blit_at(layer, &ratimos_bg_dither_desc, area->x1, area->y1);
        break;
    case RATIMOS_BOOT_BG_STARS:
        stars_draw(layer, now_ms);
        break;
    case RATIMOS_BOOT_BG_SUNSET:
        sunset_draw(layer, now_ms);
        break;
    case RATIMOS_BOOT_BG_CHESS:
        chess_draw(layer, now_ms);
        break;
    case RATIMOS_BOOT_BG_BLACK:
    default: /* fonte desconhecida cai no preto: nunca um fundo indefinido */
        fill_black(layer, area, LV_OPA_COVER);
        return; /* preto sobre preto: o escurecimento nao muda nada */
    }

    if (theme->boot_bg.dim_pct > 0) {
        /* rgba(0,0,0,dim) do sketch 010, por cima do fundo e por baixo do
         * carregamento. */
        fill_black(layer, area, (lv_opa_t) (theme->boot_bg.dim_pct * 255 / 100));
    }
}

bool ratimos_theme_boot_bg_is_animated(const ratimos_theme_t * theme)
{
    if (theme == NULL) {
        return false;
    }
    switch (theme->boot_bg.source) {
    case RATIMOS_BOOT_BG_STARS:
    case RATIMOS_BOOT_BG_SUNSET:
    case RATIMOS_BOOT_BG_CHESS:
        return true;
    default:
        return false;
    }
}

void ratimos_theme_boot_bg_tick(const ratimos_theme_t * theme, uint32_t prev_ms, uint32_t now_ms,
                                lv_obj_t * stage)
{
    if (theme == NULL || stage == NULL) {
        return;
    }
    switch (theme->boot_bg.source) {
    case RATIMOS_BOOT_BG_STARS:
        stars_tick(prev_ms, now_ms, stage);
        break;
    case RATIMOS_BOOT_BG_SUNSET:
        sunset_tick(prev_ms, now_ms, stage);
        break;
    case RATIMOS_BOOT_BG_CHESS:
        chess_tick(now_ms, stage);
        break;
    default: /* fontes estaticas: nada a invalidar */
        break;
    }
}
