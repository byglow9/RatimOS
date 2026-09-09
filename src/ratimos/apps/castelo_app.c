/*
 * Castelo (PROGRESSAO-01) -- tela dedicada da progressao castelo/jardim.
 *
 * Cache-once igual a jogos_app.c/conexo.c: a tela e construida UMA vez e
 * guardada em s_castelo_screen. Como a cena muda conforme o contador
 * cresce, toda visita chama refresh_castelo_screen() para ATUALIZAR os
 * objetos ja existentes (imagem de estagio, legenda, faixa de conquistas)
 * em vez de reconstruir a tela -- reconstruir a cada visita e exatamente o
 * vazamento de heap do LVGL que a Fase 1 ja corrigiu, nao pode voltar.
 *
 * So le progressao atraves de ratimos_storage_get_progression() -- nunca
 * abre arquivo diretamente (mesma disciplina de todo src/ratimos/apps/).
 */
#include <string.h>

#include "castelo_app.h"

#include "../app_shell.h"
#include "../theme.h"
#include "../progression.h"
#include "../fonts/ratimos_fonts.h"
#include "../../storage/content_api.h"

#define UNLOCK_ICON_SIZE 32

static lv_obj_t * s_castelo_screen = NULL;
static lv_obj_t * s_stage_image = NULL;
static lv_obj_t * s_caption_label = NULL;
static lv_obj_t * s_unlock_icons[RATIMOS_GAME_COUNT];

/*
 * Copia de status por estagio (UI-SPEC "Progression System UI" +
 * Copywriting Contract): sempre celebratoria, nunca de risco/perda -- este
 * modulo nunca menciona um streak em risco, uma cobranca por ausencia nem
 * uma contagem regressiva. O premio existe pra encantar quem recebe o
 * presente, nao pra reter a jogadora.
 */
static void set_caption(uint16_t shared_completions)
{
    if (ratimos_progression_is_complete(shared_completions)) {
        lv_obj_set_style_text_font(s_caption_label, &ratimos_font_title_20, 0);
        lv_label_set_text(s_caption_label, "castelo completo! parabens");
        return;
    }

    lv_obj_set_style_text_font(s_caption_label, &ratimos_font_title_16, 0);

    if (shared_completions == 0) {
        lv_label_set_text(s_caption_label, "o castelo esta apenas comecando");
        return;
    }

    lv_label_set_text_fmt(s_caption_label, "dia %d de %d",
                          (int) shared_completions,
                          (int) ratimos_progression_target_completions());
}

static void refresh_castelo_screen(void)
{
    ratimos_progression_state_t state;
    if (!ratimos_storage_get_progression(&state)) {
        /* Sem save valido ainda: trata como "zero completions" -- o mesmo
         * estado que uma jogadora nova ve, nunca um erro visivel aqui (o
         * jogo que grava a vitoria e' quem mostra aviso de save invalido,
         * nao esta tela so-leitura). */
        memset(&state, 0, sizeof(state));
    }

    const lv_image_dsc_t * stage_img =
        ratimos_progress_image_by_id(ratimos_progression_stage_asset_id(state.shared_completions));
    if (stage_img) {
        lv_image_set_src(s_stage_image, stage_img);
        lv_obj_clear_flag(s_stage_image, LV_OBJ_FLAG_HIDDEN);
    } else {
        /* NULL == "nao renderiza nada" (contrato de ratimos_progress_image_by_id) --
         * nunca desreferencia, so esconde o widget de imagem. */
        lv_obj_add_flag(s_stage_image, LV_OBJ_FLAG_HIDDEN);
    }

    set_caption(state.shared_completions);

    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        const lv_image_dsc_t * unlock_img =
            ratimos_progress_image_by_id(ratimos_progression_unlock_asset_id((ratimos_game_kind_t) i));
        if (unlock_img) {
            lv_image_set_src(s_unlock_icons[i], unlock_img);
        }

        /* Desbloqueado: opacidade total. Bloqueado: renderiza ATENUADO, nao
         * escondido -- a jogadora precisa ver o que ainda falta conquistar
         * (must-have truth do plano). */
        lv_opa_t opa = state.game_exclusive_unlocked[i] ? LV_OPA_COVER : LV_OPA_40;
        lv_obj_set_style_image_opa(s_unlock_icons[i], opa, 0);
    }
}

static lv_obj_t * build_castelo_screen(void)
{
    ratimos_app_shell_t shell = ratimos_app_shell_create("castelo", "seu jardim");

    s_stage_image = lv_image_create(shell.content);
    lv_obj_set_width(s_stage_image, lv_pct(100));

    s_caption_label = lv_label_create(shell.content);
    lv_label_set_text(s_caption_label, "");
    lv_obj_set_width(s_caption_label, lv_pct(100));
    lv_label_set_long_mode(s_caption_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(s_caption_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_caption_label, RATIMOS_COLOR_TEXT, 0);

    lv_obj_t * conquistas_label = lv_label_create(shell.content);
    lv_label_set_text(conquistas_label, "conquistas");
    lv_obj_set_style_text_color(conquistas_label, RATIMOS_COLOR_TEXT_MUTED, 0);

    lv_obj_t * unlock_row = lv_obj_create(shell.content);
    lv_obj_remove_style_all(unlock_row);
    lv_obj_set_width(unlock_row, lv_pct(100));
    lv_obj_set_height(unlock_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(unlock_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(unlock_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(unlock_row, LV_OBJ_FLAG_SCROLLABLE);

    for (size_t i = 0; i < RATIMOS_GAME_COUNT; i++) {
        s_unlock_icons[i] = lv_image_create(unlock_row);
        lv_obj_set_size(s_unlock_icons[i], UNLOCK_ICON_SIZE, UNLOCK_ICON_SIZE);
    }

    refresh_castelo_screen();

    return shell.screen;
}

void ratimos_castelo_show(lv_event_t * e)
{
    (void) e;
    if (!s_castelo_screen) {
        s_castelo_screen = build_castelo_screen();
    } else {
        refresh_castelo_screen();
    }
    lv_screen_load(s_castelo_screen);
}
