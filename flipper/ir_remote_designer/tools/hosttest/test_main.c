/* Rendert Vorlagen der App auf dem Host als PBM-Bilder und prueft Navigation. */
#include <furi.h>
#include <gui/canvas.h>
#include <gui/view.h>
#include <ctype.h>
#include "../../ird_layout.h"
#include "../../ird_templates.h"
#include "../../views/ird_remote_view.h"

Canvas* canvas_test_alloc(void);
uint8_t* canvas_test_pixels(Canvas* c);

/* Minimal-Implementierung der Layout-Funktionen (Original benoetigt Storage). */
bool ird_remote_is_area_free(const IrdRemote* r, uint8_t x, uint8_t y, uint8_t w, uint8_t h, int ignore) {
    if(w < 1 || h < 1) return false;
    if(x + w > IRD_GRID_COLS || y + h > IRD_GRID_ROWS) return false;
    for(uint8_t i = 0; i < r->button_count; i++) {
        if((int)i == ignore) continue;
        const IrdButton* b = &r->buttons[i];
        if((x < b->x + b->w) && (b->x < x + w) && (y < b->y + b->h) && (b->y < y + h)) return false;
    }
    return true;
}
int ird_remote_add_button_at(IrdRemote* r, const char* label, uint8_t icon, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    if(r->button_count >= IRD_MAX_BUTTONS) return -1;
    if(!ird_remote_is_area_free(r, x, y, w, h, -1)) {
        fprintf(stderr, "KOLLISION: %s bei %u,%u %ux%u\n", label, x, y, w, h);
        return -1;
    }
    IrdButton* b = &r->buttons[r->button_count];
    memset(b, 0, sizeof(*b));
    strlcpy(b->label, label, sizeof(b->label));
    b->icon = icon; b->x = x; b->y = y; b->w = w; b->h = h; b->file_signal_index = -1;
    return r->button_count++;
}
uint8_t ird_remote_row_count(const IrdRemote* r) {
    uint8_t rows = 0;
    for(uint8_t i = 0; i < r->button_count; i++)
        if(r->buttons[i].y + r->buttons[i].h > rows) rows = r->buttons[i].y + r->buttons[i].h;
    return rows;
}

static void save_pbm(Canvas* c, const char* name) {
    FILE* f = fopen(name, "w");
    fprintf(f, "P1\n128 64\n");
    uint8_t* p = canvas_test_pixels(c);
    for(int y = 0; y < 64; y++) {
        for(int x = 0; x < 128; x++) fputc(p[y * 128 + x] ? '1' : '0', f);
        fputc('\n', f);
    }
    fclose(f);
}

static void dump_frame(IrdRemoteView* view, Canvas* c, const char* name) {
    canvas_clear(c);
    view_test_draw(ird_remote_view_get_view(view), c);
    save_pbm(c, name);
}

static void cb(IrdRemoteViewEvent e, void* ctx) {
    int* counter = ctx;
    counter[e]++;
}

int main(void) {
    Canvas* c = canvas_test_alloc();
    IrdRemoteView* view = ird_remote_view_alloc();
    int events[4] = {0};
    ird_remote_view_set_callback(view, cb, events);
    char name[64];

    for(uint8_t cat = 0; cat < IrdCategoryCount; cat++) {
        IrdRemote* r = calloc(1, sizeof(IrdRemote));
        r->path = furi_string_alloc();
        strlcpy(r->name, ird_category_name(cat), sizeof(r->name));
        r->category = cat;
        ird_template_apply(r, cat);
        /* Jeder zweite Knopf bekommt ein "Signal", damit beide Zustaende sichtbar sind */
        for(uint8_t i = 0; i < r->button_count; i += 2) r->buttons[i].has_signal = true;
        ird_remote_view_set_remote(view, r);
        snprintf(name, sizeof(name), "out/cat%02u_a.pbm", cat);
        dump_frame(view, c, name);
        /* Navigation: 2x rechts, 3x runter */
        View* v = ird_remote_view_get_view(view);
        view_test_input(v, InputKeyRight, InputTypePress);
        view_test_input(v, InputKeyRight, InputTypePress);
        view_test_input(v, InputKeyDown, InputTypePress);
        view_test_input(v, InputKeyDown, InputTypePress);
        view_test_input(v, InputKeyDown, InputTypePress);
        snprintf(name, sizeof(name), "out/cat%02u_b.pbm", cat);
        dump_frame(view, c, name);
        printf("Kategorie %u (%s): %u Knoepfe, %u Zeilen, Fokus nach Navigation = %u (%s)\n",
            cat, ird_category_name(cat), r->button_count, ird_remote_row_count(r),
            ird_remote_view_get_focus(view), r->buttons[ird_remote_view_get_focus(view)].label);
        free(r->path); free(r);
    }

    /* Verschiebe-Modus + Sende-Events am TV-Layout */
    IrdRemote* r = calloc(1, sizeof(IrdRemote));
    r->path = furi_string_alloc();
    strlcpy(r->name, "Wohnzimmer TV", sizeof(r->name));
    ird_template_apply(r, IrdCategoryTv);
    ird_remote_view_set_remote(view, r);
    View* v = ird_remote_view_get_view(view);
    view_test_input(v, InputKeyOk, InputTypePress);
    view_test_input(v, InputKeyOk, InputTypeRelease);
    bool back_short_consumed = view_test_input(v, InputKeyBack, InputTypeShort);
    bool back_long_consumed = view_test_input(v, InputKeyBack, InputTypeLong);
    printf("Events: press=%d release=%d menu=%d movedone=%d | Back kurz konsumiert=%d, Back lang konsumiert=%d\n",
        events[0], events[1], events[2], events[3], back_short_consumed, back_long_consumed);
    ird_remote_view_set_focus(view, 0); /* Power */
    ird_remote_view_set_move_mode(view, true);
    view_test_input(v, InputKeyDown, InputTypePress); /* belegt durch Vol+ -> keine Bewegung */
    view_test_input(v, InputKeyRight, InputTypePress); /* belegt durch Stumm -> keine Bewegung */
    printf("Power nach blockierten Moves: %u,%u (erwartet 0,0)\n", r->buttons[0].x, r->buttons[0].y);
    ird_remote_view_set_focus(view, 5); /* Hoch (2,1) */
    view_test_input(v, InputKeyLeft, InputTypePress); /* (1,1) ist frei */
    printf("Hoch nach Move links: %u,%u (erwartet 1,1)\n", r->buttons[5].x, r->buttons[5].y);
    dump_frame(view, c, "out/move_mode.pbm");
    view_test_input(v, InputKeyOk, InputTypeShort);
    printf("movedone=%d move_mode=%d\n", events[3], ird_remote_view_get_move_mode(view));
    /* Leere Fernbedienung */
    r->button_count = 0;
    ird_remote_view_set_remote(view, r);
    dump_frame(view, c, "out/empty.pbm");
    ird_remote_view_free(view);
    return 0;
}
