#include "../ird_app_i.h"
#include <string.h>

static void ird_scene_name_text_callback(void* context) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventTextEntered);
}

void ird_scene_name_on_enter(void* context) {
    IrdApp* app = context;
    TextInput* text_input = app->text_input;
    size_t buffer_size = IRD_NAME_LEN;
    bool clear_default = true;

    text_input_reset(text_input);

    switch(app->name_mode) {
    case IrdNameModeNewRemote:
        text_input_set_header_text(text_input, "Name der Fernbedienung:");
        strlcpy(app->text_buffer, ird_category_name(app->new_category), sizeof(app->text_buffer));
        /* Leerzeichen und Sonderzeichen im Vorschlag vermeiden */
        for(char* c = app->text_buffer; *c; c++) {
            if(*c == ' ' || *c == '/') *c = '_';
        }
        app->validator = validator_is_file_alloc_init(IRD_REMOTES_DIR, IRD_REMOTE_EXT, "");
        text_input_set_validator(text_input, validator_is_file_callback, app->validator);
        break;
    case IrdNameModeRenameRemote:
        text_input_set_header_text(text_input, "Fernbedienung umbenennen:");
        strlcpy(app->text_buffer, app->remote->name, sizeof(app->text_buffer));
        app->validator =
            validator_is_file_alloc_init(IRD_REMOTES_DIR, IRD_REMOTE_EXT, app->remote->name);
        text_input_set_validator(text_input, validator_is_file_callback, app->validator);
        clear_default = false;
        break;
    case IrdNameModeRenameButton:
        text_input_set_header_text(text_input, "Knopf-Beschriftung:");
        buffer_size = IRD_LABEL_LEN;
        strlcpy(
            app->text_buffer, app->remote->buttons[app->current_button].label, IRD_LABEL_LEN);
        clear_default = false;
        break;
    case IrdNameModeNewButton:
    default:
        text_input_set_header_text(text_input, "Neuer Knopf - Beschriftung:");
        buffer_size = IRD_LABEL_LEN;
        strlcpy(app->text_buffer, "Neu", IRD_LABEL_LEN);
        break;
    }

    text_input_set_minimum_length(text_input, 1);
    text_input_set_result_callback(
        text_input, ird_scene_name_text_callback, app, app->text_buffer, buffer_size, clear_default);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewTextInput);
}

bool ird_scene_name_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != IrdCustomEventTextEntered) {
        return false;
    }

    switch(app->name_mode) {
    case IrdNameModeNewRemote:
        ird_remote_reset(app->remote, app->text_buffer, app->new_category);
        ird_template_apply(app->remote, app->new_category);
        app->current_button = 0;
        if(ird_remote_save(app->remote, app->storage, NULL, -1)) {
            ird_remote_view_set_remote(app->remote_view, app->remote);
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneStart);
            scene_manager_next_scene(app->scene_manager, IrdSceneRemote);
        } else {
            ird_app_show_message(app, "Fehler", "Speichern\nfehlgeschlagen", IrdSceneStart);
        }
        break;
    case IrdNameModeRenameRemote:
        if(!ird_remote_rename(app->remote, app->storage, app->text_buffer)) {
            ird_app_show_message(app, "Fehler", "Umbenennen\nfehlgeschlagen", IrdSceneRemote);
        } else {
            ird_remote_view_refresh(app->remote_view);
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneRemote);
        }
        break;
    case IrdNameModeRenameButton:
        strlcpy(app->remote->buttons[app->current_button].label, app->text_buffer, IRD_LABEL_LEN);
        ird_app_save_remote(app, NULL, -1);
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneRemote);
        break;
    case IrdNameModeNewButton:
    default: {
        int idx = ird_remote_add_button(app->remote, app->text_buffer, IrdIconNone, 1, 1);
        if(idx < 0) {
            ird_app_show_message(app, "Kein Platz", "Raster ist voll\n(max. 40 Knoepfe)", IrdSceneRemote);
        } else {
            app->current_button = idx;
            ird_app_save_remote(app, NULL, -1);
            ird_remote_view_set_focus(app->remote_view, idx);
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneRemote);
        }
        break;
    }
    }
    return true;
}

void ird_scene_name_on_exit(void* context) {
    IrdApp* app = context;
    text_input_set_validator(app->text_input, NULL, NULL);
    if(app->validator) {
        validator_is_file_free(app->validator);
        app->validator = NULL;
    }
    text_input_reset(app->text_input);
}
