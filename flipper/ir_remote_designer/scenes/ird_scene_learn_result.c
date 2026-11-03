#include "../ird_app_i.h"
#include <string.h>

static void ird_scene_learn_result_dialog_callback(DialogExResult result, void* context) {
    IrdApp* app = context;
    if(result == DialogExResultLeft) {
        view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventDialogLeft);
    } else if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventDialogRight);
    }
}

void ird_scene_learn_result_on_enter(void* context) {
    IrdApp* app = context;
    DialogEx* dialog = app->dialog_ex;

    dialog_ex_reset(dialog);
    ird_signal_describe(app->pending_signal, app->tmp_str);
    dialog_ex_set_header(dialog, "Signal empfangen", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(dialog, furi_string_get_cstr(app->tmp_str), 64, 26, AlignCenter, AlignTop);
    dialog_ex_set_left_button_text(dialog, "Nochmal");
    dialog_ex_set_right_button_text(dialog, "Speichern");
    dialog_ex_set_context(dialog, app);
    dialog_ex_set_result_callback(dialog, ird_scene_learn_result_dialog_callback);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewDialogEx);
}

bool ird_scene_learn_result_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    SceneManager* sm = app->scene_manager;

    if(event.type == SceneManagerEventTypeBack) {
        scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        return true;
    }
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == IrdCustomEventDialogLeft) {
        scene_manager_previous_scene(sm); /* erneut anlernen */
        return true;
    }
    if(event.event == IrdCustomEventDialogRight) {
        IrdButton* b = &app->remote->buttons[app->current_button];
        strlcpy(b->source, "Angelernt", sizeof(b->source));
        b->has_signal = true;
        if(ird_app_save_remote(app, app->pending_signal, app->current_button)) {
            notification_message(app->notifications, &sequence_success);
            ird_app_show_message(app, "Gespeichert", b->label, IrdSceneRemote);
        } else {
            ird_app_show_message(app, "Fehler", "Speichern\nfehlgeschlagen", IrdSceneRemote);
        }
        return true;
    }
    return false;
}

void ird_scene_learn_result_on_exit(void* context) {
    IrdApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
