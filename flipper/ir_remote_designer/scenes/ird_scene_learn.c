#include "../ird_app_i.h"

void ird_scene_learn_on_enter(void* context) {
    IrdApp* app = context;
    Popup* popup = app->popup;

    popup_reset(popup);
    popup_set_header(popup, "Signal anlernen", 64, 4, AlignCenter, AlignTop);
    popup_set_text(
        popup,
        "Original-Fernbedienung auf\nden Flipper richten und\nTaste druecken.\n\nZurueck = Abbrechen",
        64,
        18,
        AlignCenter,
        AlignTop);
    popup_disable_timeout(popup);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewPopup);
    ird_rx_start(app);
}

bool ird_scene_learn_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == IrdCustomEventSignalReceived) {
        ird_rx_stop(app);
        scene_manager_next_scene(app->scene_manager, IrdSceneLearnResult);
        return true;
    }
    return false;
}

void ird_scene_learn_on_exit(void* context) {
    IrdApp* app = context;
    ird_rx_stop(app);
    popup_reset(app->popup);
}
