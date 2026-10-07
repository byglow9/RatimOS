#include "app_shell.h"
#include "theme.h"
#include "status_bar.h"
#include "home_screen.h"

/* LV_OBJ_FLAG_USER_1 = "tela persistente" (nenhum outro uso de flags de
 * usuario no projeto). */
#define RATIMOS_SCREEN_FLAG_PERSISTENT LV_OBJ_FLAG_USER_1

void ratimos_screen_set_persistent(lv_obj_t * scr)
{
    lv_obj_add_flag(scr, RATIMOS_SCREEN_FLAG_PERSISTENT);
}

void ratimos_screen_load(lv_obj_t * scr)
{
    lv_obj_t * old = lv_screen_active();
    if (old == scr) {
        return;
    }
    bool delete_old = old != NULL && !lv_obj_has_flag(old, RATIMOS_SCREEN_FLAG_PERSISTENT);
    lv_screen_load_anim(scr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, delete_old);
}

ratimos_app_shell_t ratimos_app_shell_create(const char * section_label,
                                              const char * bottom_right_hint)
{
    return ratimos_app_shell_create_with_back(section_label, bottom_right_hint, ratimos_home_screen_show);
}

ratimos_app_shell_t ratimos_app_shell_create_with_back(const char * section_label,
                                                        const char * bottom_right_hint,
                                                        lv_event_cb_t back_cb)
{
    ratimos_app_shell_t shell;

    shell.screen = lv_obj_create(NULL);
    ratimos_theme_apply_screen(shell.screen);
    lv_obj_set_flex_flow(shell.screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(shell.screen, LV_OBJ_FLAG_SCROLLABLE);

    ratimos_topbar_create(shell.screen);
    ratimos_sectionbar_create(shell.screen, section_label);

    shell.content = lv_obj_create(shell.screen);
    lv_obj_remove_style_all(shell.content);
    lv_obj_set_width(shell.content, RATIMOS_SCREEN_W);
    lv_obj_set_flex_grow(shell.content, 1);
    lv_obj_set_style_pad_all(shell.content, RATIMOS_CONTENT_PAD, 0);
    lv_obj_set_flex_flow(shell.content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(shell.content, RATIMOS_CONTENT_GAP, 0);

    ratimos_bottombar_create(shell.screen, "voltar", back_cb ? back_cb : ratimos_home_screen_show,
                             bottom_right_hint);

    return shell;
}
