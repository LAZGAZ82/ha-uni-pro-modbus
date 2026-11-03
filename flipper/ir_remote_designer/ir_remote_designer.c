#include "ird_app_i.h"

#include <toolbox/path.h>
#include <stdlib.h>
#include <string.h>

#define TAG "IrRemoteDesigner"

static bool ird_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    IrdApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool ird_back_event_callback(void* context) {
    furi_assert(context);
    IrdApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

bool ird_app_save_remote(IrdApp* app, const IrdSignal* new_signal, int new_signal_button) {
    bool ok = ird_remote_save(app->remote, app->storage, new_signal, new_signal_button);
    if(!ok) {
        notification_message(app->notifications, &sequence_error);
    }
    ird_remote_view_refresh(app->remote_view);
    return ok;
}

void ird_app_show_message(IrdApp* app, const char* header, const char* text, uint32_t return_scene) {
    app->msg_header = header;
    app->msg_text = text;
    app->msg_return_scene = return_scene;
    scene_manager_next_scene(app->scene_manager, IrdSceneMessage);
}

/* Einfache Sortierung (qsort ist fuer FAPs nicht freigegeben). */
static void ird_sort_names(char (*names)[IRD_NAME_LEN], uint8_t count) {
    char tmp[IRD_NAME_LEN];
    for(uint8_t i = 1; i < count; i++) {
        memcpy(tmp, names[i], IRD_NAME_LEN);
        int j = i - 1;
        while(j >= 0 && strcasecmp(names[j], tmp) > 0) {
            memcpy(names[j + 1], names[j], IRD_NAME_LEN);
            j--;
        }
        memcpy(names[j + 1], tmp, IRD_NAME_LEN);
    }
}

void ird_app_scan_remotes(IrdApp* app) {
    app->remote_count = 0;
    File* dir = storage_file_alloc(app->storage);
    FileInfo info;
    char name[64];
    if(storage_dir_open(dir, IRD_REMOTES_DIR)) {
        while(app->remote_count < IRD_MAX_REMOTES &&
              storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info)) continue;
            size_t len = strlen(name);
            size_t ext_len = strlen(IRD_REMOTE_EXT);
            if(len <= ext_len) continue;
            if(strcasecmp(name + len - ext_len, IRD_REMOTE_EXT) != 0) continue;
            name[len - ext_len] = '\0';
            strlcpy(app->remote_names[app->remote_count], name, IRD_NAME_LEN);
            app->remote_count++;
        }
    }
    storage_dir_close(dir);
    storage_file_free(dir);
    if(app->remote_count > 1) {
        ird_sort_names(app->remote_names, app->remote_count);
    }
}

static IrdApp* ird_app_alloc(void) {
    IrdApp* app = malloc(sizeof(IrdApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&ird_scene_handlers, app);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, ird_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, ird_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, IrdViewSubmenu, submenu_get_view(app->submenu));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, IrdViewTextInput, text_input_get_view(app->text_input));

    app->dialog_ex = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, IrdViewDialogEx, dialog_ex_get_view(app->dialog_ex));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, IrdViewPopup, popup_get_view(app->popup));

    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, IrdViewVarItemList, variable_item_list_get_view(app->var_item_list));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, IrdViewWidget, widget_get_view(app->widget));

    app->remote_view = ird_remote_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, IrdViewRemote, ird_remote_view_get_view(app->remote_view));

    app->validator = NULL;

    app->worker = infrared_worker_alloc();
    app->is_transmitting = false;
    app->is_receiving = false;
    app->last_tx_tick = 0;
    app->tx_signal = ird_signal_alloc();
    app->pending_signal = ird_signal_alloc();

    app->remote = ird_remote_alloc();
    app->current_button = 0;
    app->new_category = 0;
    app->name_mode = IrdNameModeNewRemote;
    app->delete_mode = IrdDeleteModeButton;
    memset(app->text_buffer, 0, sizeof(app->text_buffer));
    app->file_path = furi_string_alloc();
    app->tmp_str = furi_string_alloc();

    app->remote_names = malloc(IRD_MAX_REMOTES * IRD_NAME_LEN);
    app->remote_count = 0;

    app->msg_header = "";
    app->msg_text = "";
    app->msg_return_scene = IrdSceneStart;

    return app;
}

static void ird_app_free(IrdApp* app) {
    ird_tx_stop(app);
    ird_rx_stop(app);

    view_dispatcher_remove_view(app->view_dispatcher, IrdViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewDialogEx);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewVarItemList);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, IrdViewRemote);

    submenu_free(app->submenu);
    text_input_free(app->text_input);
    dialog_ex_free(app->dialog_ex);
    popup_free(app->popup);
    variable_item_list_free(app->var_item_list);
    widget_free(app->widget);
    ird_remote_view_free(app->remote_view);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    infrared_worker_free(app->worker);
    ird_signal_free(app->tx_signal);
    ird_signal_free(app->pending_signal);
    ird_remote_free(app->remote);

    furi_string_free(app->file_path);
    furi_string_free(app->tmp_str);
    free(app->remote_names);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t ir_remote_designer_app(void* p) {
    UNUSED(p);
    IrdApp* app = ird_app_alloc();

    if(!ird_storage_ensure_dirs(app->storage)) {
        FURI_LOG_E(TAG, "Datenverzeichnis konnte nicht angelegt werden");
    }

    scene_manager_next_scene(app->scene_manager, IrdSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    ird_app_free(app);
    return 0;
}
