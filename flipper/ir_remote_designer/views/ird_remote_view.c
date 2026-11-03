#include "ird_remote_view.h"
#include "../ird_icons.h"

#include <gui/elements.h>
#include <furi.h>

/* Geometrie: Kopfzeile 11 px, darunter 4 sichtbare Zeilen zu 13 px (= 63 px). */
#define HEADER_H     11
#define GRID_Y       (HEADER_H + 1)
#define CELL_W       31
#define CELL_H       13
#define VISIBLE_ROWS 4
#define GRID_W       (CELL_W * IRD_GRID_COLS) /* 124 px, rechts 4 px fuer Scrollbalken */

typedef struct {
    IrdRemote* remote;
    uint8_t focus;
    uint8_t scroll_row;
    bool move_mode;
    bool transmitting;
    const char* status;
} IrdRemoteViewModel;

struct IrdRemoteView {
    View* view;
    IrdRemoteViewCallback callback;
    void* context;
};

/* ---------------------------------------------------------------- Zeichnen */

/* Kuerzt den Text auf max_w Pixel. Bei schmalen Knoepfen ohne "...", damit
 * moeglichst viele Zeichen sichtbar bleiben. */
static void fit_label(Canvas* canvas, FuriString* s, int32_t max_w, bool ellipsis) {
    if(max_w < 1) max_w = 1;
    if(ellipsis) {
        elements_string_fit_width(canvas, s, max_w);
        return;
    }
    while(furi_string_size(s) > 1 &&
          canvas_string_width(canvas, furi_string_get_cstr(s)) > max_w) {
        furi_string_left(s, furi_string_size(s) - 1);
    }
}

static void draw_label(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    int32_t max_w,
    const char* text,
    bool ellipsis) {
    FuriString* s = furi_string_alloc_set(text);
    fit_label(canvas, s, max_w, ellipsis);
    canvas_draw_str_aligned(canvas, cx, cy, AlignCenter, AlignCenter, furi_string_get_cstr(s));
    furi_string_free(s);
}

static void draw_button(Canvas* canvas, const IrdButton* b, bool focused, bool move_mode, int32_t scroll_row) {
    int32_t x = b->x * CELL_W + 1;
    int32_t y = GRID_Y + (b->y - scroll_row) * CELL_H + 1;
    int32_t w = b->w * CELL_W - 2;
    int32_t h = b->h * CELL_H - 2;

    canvas_set_color(canvas, ColorBlack);
    if(focused) {
        canvas_draw_rbox(canvas, x, y, w, h, 2);
        if(move_mode) {
            /* Doppelrahmen als Markierung fuer den Verschiebe-Modus */
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_rframe(canvas, x + 1, y + 1, w - 2, h - 2, 2);
        }
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_draw_rframe(canvas, x, y, w, h, 2);
    }

    /* Inhalt */
    const bool has_icon = b->icon != IrdIconNone && b->icon < IrdIconCount;
    const int32_t cx = x + w / 2;
    const int32_t cy = y + h / 2;
    /* Platz fuer die "?"-Markierung rechts oben freihalten */
    const int32_t marker_w = b->has_signal ? 0 : 6;
    const int32_t avail = w - 4 - marker_w;
    const bool narrow = b->w == 1;

    if(has_icon && b->h == 1 && b->w == 1) {
        ird_icon_draw(canvas, cx - IRD_ICON_W / 2 - marker_w / 3, cy - IRD_ICON_H / 2, b->icon);
    } else if(has_icon && b->h == 1) {
        canvas_set_font(canvas, FontSecondary);
        int32_t text_w = canvas_string_width(canvas, b->label);
        if(text_w <= avail - IRD_ICON_W - 2 || text_w > avail) {
            /* Symbol links, Text im Rest (ggf. gekuerzt) */
            int32_t tx = x + 3 + IRD_ICON_W + 2;
            int32_t tw = avail - IRD_ICON_W - 3;
            ird_icon_draw(canvas, x + 3, cy - IRD_ICON_H / 2, b->icon);
            draw_label(canvas, tx + tw / 2, cy, tw, b->label, true);
        } else {
            /* Text passt nur ohne Symbol: dann nur Text */
            draw_label(canvas, x + 2 + avail / 2, cy, avail, b->label, true);
        }
    } else if(has_icon) {
        ird_icon_draw(canvas, cx - IRD_ICON_W / 2, y + 2, b->icon);
        canvas_set_font(canvas, FontSecondary);
        draw_label(
            canvas, cx, y + 2 + IRD_ICON_H + (h - IRD_ICON_H - 2) / 2, avail, b->label, !narrow);
    } else {
        canvas_set_font(canvas, narrow ? FontSecondary : FontPrimary);
        draw_label(canvas, x + 2 + avail / 2, cy, avail, b->label, !narrow);
    }

    /* Markierung fuer Knopf ohne Signal */
    if(!b->has_signal) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, x + w - 2, y + 1, AlignRight, AlignTop, "?");
    }
    canvas_set_color(canvas, ColorBlack);
}

