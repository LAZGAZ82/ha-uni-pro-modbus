#include "../ird_app_i.h"

static void ird_scene_remote_view_callback(IrdRemoteViewEvent event, void* context) {
    IrdApp* app = context;
    uint32_t custom;
    switch(event) {
    case IrdRemoteViewEventPress:
        custom = IrdCustomEventTxStart;
        break;
    case IrdRemoteViewEventRelease:
        custom = IrdCustomEventTxStop;
        break;
    case IrdRemoteViewEventMenu:
        custom = IrdCustomEventMenu;
        break;
    case IrdRemoteViewEventMoveDone:
    default:
        custom = IrdCustomEventMoveDone;
        break;
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, custom);
}

void ird_scene_remote_on_enter(void* context) {
    IrdApp* app = context;
    ird_remote_view_set_callback(app->remote_view, ird_scene_remote_view_callback, app);
    ird_remote_view_set_status(app->remote_view, NULL);
    ird_remote_view_refresh(app->remote_view);
    ird_remote_view_set_focus(app->remote_view, app->current_button);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewRemote);
}

bool ird_scene_remote_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        switch(event.event) {
        case IrdCustomEventTxStart: {
            uint8_t focus = ird_remote_view_get_focus(app->remote_view);
            ird_remote_view_set_status(app->remote_view, NULL);
            if(!ird_tx_start(app, focus)) {
                ird_remote_view_set_status(app->remote_view, "Kein Signal!");
                notification_message(app->notifications, &sequence_blink_red_100);
            }
            break;
        }
        case IrdCustomEventTxStop:
            ird_tx_stop(app);
            break;
        case IrdCustomEventMenu:
            ird_tx_stop(app);
            app->current_button = ird_remote_view_get_focus(app->remote_view);
            scene_manager_next_scene(app->scene_manager, IrdSceneButtonMenu);
            break;
        case IrdCustomEventMoveDone:
            ird_app_save_remote(app, NULL, -1);
            break;
        default:
            consumed = false;
            break;
        }
    }
    return consumed;
}

void ird_scene_remote_on_exit(void* context) {
    IrdApp* app = context;
    ird_tx_stop(app);
    ird_remote_view_set_status(app->remote_view, NULL);
}
