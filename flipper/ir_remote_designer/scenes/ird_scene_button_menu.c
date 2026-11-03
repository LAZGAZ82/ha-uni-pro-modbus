#include "../ird_app_i.h"

typedef enum {
    IrdButtonMenuAssign,
    IrdButtonMenuLearn,
    IrdButtonMenuClearSignal,
    IrdButtonMenuRename,
    IrdButtonMenuIcon,
    IrdButtonMenuSize,
    IrdButtonMenuMove,
    IrdButtonMenuDelete,
    IrdButtonMenuNewButton,
    IrdButtonMenuRenameRemote,
    IrdButtonMenuDeleteRemote,
} IrdButtonMenuIndex;

static void ird_scene_button_menu_callback(void* context, uint32_t index) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void ird_scene_button_menu_on_enter(void* context) {
    IrdApp* app = context;
    Submenu* submenu = app->submenu;
    submenu_reset(submenu);

    const bool has_button = app->remote->button_count > 0 &&
                            app->current_button < app->remote->button_count;

    if(has_button) {
        const IrdButton* b = &app->remote->buttons[app->current_button];
        furi_string_printf(app->tmp_str, "Knopf: %s", b->label);
        submenu_set_header(submenu, furi_string_get_cstr(app->tmp_str));
        submenu_add_item(
            submenu, "Signal aus Bibliothek", IrdButtonMenuAssign, ird_scene_button_menu_callback, app);
        submenu_add_item(
            submenu, "Signal anlernen", IrdButtonMenuLearn, ird_scene_button_menu_callback, app);
        if(b->has_signal) {
            submenu_add_item(
                submenu, "Signal entfernen", IrdButtonMenuClearSignal, ird_scene_button_menu_callback, app);
        }
        submenu_add_item(
            submenu, "Umbenennen", IrdButtonMenuRename, ird_scene_button_menu_callback, app);
        submenu_add_item(
            submenu, "Symbol aendern", IrdButtonMenuIcon, ird_scene_button_menu_callback, app);
        submenu_add_item(
            submenu, "Groesse aendern", IrdButtonMenuSize, ird_scene_button_menu_callback, app);
        submenu_add_item(
            submenu, "Verschieben", IrdButtonMenuMove, ird_scene_button_menu_callback, app);
        submenu_add_item(
            submenu, "Knopf loeschen", IrdButtonMenuDelete, ird_scene_button_menu_callback, app);
    } else {
        submenu_set_header(submenu, "Fernbedienung");
    }
    submenu_add_item(
        submenu, "+ Neuer Knopf", IrdButtonMenuNewButton, ird_scene_button_menu_callback, app);
    submenu_add_item(
        submenu, "FB umbenennen", IrdButtonMenuRenameRemote, ird_scene_button_menu_callback, app);
    submenu_add_item(
        submenu, "FB loeschen", IrdButtonMenuDeleteRemote, ird_scene_button_menu_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewSubmenu);
}

bool ird_scene_button_menu_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    SceneManager* sm = app->scene_manager;

    switch(event.event) {
    case IrdButtonMenuAssign:
        scene_manager_next_scene(sm, IrdSceneSignalFile);
        break;
    case IrdButtonMenuLearn:
        scene_manager_next_scene(sm, IrdSceneLearn);
        break;
    case IrdButtonMenuClearSignal:
        ird_remote_clear_signal(app->remote, app->current_button);
        ird_app_save_remote(app, NULL, -1);
        scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        break;
    case IrdButtonMenuRename:
        app->name_mode = IrdNameModeRenameButton;
        scene_manager_next_scene(sm, IrdSceneName);
        break;
    case IrdButtonMenuIcon:
        scene_manager_next_scene(sm, IrdSceneIcon);
        break;
    case IrdButtonMenuSize:
        scene_manager_next_scene(sm, IrdSceneSize);
        break;
    case IrdButtonMenuMove:
        ird_remote_view_set_move_mode(app->remote_view, true);
        scene_manager_search_and_switch_to_previous_scene(sm, IrdSceneRemote);
        break;
    case IrdButtonMenuDelete:
        app->delete_mode = IrdDeleteModeButton;
        scene_manager_next_scene(sm, IrdSceneDelete);
        break;
    case IrdButtonMenuNewButton:
        app->name_mode = IrdNameModeNewButton;
        scene_manager_next_scene(sm, IrdSceneName);
        break;
    case IrdButtonMenuRenameRemote:
        app->name_mode = IrdNameModeRenameRemote;
        scene_manager_next_scene(sm, IrdSceneName);
        break;
    case IrdButtonMenuDeleteRemote:
        app->delete_mode = IrdDeleteModeRemote;
        scene_manager_next_scene(sm, IrdSceneDelete);
        break;
    default:
        return false;
    }
    return true;
}

void ird_scene_button_menu_on_exit(void* context) {
    IrdApp* app = context;
    submenu_reset(app->submenu);
}
