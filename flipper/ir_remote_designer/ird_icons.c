#include "ird_icons.h"

const char* ird_icon_name(uint8_t icon) {
    if(icon >= IrdIconCount) return "?";
    return ird_icon_table[icon].name;
}

void ird_icon_draw(Canvas* canvas, int32_t x, int32_t y, uint8_t icon) {
    if(icon == IrdIconNone || icon >= IrdIconCount) return;
    canvas_draw_xbm(canvas, x, y, IRD_ICON_W, IRD_ICON_H, ird_icon_table[icon].bits);
}
