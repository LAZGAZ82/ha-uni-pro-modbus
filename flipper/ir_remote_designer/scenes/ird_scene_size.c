#include "../ird_app_i.h"

static const char* const width_names[IRD_MAX_BTN_W] = {"1", "2", "3", "4"};
static const char* const height_names[IRD_MAX_BTN_H] = {"1", "2"};

static void ird_scene_size_apply(IrdApp* app, VariableItem* item, bool is_width) {
    IrdButton* b = &app->remote->buttons[app->current_button];
    uint8_t idx = variable_item_get_current_value_index(item);
    uint8_t new_w = is_width ? idx + 1 : b->w;
    uint8_t new_h = is_width ? b->h : idx + 1;
    uint8_t x = b->x;
    uint8_t y = b->y;

    /* Falls der Knopf rechts/unten aus dem Raster ragt, nach links/oben ruecken */
    if(x + new_w > IRD_GRID_COLS) x = IRD_GRID_COLS - new_w;
    if(y + new_h > IRD_GRID_ROWS) y = IRD_GRID_ROWS - new_h;

    if(ird_remote_is_area_free(app->remote, x, y, new_w, new_h, app->current_button)) {
        b->x = x;
        b->y = y;
        b->w = new_w;
        b->h = new_h;
        variable_item_set_current_value_text(item, is_width ? width_names[idx] : height_names[idx]);
    } else {
        /* kein Platz: alten Wert wiederherstellen */
        uint8_t old = is_width ? b->w - 1 : b->h - 1;
        variable_item_set_current_value_index(item, old);
        variable_item_set_current_value_text(item, is_width ? width_names[old] : height_names[old]);
        notification_message(app->notifications, &sequence_blink_red_100);
    }
}

static void ird_scene_size_width_changed(VariableItem* item) {
    ird_scene_size_apply(variable_item_get_context(item), item, true);
}

static void ird_scene_size_height_changed(VariableItem* item) {
    ird_scene_size_apply(variable_item_get_context(item), item, false);
}

void ird_scene_size_on_enter(void* context) {
    IrdApp* app = context;
    VariableItemList* list = app->var_item_list;
    const IrdButton* b = &app->remote->buttons[app->current_button];

    variable_item_list_reset(list);

    VariableItem* item =
        variable_item_list_add(list, "Breite (Spalten)", IRD_MAX_BTN_W, ird_scene_size_width_changed, app);
    variable_item_set_current_value_index(item, b->w - 1);
    variable_item_set_current_value_text(item, width_names[b->w - 1]);

    item = variable_item_list_add(
        list, "Hoehe (Zeilen)", IRD_MAX_BTN_H, ird_scene_size_height_changed, app);
    variable_item_set_current_value_index(item, b->h - 1);
    variable_item_set_current_value_text(item, height_names[b->h - 1]);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewVarItemList);
}

bool ird_scene_size_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type == SceneManagerEventTypeBack) {
        ird_app_save_remote(app, NULL, -1);
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, IrdSceneRemote);
        return true;
    }
    return false;
}

void ird_scene_size_on_exit(void* context) {
    IrdApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
