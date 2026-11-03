/* Host-Shim: 128x64-Canvas, Mini-Font (3x5), FuriString, View. Nur fuer Layout-Tests. */
#include <furi.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <gui/view.h>

/* ---------------------------------------------------------------- Canvas */
struct Canvas {
    uint8_t px[64][128];
    Color color;
    Font font;
};

Canvas* canvas_test_alloc(void) {
    Canvas* c = calloc(1, sizeof(Canvas));
    c->color = ColorBlack;
    return c;
}
uint8_t* canvas_test_pixels(Canvas* c) {
    return &c->px[0][0];
}

static void put(Canvas* c, int32_t x, int32_t y) {
    if(x < 0 || y < 0 || x >= 128 || y >= 64) return;
    if(c->color == ColorXOR)
        c->px[y][x] ^= 1;
    else
        c->px[y][x] = (c->color == ColorBlack);
}
void canvas_clear(Canvas* c) {
    memset(c->px, 0, sizeof(c->px));
}
void canvas_set_color(Canvas* c, Color color) {
    c->color = color;
}
void canvas_set_font(Canvas* c, Font font) {
    c->font = font;
}
void canvas_draw_dot(Canvas* c, int32_t x, int32_t y) {
    put(c, x, y);
}
void canvas_draw_box(Canvas* c, int32_t x, int32_t y, size_t w, size_t h) {
    for(size_t j = 0; j < h; j++)
        for(size_t i = 0; i < w; i++) put(c, x + i, y + j);
}
void canvas_draw_frame(Canvas* c, int32_t x, int32_t y, size_t w, size_t h) {
    for(size_t i = 0; i < w; i++) {
        put(c, x + i, y);
        put(c, x + i, y + h - 1);
    }
    for(size_t j = 0; j < h; j++) {
        put(c, x, y + j);
        put(c, x + w - 1, y + j);
    }
}
void canvas_draw_rframe(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, size_t r) {
    for(size_t i = r; i < w - r; i++) {
        put(c, x + i, y);
        put(c, x + i, y + h - 1);
    }
    for(size_t j = r; j < h - r; j++) {
        put(c, x, y + j);
        put(c, x + w - 1, y + j);
    }
    /* Ecken (r=2 vereinfachte Rundung) */
    for(size_t k = 1; k < r; k++) {
        put(c, x + k, y + r - k);
        put(c, x + w - 1 - k, y + r - k);
        put(c, x + k, y + h - 1 - (r - k));
        put(c, x + w - 1 - k, y + h - 1 - (r - k));
    }
}
void canvas_draw_rbox(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, size_t r) {
    for(size_t j = 0; j < h; j++) {
        size_t inset = 0;
        if(j < r) inset = r - j;
        if(j >= h - r) inset = j - (h - r) + 1;
        for(size_t i = inset; i < w - inset; i++) put(c, x + i, y + j);
    }
}
void canvas_draw_xbm(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, const uint8_t* bitmap) {
    size_t stride = (w + 7) / 8;
    for(size_t j = 0; j < h; j++)
        for(size_t i = 0; i < w; i++)
            if(bitmap[j * stride + i / 8] & (1 << (i % 8))) put(c, x + i, y + j);
}

/* 3x5-Font, Grossbuchstaben + Ziffern + ein paar Zeichen */
static const char* glyph(char ch) {
    switch(ch) {
    case 'A': return "010101111101101";
    case 'B': return "110101110101110";
    case 'C': return "011100100100011";
    case 'D': return "110101101101110";
    case 'E': return "111100110100111";
    case 'F': return "111100110100100";
    case 'G': return "011100101101011";
    case 'H': return "101101111101101";
    case 'I': return "111010010010111";
    case 'J': return "001001001101010";
    case 'K': return "101101110101101";
    case 'L': return "100100100100111";
    case 'M': return "101111111101101";
    case 'N': return "101111111111101";
    case 'O': return "010101101101010";
    case 'P': return "110101110100100";
    case 'Q': return "010101101111011";
    case 'R': return "110101110101101";
    case 'S': return "011100010001110";
    case 'T': return "111010010010010";
    case 'U': return "101101101101011";
    case 'V': return "101101101101010";
    case 'W': return "101101111111101";
    case 'X': return "101101010101101";
    case 'Y': return "101101010010010";
    case 'Z': return "111001010100111";
    case '0': return "010101101101010";
    case '1': return "010110010010111";
    case '2': return "110001010100111";
    case '3': return "110001010001110";
    case '4': return "101101111001001";
    case '5': return "111100110001110";
    case '6': return "011100110101010";
    case '7': return "111001010010010";
    case '8': return "010101010101010";
    case '9': return "010101011001110";
    case '+': return "000010111010000";
    case '-': return "000000111000000";
    case '/': return "001001010100100";
    case '?': return "110001010000010";
    case '.': return "000000000000010";
    case ':': return "000010000010000";
    case '(': return "001010010010001";
    case ')': return "100010010010100";
    case '=': return "000111000111000";
    case '_': return "000000000000111";
    case ' ': return NULL;
    default: return "111111111111111";
    }
}
static int advance(Canvas* c) {
    return c->font == FontPrimary ? 6 : 5;
}
static int font_h(Canvas* c) {
    return c->font == FontPrimary ? 7 : 6; /* Anhaltswerte helvB08 / haxrcorp */
}
uint16_t canvas_string_width(Canvas* c, const char* s) {
    return (uint16_t)(strlen(s) * advance(c) - (strlen(s) ? 1 : 0));
}
static void draw_char(Canvas* c, int32_t x, int32_t y, char ch, bool bold) {
    const char* g = glyph((char)toupper((unsigned char)ch));
    if(!g) return;
    for(int j = 0; j < 5; j++)
        for(int i = 0; i < 3; i++)
            if(g[j * 3 + i] == '1') {
                put(c, x + i, y + j);
                if(bold) put(c, x + i + 1, y + j);
            }
}
/* y = Grundlinie */
void canvas_draw_str(Canvas* c, int32_t x, int32_t y, const char* s) {
    bool bold = c->font == FontPrimary;
    for(; *s; s++) {
        draw_char(c, x, y - 5, *s, bold);
        x += advance(c);
    }
}
void canvas_draw_str_aligned(Canvas* c, int32_t x, int32_t y, Align h, Align v, const char* s) {
    int w = canvas_string_width(c, s);
    int fh = font_h(c);
    if(h == AlignCenter) x -= w / 2;
    if(h == AlignRight) x -= w;
    if(v == AlignTop) y += fh;
    if(v == AlignCenter) y += fh / 2;
    canvas_draw_str(c, x, y, s);
}

