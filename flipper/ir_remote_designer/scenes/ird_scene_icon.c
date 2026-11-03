#include "../ird_app_i.h"

static void ird_scene_icon_callback(void* context, uint32_t index) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void ird_scene_icon_on_enter(void* context) {
    IrdApp* app = context;
    Submenu* submenu = app->submenu;
    submenu_reset(submenu);
    submenu_set_header(submenu, "Symbol waehlen");
    for(uint8_t i = 0; i < IrdIconCount; i++) {
        submenu_add_item(submenu, ird_icon_name(i), i, ird_scene_icon_callback, app);
    }
    submenu_set_selected_item(submenu, app->remote->buttons[app->current_button].icon);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewSubmenu);
}

bool ird_scene_icon_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event >= IrdIconCount) return false;
    app->remote->buttons[app->current_button].icon = event.event;
    ird_app_save_remote(app, NULL, -1);
    scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneRemote);
    return true;
}

void ird_scene_icon_on_exit(void* context) {
    IrdApp* app = context;
    submenu_reset(app->submenu);
}
