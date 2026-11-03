#include "ird_layout.h"

#include <flipper_format/flipper_format.h>
#include <toolbox/path.h>
#include <string.h>

#define TAG "IrdLayout"

#define KEY_REMOTE   "Remote"
#define KEY_CATEGORY "Category"
#define KEY_BUTTON   "Button"
#define KEY_ICON     "Icon"
#define KEY_POS      "Pos"
#define KEY_SOURCE   "Source"
#define KEY_SIGNAL   "Signal"

#define TMP_SUFFIX ".tmp"

IrdRemote* ird_remote_alloc(void) {
    IrdRemote* remote = malloc(sizeof(IrdRemote));
    remote->path = furi_string_alloc();
    ird_remote_reset(remote, "", 0);
    return remote;
}

void ird_remote_free(IrdRemote* remote) {
    furi_string_free(remote->path);
    free(remote);
}

void ird_remote_path_for_name(FuriString* out, const char* name) {
    furi_string_printf(out, "%s/%s%s", IRD_REMOTES_DIR, name, IRD_REMOTE_EXT);
}

void ird_remote_reset(IrdRemote* remote, const char* name, uint8_t category) {
    memset(remote->buttons, 0, sizeof(remote->buttons));
    remote->button_count = 0;
    remote->category = category;
    strlcpy(remote->name, name, sizeof(remote->name));
    ird_remote_path_for_name(remote->path, remote->name);
}

bool ird_storage_ensure_dirs(Storage* storage) {
    return storage_simply_mkdir(storage, IRD_REMOTES_DIR);
}

/* ------------------------------------------------------------------------ */

static bool ird_read_header(FlipperFormat* ff) {
    bool ok = false;
    FuriString* type = furi_string_alloc();
    uint32_t version = 0;
    do {
        if(!flipper_format_read_header(ff, type, &version)) break;
        if(!furi_string_equal(type, IRD_FILE_TYPE)) break;
        if(version != IRD_FILE_VERSION) break;
        ok = true;
    } while(false);
    furi_string_free(type);
    return ok;
}

bool ird_remote_load(IrdRemote* remote, Storage* storage, const char* path) {
    bool ok = false;
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* str = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;
        if(!ird_read_header(ff)) break;

        ird_remote_reset(remote, "", 0);
        furi_string_set(remote->path, path);

        if(!flipper_format_read_string(ff, KEY_REMOTE, str)) break;
        strlcpy(remote->name, furi_string_get_cstr(str), sizeof(remote->name));

        uint32_t category = 0;
        if(!flipper_format_read_uint32(ff, KEY_CATEGORY, &category, 1)) break;
        remote->category = (uint8_t)category;

        int16_t signal_index = 0;
        while(remote->button_count < IRD_MAX_BUTTONS &&
              flipper_format_read_string(ff, KEY_BUTTON, str)) {
            IrdButton* b = &remote->buttons[remote->button_count];
            strlcpy(b->label, furi_string_get_cstr(str), sizeof(b->label));

            uint32_t icon = 0;
            uint32_t pos[4] = {0, 0, 1, 1};
            uint32_t has_signal = 0;
            if(!flipper_format_read_uint32(ff, KEY_ICON, &icon, 1)) break;
            if(!flipper_format_read_uint32(ff, KEY_POS, pos, 4)) break;
            if(!flipper_format_read_string(ff, KEY_SOURCE, str)) break;
            if(!flipper_format_read_uint32(ff, KEY_SIGNAL, &has_signal, 1)) break;

            b->icon = (uint8_t)icon;
            b->x = (uint8_t)pos[0];
            b->y = (uint8_t)pos[1];
            b->w = (uint8_t)CLAMP(pos[2], (uint32_t)IRD_MAX_BTN_W, (uint32_t)1);
            b->h = (uint8_t)CLAMP(pos[3], (uint32_t)IRD_MAX_BTN_H, (uint32_t)1);
            if(b->x + b->w > IRD_GRID_COLS) b->x = IRD_GRID_COLS - b->w;
            if(b->y + b->h > IRD_GRID_ROWS) b->y = IRD_GRID_ROWS - b->h;
            if(furi_string_equal(str, "-")) {
                b->source[0] = '\0';
            } else {
                strlcpy(b->source, furi_string_get_cstr(str), sizeof(b->source));
            }
            b->has_signal = has_signal != 0;
            b->file_signal_index = b->has_signal ? signal_index++ : -1;
            remote->button_count++;
        }
        ok = true;
    } while(false);

    furi_string_free(str);
    flipper_format_free(ff);
    if(!ok) FURI_LOG_E(TAG, "Laden fehlgeschlagen: %s", path);
    return ok;
}

/* Liest aus der alten Datei sequentiell das Signal mit Index `index`.
 * `consumed` zaehlt bereits gelesene Signalbloecke mit. */