static void draw_header(Canvas* canvas, const IrdRemoteViewModel* m) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, HEADER_H);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontSecondary);

    FuriString* left = furi_string_alloc_set(m->remote ? m->remote->name : "");
    FuriString* right = furi_string_alloc();

    if(m->status) {
        furi_string_set(right, m->status);
    } else if(m->move_mode) {
        furi_string_set(left, "Verschieben");
        furi_string_set(right, "OK = fertig");
    } else if(m->transmitting) {
        furi_string_set(right, "Sende...");
    } else if(m->remote && m->remote->button_count > 0 && m->focus < m->remote->button_count) {
        const IrdButton* b = &m->remote->buttons[m->focus];
        furi_string_set(right, b->label);
        if(!b->has_signal) furi_string_cat(right, " (kein Signal)");
    }

    int32_t right_w = canvas_string_width(canvas, furi_string_get_cstr(right));
    if(right_w > 90) right_w = 90;
    elements_string_fit_width(canvas, right, right_w);
    int32_t left_max = 126 - right_w - 4;
    if(left_max < 20) left_max = 20;
    elements_string_fit_width(canvas, left, left_max);

    canvas_draw_str_aligned(canvas, 2, HEADER_H / 2, AlignLeft, AlignCenter, furi_string_get_cstr(left));
    canvas_draw_str_aligned(canvas, 126, HEADER_H / 2, AlignRight, AlignCenter, furi_string_get_cstr(right));

    furi_string_free(left);
    furi_string_free(right);
    canvas_set_color(canvas, ColorBlack);
}

static void ird_remote_view_draw(Canvas* canvas, void* model) {
    IrdRemoteViewModel* m = model;
    canvas_clear(canvas);
    draw_header(canvas, m);

    if(!m->remote || m->remote->button_count == 0) {
        canvas_set_font(canvas, FontSecondary);
        elements_multiline_text_aligned(
            canvas, 64, 38, AlignCenter, AlignCenter, "Keine Knoepfe vorhanden.\nZurueck lang druecken\n= Menue");
        return;
    }

    for(uint8_t i = 0; i < m->remote->button_count; i++) {
        const IrdButton* b = &m->remote->buttons[i];
        if(b->y < m->scroll_row) continue;
        if(b->y + b->h > m->scroll_row + VISIBLE_ROWS) continue;
        draw_button(canvas, b, i == m->focus, m->move_mode, m->scroll_row);
    }

    uint8_t rows = ird_remote_row_count(m->remote);
    if(rows > VISIBLE_ROWS) {
        /* schmaler Scrollbalken rechts */
        int32_t track_h = 64 - GRID_Y;
        int32_t thumb_h = MAX(4, track_h * VISIBLE_ROWS / rows);
        int32_t thumb_y = GRID_Y + (track_h - thumb_h) * m->scroll_row / (rows - VISIBLE_ROWS);
        for(int32_t yy = GRID_Y; yy < 64; yy += 2) canvas_draw_dot(canvas, 126, yy);
        canvas_draw_box(canvas, 125, thumb_y, 3, thumb_h);
    }
}

/* ------------------------------------------------------------- Navigation */

