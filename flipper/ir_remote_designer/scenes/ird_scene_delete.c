#include "../ird_app_i.h"

static void ird_scene_delete_dialog_callback(DialogExResult result, void* context) {
    IrdApp* app = context;
    if(result == DialogExResultLeft) {
        view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventDialogLeft);
    } else if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventDialogRight);
    }
}

void ird_scene_delete_on_enter(void* context) {
    IrdApp* app = context;
    DialogEx* dialog = app->dialog_ex;
    dialog_ex_reset(dialog);

    if(app->delete_mode == IrdDeleteModeButton) {
        dialog_ex_set_header(dialog, "Knopf loeschen?", 64, 4, AlignCenter, AlignTop);
        furi_string_set(app->tmp_str, app->remote->buttons[app->current_button].label);
    } else {
        dialog_ex_set_header(dialog, "Fernbedienung loeschen?", 64, 4, AlignCenter, AlignTop);
        furi_string_printf(app->tmp_str, "%s\n(alle Zuweisungen gehen verloren)", app->remote->name);
    }
    dialog_ex_set_text(dialog, furi_string_get_cstr(app->tmp_str), 64, 24, AlignCenter, AlignTop);
    dialog_ex_set_left_button_text(dialog, "Abbrechen");
    dialog_ex_set_right_button_text(dialog, "Loeschen");
    dialog_ex_set_context(dialog, app);
    dialog_ex_set_result_callback(dialog, ird_scene_delete_dialog_callback);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewDialogEx);
}

bool ird_scene_delete_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    SceneManager* sm = app->scene_manager;

    if(event.type == SceneManagerEventTypeBack) {
        scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        return true;
    }
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == IrdCustomEventDialogLeft) {
        scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        return true;
    }
    if(event.event == IrdCustomEventDialogRight) {
        if(app->delete_mode == IrdDeleteModeButton) {
            ird_remote_remove_button(app->remote, app->current_button);
            if(app->current_button >= app->remote->button_count && app->current_button > 0) {
                app->current_button = app->remote->button_count ? app->remote->button_count - 1 : 0;
            }
            ird_app_save_remote(app, NULL, -1);
            ird_remote_view_set_focus(app->remote_view, app->current_button);
            scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        } else {
            if(ird_remote_delete_file(app->remote, app->storage)) {
                ird_remote_view_set_remote(app->remote_view, NULL);
                scene_manager_set_scene_state(sm, IrdSceneStart, 0);
                scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneStart);
            } else {
                ird_app_show_message(app, "Fehler", "Datei konnte nicht\ngeloescht werden", IrdSceneRemote);
            }
        }
        return true;
    }
    return false;
}

void ird_scene_delete_on_exit(void* context) {
    IrdApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