static bool ird_read_old_signal(
    FlipperFormat* ff_old,
    IrdSignal* signal,
    int16_t index,
    int16_t* consumed) {
    if(!ff_old) return false;
    if(index < *consumed) {
        /* Sollte nicht vorkommen (Reihenfolge bleibt erhalten) - zur Sicherheit neu beginnen. */
        if(!flipper_format_rewind(ff_old)) return false;
        if(!ird_read_header(ff_old)) return false;
        *consumed = 0;
    }
    if(!ird_signal_search_by_index_and_read(signal, ff_old, (size_t)(index - *consumed))) {
        return false;
    }
    *consumed = index + 1;
    return ird_signal_is_valid(signal);
}

bool ird_remote_save(
    IrdRemote* remote,
    Storage* storage,
    const IrdSignal* new_signal,
    int new_signal_button) {
    bool ok = false;
    const char* path = furi_string_get_cstr(remote->path);
    FuriString* tmp_path = furi_string_alloc_printf("%s%s", path, TMP_SUFFIX);
    FlipperFormat* ff_old = NULL;
    FlipperFormat* ff_new = flipper_format_file_alloc(storage);
    IrdSignal* scratch = ird_signal_alloc();
    int16_t consumed = 0;
    int16_t new_index[IRD_MAX_BUTTONS];

    do {
        if(storage_file_exists(storage, path)) {
            ff_old = flipper_format_file_alloc(storage);
            if(!flipper_format_file_open_existing(ff_old, path) || !ird_read_header(ff_old)) {
                FURI_LOG_W(TAG, "Alte Datei nicht lesbar, Signale gehen verloren");
                flipper_format_free(ff_old);
                ff_old = NULL;
            }
        }

        if(!flipper_format_file_open_always(ff_new, furi_string_get_cstr(tmp_path))) break;
        if(!flipper_format_write_header_cstr(ff_new, IRD_FILE_TYPE, IRD_FILE_VERSION)) break;
        if(!flipper_format_write_string_cstr(ff_new, KEY_REMOTE, remote->name)) break;
        uint32_t category = remote->category;
        if(!flipper_format_write_uint32(ff_new, KEY_CATEGORY, &category, 1)) break;

        int16_t signal_counter = 0;
        bool write_ok = true;
        for(uint8_t i = 0; i < remote->button_count && write_ok; i++) {
            IrdButton* b = &remote->buttons[i];
            bool has_new = (new_signal != NULL) && (new_signal_button == (int)i) &&
                           ird_signal_is_valid(new_signal);
            bool has_old = b->has_signal && b->file_signal_index >= 0 && ff_old != NULL;
            const IrdSignal* to_write = NULL;

            if(has_new) {
                to_write = new_signal;
            } else if(has_old) {
                if(ird_read_old_signal(ff_old, scratch, b->file_signal_index, &consumed)) {
                    to_write = scratch;
                } else {
                    FURI_LOG_W(TAG, "Signal %d von '%s' nicht lesbar", b->file_signal_index, b->label);
                }
            }

            uint32_t icon = b->icon;
            uint32_t pos[4] = {b->x, b->y, b->w, b->h};
            uint32_t has_signal = to_write ? 1 : 0;
            const char* source = b->source[0] ? b->source : "-";

            write_ok = flipper_format_write_empty_line(ff_new) &&
                       flipper_format_write_string_cstr(ff_new, KEY_BUTTON, b->label) &&
                       flipper_format_write_uint32(ff_new, KEY_ICON, &icon, 1) &&
                       flipper_format_write_uint32(ff_new, KEY_POS, pos, 4) &&
                       flipper_format_write_string_cstr(ff_new, KEY_SOURCE, source) &&
                       flipper_format_write_uint32(ff_new, KEY_SIGNAL, &has_signal, 1);
            if(write_ok && to_write) {
                write_ok = ird_signal_save(to_write, ff_new, b->label);
            }
            new_index[i] = to_write ? signal_counter++ : -1;
        }
        if(!write_ok) break;

        flipper_format_file_close(ff_new);
        if(ff_old) {
            flipper_format_file_close(ff_old);
        }

        if(storage_file_exists(storage, path)) {
            if(storage_common_remove(storage, path) != FSE_OK) break;
        }
        if(storage_common_rename(storage, furi_string_get_cstr(tmp_path), path) != FSE_OK) break;

        for(uint8_t i = 0; i < remote->button_count; i++) {
            remote->buttons[i].file_signal_index = new_index[i];
            remote->buttons[i].has_signal = new_index[i] >= 0;
            if(!remote->buttons[i].has_signal) remote->buttons[i].source[0] = '\0';
        }
        ok = true;
    } while(false);

    if(!ok) {
        FURI_LOG_E(TAG, "Speichern fehlgeschlagen: %s", path);
        flipper_format_file_close(ff_new);
        storage_common_remove(storage, furi_string_get_cstr(tmp_path));
    }

    ird_signal_free(scratch);
    flipper_format_free(ff_new);
    if(ff_old) flipper_format_free(ff_old);
    furi_string_free(tmp_path);
    return ok;
}

