#include "../ird_app_i.h"

static void ird_scene_category_submenu_callback(void* context, uint32_t index) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void ird_scene_category_on_enter(void* context) {
    IrdApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Geraetetyp waehlen");
    for(uint8_t i = 0; i < IrdCategoryCount; i++) {
        submenu_add_item(submenu, ird_category_name(i), i, ird_scene_category_submenu_callback, app);
    }
    submenu_set_selected_item(submenu, app->new_category);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewSubmenu);
}

bool ird_scene_category_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event < IrdCategoryCount) {
        app->new_category = event.event;
        app->name_mode = IrdNameModeNewRemote;
        scene_manager_next_scene(app->scene_manager, IrdSceneName);
        return true;
    }
    return false;
}

void ird_scene_category_on_exit(void* context) {
    IrdApp* app = context;
    submenu_reset(app->submenu);
}
