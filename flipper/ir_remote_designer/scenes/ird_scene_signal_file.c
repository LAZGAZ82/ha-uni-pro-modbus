#include "../ird_app_i.h"
#include "ir_remote_designer_icons.h"

void ird_scene_signal_file_on_enter(void* context) {
    IrdApp* app = context;

    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, IRD_IR_EXT, &I_ir_10px);
    options.base_path = IRD_IR_LIBRARY_DIR;
    options.skip_assets = false; /* auch die Universal-Bibliothek anzeigen */
    options.hide_ext = true;

    if(furi_string_empty(app->file_path) ||
       !furi_string_start_with(app->file_path, IRD_IR_LIBRARY_DIR)) {
        furi_string_set(app->file_path, IRD_IR_LIBRARY_DIR);
    }

    const bool selected =
        dialog_file_browser_show(app->dialogs, app->file_path, app->file_path, &options);

    if(selected) {
        scene_manager_next_scene(app->scene_manager, IrdSceneSignalPick);
    } else {
        scene_manager_previous_scene(app->scene_manager);
    }
}

bool ird_scene_signal_file_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ird_scene_signal_file_on_exit(void* context) {
    UNUSED(context);
}
