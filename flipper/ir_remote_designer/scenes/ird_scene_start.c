#include "../ird_app_i.h"

enum {
    IrdStartIndexNew = 0,
    IrdStartIndexAbout = 1,
    IrdStartIndexRemoteBase = 16,
};

static void ird_scene_start_submenu_callback(void* context, uint32_t index) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void ird_scene_start_on_enter(void* context) {
    IrdApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "IR Remote Designer");
    submenu_add_item(
        submenu, "+ Neue Fernbedienung", IrdStartIndexNew, ird_scene_start_submenu_callback, app);

    ird_app_scan_remotes(app);
    for(uint8_t i = 0; i < app->remote_count; i++) {
        submenu_add_item(
            submenu,
            app->remote_names[i],
            IrdStartIndexRemoteBase + i,
            ird_scene_start_submenu_callback,
            app);
    }

    submenu_add_item(submenu, "Hilfe / Info", IrdStartIndexAbout, ird_scene_start_submenu_callback, app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, IrdSceneStart));
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewSubmenu);
}

bool ird_scene_start_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        const uint32_t index = event.event;
        scene_manager_set_scene_state(app->scene_manager, IrdSceneStart, index);
        if(index == IrdStartIndexNew) {
            scene_manager_next_scene(app->scene_manager, IrdSceneCategory);
            consumed = true;
        } else if(index == IrdStartIndexAbout) {
            scene_manager_next_scene(app->scene_manager, IrdSceneAbout);
            consumed = true;
        } else if(index >= IrdStartIndexRemoteBase) {
            uint32_t i = index - IrdStartIndexRemoteBase;
            if(i < app->remote_count) {
                ird_remote_path_for_name(app->file_path, app->remote_names[i]);
                if(ird_remote_load(app->remote, app->storage, furi_string_get_cstr(app->file_path))) {
                    app->current_button = 0;
                    ird_remote_view_set_remote(app->remote_view, app->remote);
                    scene_manager_next_scene(app->scene_manager, IrdSceneRemote);
                } else {
                    ird_app_show_message(
                        app, "Fehler", "Datei konnte nicht\ngeladen werden", IrdSceneStart);
                }
            }
            consumed = true;
        }
    }
    return consumed;
}

void ird_scene_start_on_exit(void* context) {
    IrdApp* app = context;
    submenu_reset(app->submenu);
}