/* -------------------------------------------------------------- Elements */
void elements_string_fit_width(Canvas* c, FuriString* s, size_t width) {
    if(canvas_string_width(c, furi_string_get_cstr(s)) <= width) return;
    while(furi_string_size(s) > 1 && canvas_string_width(c, furi_string_get_cstr(s)) + 3 * advance(c) > (int)width) {
        furi_string_left(s, furi_string_size(s) - 1);
    }
    furi_string_cat_str(s, "...");
}
void elements_multiline_text_aligned(Canvas* c, int32_t x, int32_t y, Align h, Align v, const char* text) {
    char buf[128];
    strlcpy(buf, text, sizeof(buf));
    int lines = 1;
    for(const char* p = buf; *p; p++) if(*p == '\n') lines++;
    int lh = font_h(c) + 2;
    if(v == AlignCenter) y -= (lines * lh) / 2 - lh / 2;
    char* line = strtok(buf, "\n");
    while(line) {
        canvas_draw_str_aligned(c, x, y, h, AlignCenter, line);
        y += lh;
        line = strtok(NULL, "\n");
    }
}

/* ------------------------------------------------------------ FuriString */
FuriString* furi_string_alloc(void) {
    FuriString* s = malloc(sizeof(FuriString));
    s->s = calloc(1, 1);
    return s;
}
FuriString* furi_string_alloc_set_str(const char* v) {
    FuriString* s = furi_string_alloc();
    furi_string_set_str(s, v);
    return s;
}
FuriString* furi_string_alloc_set_fs(const FuriString* v) {
    return furi_string_alloc_set_str(v->s);
}
FuriString* furi_string_alloc_printf(const char* fmt, ...) {
    FuriString* s = furi_string_alloc();
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    furi_string_set_str(s, buf);
    return s;
}
void furi_string_free(FuriString* s) {
    free(s->s);
    free(s);
}
void furi_string_set_str(FuriString* s, const char* v) {
    free(s->s);
    s->s = strdup(v);
}
void furi_string_set_fs(FuriString* s, const FuriString* v) {
    furi_string_set_str(s, v->s);
}
void furi_string_cat_str(FuriString* s, const char* v) {
    size_t n = strlen(s->s) + strlen(v) + 1;
    s->s = realloc(s->s, n);
    strcat(s->s, v);
}
void furi_string_cat_fs(FuriString* s, const FuriString* v) {
    furi_string_cat_str(s, v->s);
}
void furi_string_printf(FuriString* s, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    furi_string_set_str(s, buf);
}
const char* furi_string_get_cstr(const FuriString* s) {
    return s->s;
}
bool furi_string_empty(const FuriString* s) {
    return s->s[0] == 0;
}
bool furi_string_equal_str(const FuriString* s, const char* v) {
    return strcmp(s->s, v) == 0;
}
bool furi_string_equal_fs(const FuriString* s, const FuriString* v) {
    return strcmp(s->s, v->s) == 0;
}
void furi_string_left(FuriString* s, size_t n) {
    if(strlen(s->s) > n) s->s[n] = 0;
}
size_t furi_string_size(const FuriString* s) {
    return strlen(s->s);
}
size_t strlcpy(char* dst, const char* src, size_t size) {
    size_t n = strlen(src);
    if(size) {
        size_t c = n < size - 1 ? n : size - 1;
        memcpy(dst, src, c);
        dst[c] = 0;
    }
    return n;
}

/* ------------------------------------------------------------------ View */
struct View {
    void* model;
    void* context;
    ViewDrawCallback draw;
    ViewInputCallback input;
};
View* view_alloc(void) {
    return calloc(1, sizeof(View));
}
void view_free(View* v) {
    free(v->model);
    free(v);
}
void view_allocate_model(View* v, ViewModelType t, size_t size) {
    (void)t;
    v->model = calloc(1, size);
}
void* view_get_model(View* v) {
    return v->model;
}
void view_commit_model(View* v, bool update) {
    (void)v;
    (void)update;
}
void view_set_context(View* v, void* ctx) {
    v->context = ctx;
}
void view_set_draw_callback(View* v, ViewDrawCallback cb) {
    v->draw = cb;
}
void view_set_input_callback(View* v, ViewInputCallback cb) {
    v->input = cb;
}
void view_test_draw(View* v, Canvas* canvas) {
    v->draw(canvas, v->model);
}
bool view_test_input(View* v, InputKey key, InputType type) {
    InputEvent e = {.sequence = 0, .key = key, .type = type};
    return v->input(&e, v->context);
}
