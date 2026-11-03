#include "../ird_app_i.h"
#include <flipper_format/flipper_format.h>
#include <toolbox/path.h>
#include <string.h>

#define IR_FILE_TYPE    "IR signals file"
#define IR_FILE_VERSION 1

static void ird_scene_signal_pick_callback(void* context, uint32_t index) {
    IrdApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static bool ird_open_ir_file(IrdApp* app, FlipperFormat* ff) {
    bool ok = false;
    FuriString* type = furi_string_alloc();
    uint32_t version = 0;
    do {
        if(!flipper_format_file_open_existing(ff, furi_string_get_cstr(app->file_path))) break;
        if(!flipper_format_read_header(ff, type, &version)) break;
        if(!furi_string_equal(type, IR_FILE_TYPE) || version != IR_FILE_VERSION) break;
        ok = true;
    } while(false);
    furi_string_free(type);
    return ok;
}

/* Signalnamen einlesen und als Menue aufbauen. Rueckgabe: Anzahl Eintraege. */
static uint32_t ird_scene_signal_pick_build(IrdApp* app) {
    FlipperFormat* ff = flipper_format_file_alloc(app->storage);
    FuriString* name = furi_string_alloc();
    FuriString* prev = furi_string_alloc();
    FuriString* label = furi_string_alloc();
    uint32_t count = 0;
    uint32_t dup = 1;

    if(ird_open_ir_file(app, ff)) {
        while(count < IRD_SIGNAL_LIST_MAX &&
              ird_signal_read_name(ff, name)) {
            if(furi_string_equal(name, prev)) {
                dup++;
                furi_string_printf(label, "%s (%lu)", furi_string_get_cstr(name), (unsigned long)dup);
            } else {
                dup = 1;
                furi_string_set(label, name);
                furi_string_set(prev, name);
            }
            submenu_add_item(
                app->submenu, furi_string_get_cstr(label), count, ird_scene_signal_pick_callback, app);
            count++;
        }
    }

    furi_string_free(name);
    furi_string_free(prev);
    furi_string_free(label);
    flipper_format_free(ff);
    return count;
}

/* Laedt Signal Nr. index inkl. Name in app->pending_signal / app->tmp_str. */
static bool ird_scene_signal_pick_load(IrdApp* app, uint32_t index) {
    FlipperFormat* ff = flipper_format_file_alloc(app->storage);
    bool ok = false;
    do {
        if(!ird_open_ir_file(app, ff)) break;
        bool found = true;
        for(uint32_t i = 0; i <= index; i++) {
            if(!ird_signal_read_name(ff, app->tmp_str)) {
                found = false;
                break;
            }
        }
        if(!found) break;
        if(!ird_signal_read_body(app->pending_signal, ff)) break;
        ok = ird_signal_is_valid(app->pending_signal);
    } while(false);
    flipper_format_free(ff);
    return ok;
}

void ird_scene_signal_pick_on_enter(void* context) {
    IrdApp* app = context;
    Submenu* submenu = app->submenu;
    submenu_reset(submenu);

    FuriString* header = furi_string_alloc();
    path_extract_filename(app->file_path, header, true);
    submenu_set_header(submenu, furi_string_get_cstr(header));
    furi_string_free(header);

    uint32_t count = ird_scene_signal_pick_build(app);
    if(count == 0) {
        ird_app_show_message(app, "Keine Signale", "Datei enthaelt keine\ngueltigen Signale", IrdSceneRemote);
        return;
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewSubmenu);
}

bool ird_scene_signal_pick_on_event(void* context, SceneManagerEvent event) {
    IrdApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event >= IRD_SIGNAL_LIST_MAX) return false;

    if(ird_scene_signal_pick_load(app, event.event)) {
        IrdButton* b = &app->remote->buttons[app->current_button];
        FuriString* file_name = furi_string_alloc();
        path_extract_filename(app->file_path, file_name, true);
        snprintf(
            b->source,
            sizeof(b->source),
            "%s/%s",
            furi_string_get_cstr(file_name),
            furi_string_get_cstr(app->tmp_str));
        furi_string_free(file_name);
        b->has_signal = true;
        if(ird_app_save_remote(app, app->pending_signal, app->current_button)) {
            notification_message(app->notifications, &sequence_success);
            ird_app_show_message(app, "Zugewiesen", b->label, IrdSceneRemote);
        } else {
            ird_app_show_message(app, "Fehler", "Speichern\nfehlgeschlagen", IrdSceneRemote);
        }
    } else {
        ird_app_show_message(app, "Fehler", "Signal konnte nicht\ngelesen werden", IrdSceneRemote);
    }
    return true;
}

void ird_scene_signal_pick_on_exit(void* context) {
    IrdApp* app = context;
    submenu_reset(app->submenu);
}
