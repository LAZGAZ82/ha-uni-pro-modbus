#pragma once
#include <stdint.h>
#include <stddef.h>
typedef enum { ColorWhite = 0, ColorBlack = 1, ColorXOR = 2 } Color;
typedef enum { FontPrimary, FontSecondary, FontKeyboard, FontBigNumbers } Font;
typedef enum { AlignLeft, AlignRight, AlignTop, AlignBottom, AlignCenter } Align;
typedef struct Canvas Canvas;
void canvas_clear(Canvas* c);
void canvas_set_color(Canvas* c, Color color);
void canvas_set_font(Canvas* c, Font font);
void canvas_draw_str(Canvas* c, int32_t x, int32_t y, const char* s);
void canvas_draw_str_aligned(Canvas* c, int32_t x, int32_t y, Align h, Align v, const char* s);
uint16_t canvas_string_width(Canvas* c, const char* s);
void canvas_draw_dot(Canvas* c, int32_t x, int32_t y);
void canvas_draw_box(Canvas* c, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_frame(Canvas* c, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_rframe(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, size_t r);
void canvas_draw_rbox(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, size_t r);
void canvas_draw_xbm(Canvas* c, int32_t x, int32_t y, size_t w, size_t h, const uint8_t* bitmap);
