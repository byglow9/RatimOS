#include "app_shell.h"
#include "theme.h"
#include "status_bar.h"
#include "home_screen.h"

ratimos_app_shell_t ratimos_app_shell_create(const char * section_label,
                                              const char * bottom_right_hint)
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
    lv_obj_set_style_pad_all(shell.content, 10, 0);
    lv_obj_set_flex_flow(shell.content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(shell.content, 8, 0);

    ratimos_bottombar_create(shell.screen, "voltar", ratimos_home_screen_show, bottom_right_hint);

    return shell;
}