static void ensure_focus_visible(IrdRemoteViewModel* m) {
    if(!m->remote || m->focus >= m->remote->button_count) return;
    const IrdButton* b = &m->remote->buttons[m->focus];
    if(b->y < m->scroll_row) m->scroll_row = b->y;
    if(b->y + b->h > m->scroll_row + VISIBLE_ROWS) m->scroll_row = b->y + b->h - VISIBLE_ROWS;
}

static int find_neighbor(const IrdRemote* r, uint8_t from, InputKey key) {
    const IrdButton* f = &r->buttons[from];
    int fcx = f->x * 2 + f->w; /* Mittelpunkt * 2 */
    int fcy = f->y * 2 + f->h;
    int best = -1;
    int best_score = INT32_MAX;

    for(uint8_t i = 0; i < r->button_count; i++) {
        if(i == from) continue;
        const IrdButton* b = &r->buttons[i];
        int primary = -1;
        int secondary = 0;
        int bcx = b->x * 2 + b->w;
        int bcy = b->y * 2 + b->h;
        switch(key) {
        case InputKeyRight:
            if(b->x < f->x + f->w) continue;
            primary = b->x - (f->x + f->w);
            secondary = abs(bcy - fcy);
            break;
        case InputKeyLeft:
            if(b->x + b->w > f->x) continue;
            primary = f->x - (b->x + b->w);
            secondary = abs(bcy - fcy);
            break;
        case InputKeyDown:
            if(b->y < f->y + f->h) continue;
            primary = b->y - (f->y + f->h);
            secondary = abs(bcx - fcx);
            break;
        case InputKeyUp:
            if(b->y + b->h > f->y) continue;
            primary = f->y - (b->y + b->h);
            secondary = abs(bcx - fcx);
            break;
        default:
            continue;
        }
        int score = primary * 16 + secondary;
        if(score < best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

static bool move_focus(IrdRemoteViewModel* m, InputKey key) {
    if(!m->remote || m->remote->button_count == 0) return false;
    int next = find_neighbor(m->remote, m->focus, key);
    if(next < 0) return false;
    m->focus = next;
    ensure_focus_visible(m);
    return true;
}

static bool move_button(IrdRemoteViewModel* m, InputKey key) {
    if(!m->remote || m->focus >= m->remote->button_count) return false;
    IrdButton* b = &m->remote->buttons[m->focus];
    int nx = b->x, ny = b->y;
    switch(key) {
    case InputKeyRight:
        nx++;
        break;
    case InputKeyLeft:
        nx--;
        break;
    case InputKeyDown:
        ny++;
        break;
    case InputKeyUp:
        ny--;
        break;
    default:
        return false;
    }
    if(nx < 0 || ny < 0) return false;
    if(!ird_remote_is_area_free(m->remote, nx, ny, b->w, b->h, m->focus)) return false;
    b->x = nx;
    b->y = ny;
    ensure_focus_visible(m);
    return true;
}

/* ------------------------------------------------------------------ Input */

static bool ird_remote_view_input(InputEvent* event, void* context) {
    IrdRemoteView* instance = context;
    bool consumed = false;
    bool fire_press = false, fire_release = false, fire_menu = false, fire_move_done = false;

    with_view_model(
        instance->view,
        IrdRemoteViewModel * m,
        {
            const bool is_arrow = event->key == InputKeyUp || event->key == InputKeyDown ||
                                  event->key == InputKeyLeft || event->key == InputKeyRight;

            if(is_arrow) {
                if(event->type == InputTypePress || event->type == InputTypeRepeat) {
                    if(m->move_mode) {
                        move_button(m, event->key);
                    } else {
                        move_focus(m, event->key);
                    }
                }
                consumed = true;
            } else if(event->key == InputKeyOk) {
                if(m->move_mode) {
                    if(event->type == InputTypeShort) {
                        m->move_mode = false;
                        fire_move_done = true;
                    }
                } else {
                    if(event->type == InputTypePress) fire_press = true;
                    if(event->type == InputTypeRelease) fire_release = true;
                }
                consumed = true;
            } else if(event->key == InputKeyBack) {
                if(m->move_mode) {
                    if(event->type == InputTypeShort || event->type == InputTypeLong) {
                        m->move_mode = false;
                        fire_move_done = true;
                    }
                    consumed = true;
                } else if(event->type == InputTypeLong) {
                    fire_menu = true;
                    consumed = true;
                } else if(event->type == InputTypeShort) {
                    consumed = false; /* an Navigation weiterreichen = Fernbedienung verlassen */
                } else {
                    consumed = true;
                }
            }
        },
        true);

    if(instance->callback) {
        if(fire_press) instance->callback(IrdRemoteViewEventPress, instance->context);
        if(fire_release) instance->callback(IrdRemoteViewEventRelease, instance->context);
        if(fire_menu) instance->callback(IrdRemoteViewEventMenu, instance->context);
        if(fire_move_done) instance->callback(IrdRemoteViewEventMoveDone, instance->context);
    }
    return consumed;
}

/* ---------------------------------------------------------------- Public */

IrdRemoteView* ird_remote_view_alloc(void) {
    IrdRemoteView* instance = malloc(sizeof(IrdRemoteView));
    instance->view = view_alloc();
    instance->callback = NULL;
    instance->context = NULL;
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(IrdRemoteViewModel));
    with_view_model(
        instance->view,
        IrdRemoteViewModel * m,
        {
            m->remote = NULL;
            m->focus = 0;
            m->scroll_row = 0;
            m->move_mode = false;
            m->transmitting = false;
            m->status = NULL;
        },
        false);
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, ird_remote_view_draw);
    view_set_input_callback(instance->view, ird_remote_view_input);
    return instance;
}

void ird_remote_view_free(IrdRemoteView* instance) {
    view_free(instance->view);
    free(instance);
}

View* ird_remote_view_get_view(IrdRemoteView* instance) {
    return instance->view;
}

void ird_remote_view_set_callback(IrdRemoteView* instance, IrdRemoteViewCallback callback, void* ctx) {
    instance->callback = callback;
    instance->context = ctx;
}

void ird_remote_view_set_remote(IrdRemoteView* instance, IrdRemote* remote) {
    with_view_model(
        instance->view,
        IrdRemoteViewModel * m,
        {
            m->remote = remote;
            if(!remote || m->focus >= remote->button_count) m->focus = 0;
            m->scroll_row = 0;
            m->move_mode = false;
            m->transmitting = false;
            m->status = NULL;
            ensure_focus_visible(m);
        },
        true);
}

void ird_remote_view_set_focus(IrdRemoteView* instance, uint8_t index) {
    with_view_model(
        instance->view,
        IrdRemoteViewModel * m,
        {
            if(m->remote && index < m->remote->button_count) m->focus = index;
            ensure_focus_visible(m);
        },
        true);
}

uint8_t ird_remote_view_get_focus(IrdRemoteView* instance) {
    uint8_t focus = 0;
    with_view_model(instance->view, IrdRemoteViewModel * m, { focus = m->focus; }, false);
    return focus;
}

void ird_remote_view_set_move_mode(IrdRemoteView* instance, bool enabled) {
    with_view_model(instance->view, IrdRemoteViewModel * m, { m->move_mode = enabled; }, true);
}

bool ird_remote_view_get_move_mode(IrdRemoteView* instance) {
    bool mode = false;
    with_view_model(instance->view, IrdRemoteViewModel * m, { mode = m->move_mode; }, false);
    return mode;
}

void ird_remote_view_set_transmitting(IrdRemoteView* instance, bool transmitting) {
    with_view_model(
        instance->view, IrdRemoteViewModel * m, { m->transmitting = transmitting; }, true);
}

void ird_remote_view_set_status(IrdRemoteView* instance, const char* status) {
    with_view_model(instance->view, IrdRemoteViewModel * m, { m->status = status; }, true);
}

void ird_remote_view_refresh(IrdRemoteView* instance) {
    with_view_model(
        instance->view,
        IrdRemoteViewModel * m,
        {
            if(m->remote && m->focus >= m->remote->button_count) {
                m->focus = m->remote->button_count ? m->remote->button_count - 1 : 0;
            }
            ensure_focus_visible(m);
        },
        true);
}
