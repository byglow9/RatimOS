#include "theme_catalog.h"
#include "bg_images.h"
#include "theme.h"

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
        .home_thumb = NULL,
        .boot_bg_thumb = NULL,
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

static void blit_full(lv_layer_t * layer, const lv_image_dsc_t * dsc, const lv_area_t * area)
{
    lv_draw_image_dsc_t d;
    lv_draw_image_dsc_init(&d);
    d.src = dsc;
    d.opa = LV_OPA_COVER;
    lv_area_t a = { area->x1, area->y1, area->x1 + (int32_t) dsc->header.w - 1,
                    area->y1 + (int32_t) dsc->header.h - 1 };
    lv_draw_image(layer, &d, &a);
}

void ratimos_theme_boot_bg_draw(const ratimos_theme_t * theme, lv_layer_t * layer,
                                const lv_area_t * area, uint32_t now_ms)
{
    (void) now_ms;
    if (theme == NULL || layer == NULL || area == NULL) {
        return;
    }

    switch (theme->boot_bg.source) {
    case RATIMOS_BOOT_BG_DITHER:
        blit_full(layer, &ratimos_bg_dither_desc, area);
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
    (void) theme; /* Task 3 acrescenta as fontes animadas */
    return false;
}

void ratimos_theme_boot_bg_tick(const ratimos_theme_t * theme, uint32_t prev_ms, uint32_t now_ms,
                                lv_obj_t * stage)
{
    (void) theme;
    (void) prev_ms;
    (void) now_ms;
    (void) stage; /* fontes estaticas: nada a invalidar */
}
