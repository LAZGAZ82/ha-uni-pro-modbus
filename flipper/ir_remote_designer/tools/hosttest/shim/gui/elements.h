#pragma once
#include "canvas.h"
#include <furi.h>
void elements_string_fit_width(Canvas* c, FuriString* s, size_t width);
void elements_multiline_text_aligned(Canvas* c, int32_t x, int32_t y, Align h, Align v, const char* text);