bool ird_remote_load_signal(
    const IrdRemote* remote,
    Storage* storage,
    uint8_t button,
    IrdSignal* out) {
    if(button >= remote->button_count) return false;
    const IrdButton* b = &remote->buttons[button];
    if(!b->has_signal || b->file_signal_index < 0) return false;

    bool ok = false;
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    do {
        if(!flipper_format_file_open_existing(ff, furi_string_get_cstr(remote->path))) break;
        if(!ird_read_header(ff)) break;
        if(!ird_signal_search_by_index_and_read(out, ff, b->file_signal_index)) break;
        ok = ird_signal_is_valid(out);
    } while(false);
    flipper_format_free(ff);
    return ok;
}

bool ird_remote_delete_file(IrdRemote* remote, Storage* storage) {
    const char* path = furi_string_get_cstr(remote->path);
    if(!storage_file_exists(storage, path)) return true;
    return storage_common_remove(storage, path) == FSE_OK;
}

bool ird_remote_rename(IrdRemote* remote, Storage* storage, const char* new_name) {
    FuriString* new_path = furi_string_alloc();
    ird_remote_path_for_name(new_path, new_name);
    bool ok = false;
    do {
        if(furi_string_equal(new_path, remote->path)) {
            ok = true;
            break;
        }
        if(storage_file_exists(storage, furi_string_get_cstr(new_path))) break;
        if(storage_file_exists(storage, furi_string_get_cstr(remote->path))) {
            if(storage_common_rename(
                   storage, furi_string_get_cstr(remote->path), furi_string_get_cstr(new_path)) !=
               FSE_OK)
                break;
        }
        ok = true;
    } while(false);
    if(ok) {
        strlcpy(remote->name, new_name, sizeof(remote->name));
        furi_string_set(remote->path, new_path);
        /* Namen auch in der Datei aktualisieren */
        ok = ird_remote_save(remote, storage, NULL, -1);
    }
    furi_string_free(new_path);
    return ok;
}

/* ------------------------------------------------------------------------ */

bool ird_remote_is_area_free(
    const IrdRemote* remote,
    uint8_t x,
    uint8_t y,
    uint8_t w,
    uint8_t h,
    int ignore_index) {
    if(w < 1 || h < 1) return false;
    if(x + w > IRD_GRID_COLS || y + h > IRD_GRID_ROWS) return false;
    for(uint8_t i = 0; i < remote->button_count; i++) {
        if((int)i == ignore_index) continue;
        const IrdButton* b = &remote->buttons[i];
        bool overlap = (x < b->x + b->w) && (b->x < x + w) && (y < b->y + b->h) && (b->y < y + h);
        if(overlap) return false;
    }
    return true;
}

bool ird_remote_find_free_cell(
    const IrdRemote* remote,
    uint8_t w,
    uint8_t h,
    uint8_t* out_x,
    uint8_t* out_y) {
    for(uint8_t y = 0; y + h <= IRD_GRID_ROWS; y++) {
        for(uint8_t x = 0; x + w <= IRD_GRID_COLS; x++) {
            if(ird_remote_is_area_free(remote, x, y, w, h, -1)) {
                *out_x = x;
                *out_y = y;
                return true;
            }
        }
    }
    return false;
}

int ird_remote_add_button_at(
    IrdRemote* remote,
    const char* label,
    uint8_t icon,
    uint8_t x,
    uint8_t y,
    uint8_t w,
    uint8_t h) {
    if(remote->button_count >= IRD_MAX_BUTTONS) return -1;
    if(!ird_remote_is_area_free(remote, x, y, w, h, -1)) return -1;
    IrdButton* b = &remote->buttons[remote->button_count];
    memset(b, 0, sizeof(*b));
    strlcpy(b->label, label, sizeof(b->label));
    b->icon = icon;
    b->x = x;
    b->y = y;
    b->w = w;
    b->h = h;
    b->has_signal = false;
    b->file_signal_index = -1;
    return remote->button_count++;
}

int ird_remote_add_button(
    IrdRemote* remote,
    const char* label,
    uint8_t icon,
    uint8_t w,
    uint8_t h) {
    uint8_t x, y;
    if(!ird_remote_find_free_cell(remote, w, h, &x, &y)) return -1;
    return ird_remote_add_button_at(remote, label, icon, x, y, w, h);
}

void ird_remote_remove_button(IrdRemote* remote, uint8_t index) {
    if(index >= remote->button_count) return;
    for(uint8_t i = index; i + 1 < remote->button_count; i++) {
        remote->buttons[i] = remote->buttons[i + 1];
    }
    remote->button_count--;
    memset(&remote->buttons[remote->button_count], 0, sizeof(IrdButton));
}

void ird_remote_clear_signal(IrdRemote* remote, uint8_t index) {
    if(index >= remote->button_count) return;
    remote->buttons[index].has_signal = false;
    remote->buttons[index].file_signal_index = -1;
    remote->buttons[index].source[0] = '\0';
}

uint8_t ird_remote_row_count(const IrdRemote* remote) {
    uint8_t rows = 0;
    for(uint8_t i = 0; i < remote->button_count; i++) {
        const IrdButton* b = &remote->buttons[i];
        if(b->y + b->h > rows) rows = b->y + b->h;
    }
    return rows;
}
