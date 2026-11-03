#include "../ird_app_i.h"

static void ird_scene_message_popup_callback(void* context) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventPopupDone);
}

void ird_scene_message_on_enter(void* context) {
    IrdApp* app = context;
    Popup* popup = app->popup;
    popup_reset(popup);
    popup_set_header(popup, app->msg_header, 64, 6, AlignCenter, AlignTop);
    popup_set_text(popup, app->msg_text, 64, 24, AlignCenter, AlignTop);
    popup_set_context(popup, app);
    popup_set_callback(popup, ird_scene_message_popup_callback);
    popup_set_timeout(popup, 1200);
    popup_enable_timeout(popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewPopup);
}

static void ird_scene_message_leave(IrdApp* app) {
    if(!scene_manager_search_and_switch_to_previous_scene(app->scene_manager, app->msg_return_scene)) {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneStart);
    }
}

bool ird_scene_message_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type == SceneManagerEventTypeBack) {
        ird_scene_message_leave(app);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == IrdCustomEventPopupDone) {
        ird_scene_message_leave(app);
        return true;
    }
    return false;
}

void ird_scene_message_on_exit(void* context) {
    IrdApp* app = context;
    popup_reset(app->popup);
}
