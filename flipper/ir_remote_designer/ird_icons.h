#pragma once

#include <gui/canvas.h>
#include "ird_icons_data.h"

/** Name eines Symbols (fuer Auswahlmenues). */
const char* ird_icon_name(uint8_t icon);

/** Zeichnet ein Symbol mit linker oberer Ecke bei x,y. Icon 0 (None) zeichnet nichts. */
void ird_icon_draw(Canvas* canvas, int32_t x, int32_t y, uint8_t icon);
