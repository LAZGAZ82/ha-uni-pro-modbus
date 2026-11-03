#include "ird_templates.h"
#include "ird_icons.h"

typedef struct {
    const char* label;
    uint8_t icon;
    uint8_t x, y, w, h;
} TplButton;

/* Raster: 4 Spalten, Zeilen beliebig (4 Zeilen sichtbar, Rest scrollt). */

static const TplButton tpl_tv[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Stumm", IrdIconMute, 1, 0, 1, 1},
    {"Source", IrdIconSource, 2, 0, 1, 1},
    {"Menu", IrdIconMenu, 3, 0, 1, 1},
    {"Vol+", IrdIconVolUp, 0, 1, 1, 1},
    {"Hoch", IrdIconUp, 2, 1, 1, 1},
    {"Kanal+", IrdIconChUp, 3, 1, 1, 1},
    {"Vol-", IrdIconVolDown, 0, 2, 1, 1},
    {"Links", IrdIconLeft, 1, 2, 1, 1},
    {"OK", IrdIconOk, 2, 2, 1, 1},
    {"Rechts", IrdIconRight, 3, 2, 1, 1},
    {"Zurueck", IrdIconBack, 0, 3, 1, 1},
    {"Runter", IrdIconDown, 2, 3, 1, 1},
    {"Kanal-", IrdIconChDown, 3, 3, 1, 1},
    {"Play/Pause", IrdIconPlayPause, 0, 4, 2, 1},
    {"Stop", IrdIconStop, 2, 4, 1, 1},
    {"Info", IrdIconInfo, 3, 4, 1, 1},
};

static const TplButton tpl_projector[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Source", IrdIconSource, 1, 0, 1, 1},
    {"Blank", IrdIconMoon, 2, 0, 1, 1},
    {"Menu", IrdIconMenu, 3, 0, 1, 1},
    {"Vol+", IrdIconVolUp, 0, 1, 1, 1},
    {"Hoch", IrdIconUp, 2, 1, 1, 1},
    {"Zoom+", IrdIconZoomIn, 3, 1, 1, 1},
    {"Vol-", IrdIconVolDown, 0, 2, 1, 1},
    {"Links", IrdIconLeft, 1, 2, 1, 1},
    {"OK", IrdIconOk, 2, 2, 1, 1},
    {"Rechts", IrdIconRight, 3, 2, 1, 1},
    {"Zurueck", IrdIconBack, 0, 3, 1, 1},
    {"Runter", IrdIconDown, 2, 3, 1, 1},
    {"Zoom-", IrdIconZoomOut, 3, 3, 1, 1},
    {"Stumm", IrdIconMute, 0, 4, 1, 1},
    {"Trapez", IrdIconNone, 1, 4, 2, 1},
    {"Format", IrdIconTv, 3, 4, 1, 1},
};

static const TplButton tpl_ac[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Modus", IrdIconMode, 1, 0, 1, 1},
    {"Timer", IrdIconTimer, 2, 0, 1, 1},
    {"Sleep", IrdIconMoon, 3, 0, 1, 1},
    {"Temp+", IrdIconTempUp, 0, 1, 2, 2},
    {"Temp-", IrdIconTempDown, 2, 1, 2, 2},
    {"Luefter", IrdIconFan, 0, 3, 1, 1},
    {"Swing", IrdIconSwing, 1, 3, 1, 1},
    {"Turbo", IrdIconSun, 2, 3, 1, 1},
    {"Licht", IrdIconLight, 3, 3, 1, 1},
};

static const TplButton tpl_dvd[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Eject", IrdIconEject, 1, 0, 1, 1},
    {"Titel", IrdIconHash, 2, 0, 1, 1},
    {"Menu", IrdIconMenu, 3, 0, 1, 1},
    {"Play", IrdIconPlay, 0, 1, 1, 1},
    {"Hoch", IrdIconUp, 2, 1, 1, 1},
    {"Pause", IrdIconPause, 3, 1, 1, 1},
    {"Stop", IrdIconStop, 0, 2, 1, 1},
    {"Links", IrdIconLeft, 1, 2, 1, 1},
    {"OK", IrdIconOk, 2, 2, 1, 1},
    {"Rechts", IrdIconRight, 3, 2, 1, 1},
    {"Zurueck", IrdIconBack, 0, 3, 1, 1},
    {"Runter", IrdIconDown, 2, 3, 1, 1},
    {"Audio", IrdIconSpeaker, 3, 3, 1, 1},
    {"Prev", IrdIconPrev, 0, 4, 1, 1},
    {"Rew", IrdIconRewind, 1, 4, 1, 1},
    {"FF", IrdIconForward, 2, 4, 1, 1},
    {"Next", IrdIconNext, 3, 4, 1, 1},
};

static const TplButton tpl_hifi[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Source", IrdIconSource, 1, 0, 1, 1},
    {"Tuner", IrdIconHash, 2, 0, 1, 1},
    {"USB", IrdIconNone, 3, 0, 1, 1},
    {"Vol+", IrdIconVolUp, 0, 1, 2, 1},
    {"Vol-", IrdIconVolDown, 2, 1, 2, 1},
    {"Prev", IrdIconPrev, 0, 2, 1, 1},
    {"Play/Pause", IrdIconPlayPause, 1, 2, 2, 1},
    {"Next", IrdIconNext, 3, 2, 1, 1},
    {"Stop", IrdIconStop, 0, 3, 1, 1},
    {"Stumm", IrdIconMute, 1, 3, 1, 1},
    {"Bass+", IrdIconPlus, 2, 3, 1, 1},
    {"Bass-", IrdIconMinus, 3, 3, 1, 1},
};

static const TplButton tpl_soundbar[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Source", IrdIconSource, 1, 0, 1, 1},
    {"BT", IrdIconNone, 2, 0, 1, 1},
    {"Stumm", IrdIconMute, 3, 0, 1, 1},
    {"Vol+", IrdIconVolUp, 0, 1, 2, 1},
    {"Vol-", IrdIconVolDown, 2, 1, 2, 1},
    {"Bass+", IrdIconPlus, 0, 2, 1, 1},
    {"Bass-", IrdIconMinus, 1, 2, 1, 1},
    {"Surround", IrdIconSpeaker, 2, 2, 2, 1},
    {"Nacht", IrdIconMoon, 0, 3, 1, 1},
    {"Modus", IrdIconMode, 1, 3, 1, 1},
    {"Play/Pause", IrdIconPlayPause, 2, 3, 2, 1},
};

static const TplButton tpl_receiver[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Input+", IrdIconChUp, 1, 0, 1, 1},
    {"Input-", IrdIconChDown, 2, 0, 1, 1},
    {"Stumm", IrdIconMute, 3, 0, 1, 1},
    {"Vol+", IrdIconVolUp, 0, 1, 1, 1},
    {"Hoch", IrdIconUp, 2, 1, 1, 1},
    {"HDMI1", IrdIconNone, 3, 1, 1, 1},
    {"Vol-", IrdIconVolDown, 0, 2, 1, 1},
    {"Links", IrdIconLeft, 1, 2, 1, 1},
    {"OK", IrdIconOk, 2, 2, 1, 1},
    {"Rechts", IrdIconRight, 3, 2, 1, 1},
    {"Menu", IrdIconMenu, 0, 3, 1, 1},
    {"Runter", IrdIconDown, 2, 3, 1, 1},
    {"HDMI2", IrdIconNone, 3, 3, 1, 1},
    {"Zurueck", IrdIconBack, 0, 4, 1, 1},
    {"TV", IrdIconTv, 1, 4, 1, 1},
    {"BT", IrdIconNone, 2, 4, 1, 1},
    {"Modus", IrdIconMode, 3, 4, 1, 1},
};

static const TplButton tpl_led[] = {
    {"An", IrdIconPower, 0, 0, 2, 1},
    {"Aus", IrdIconNone, 2, 0, 2, 1},
    {"Heller", IrdIconSun, 0, 1, 2, 1},
    {"Dunkler", IrdIconMoon, 2, 1, 2, 1},
    {"Rot", IrdIconNone, 0, 2, 1, 1},
    {"Gruen", IrdIconNone, 1, 2, 1, 1},
    {"Blau", IrdIconNone, 2, 2, 1, 1},
    {"Weiss", IrdIconLight, 3, 2, 1, 1},
    {"Flash", IrdIconNone, 0, 3, 1, 1},
    {"Strobe", IrdIconNone, 1, 3, 1, 1},
    {"Fade", IrdIconNone, 2, 3, 1, 1},
    {"Smooth", IrdIconNone, 3, 3, 1, 1},
};

static const TplButton tpl_fan[] = {
    {"Power", IrdIconPower, 0, 0, 2, 1},
    {"Modus", IrdIconMode, 2, 0, 1, 1},
    {"Licht", IrdIconLight, 3, 0, 1, 1},
    {"Stufe+", IrdIconPlus, 0, 1, 2, 1},
    {"Stufe-", IrdIconMinus, 2, 1, 2, 1},
    {"Swing", IrdIconSwing, 0, 2, 1, 1},
    {"Timer", IrdIconTimer, 1, 2, 1, 1},
    {"Natur", IrdIconFan, 2, 2, 1, 1},
    {"Sleep", IrdIconMoon, 3, 2, 1, 1},
};

static const TplButton tpl_presentation[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
    {"Start", IrdIconPlay, 1, 0, 1, 1},
    {"Blank", IrdIconMoon, 2, 0, 1, 1},
    {"Menu", IrdIconMenu, 3, 0, 1, 1},
    {"Zurueck", IrdIconLeft, 0, 1, 2, 2},
    {"Vor", IrdIconRight, 2, 1, 2, 2},
    {"Zoom+", IrdIconZoomIn, 0, 3, 1, 1},
    {"Zoom-", IrdIconZoomOut, 1, 3, 1, 1},
    {"Vol+", IrdIconVolUp, 2, 3, 1, 1},
    {"Vol-", IrdIconVolDown, 3, 3, 1, 1},
};

static const TplButton tpl_other[] = {
    {"Power", IrdIconPower, 0, 0, 1, 1},
};

typedef struct {
    const char* name;
    const TplButton* buttons;
    uint8_t count;
} Template;

#define TPL(n, arr) {n, arr, COUNT_OF(arr)}

static const Template templates[IrdCategoryCount] = {
    [IrdCategoryTv] = TPL("Fernseher", tpl_tv),
    [IrdCategoryProjector] = TPL("Beamer / Projektor", tpl_projector),
    [IrdCategoryAc] = TPL("Klimaanlage", tpl_ac),
    [IrdCategoryDvd] = TPL("DVD / Blu-ray", tpl_dvd),
    [IrdCategoryHifi] = TPL("Musikanlage", tpl_hifi),
    [IrdCategorySoundbar] = TPL("Soundbar", tpl_soundbar),
    [IrdCategoryReceiver] = TPL("AV-Receiver", tpl_receiver),
    [IrdCategoryLed] = TPL("LED-Beleuchtung", tpl_led),
    [IrdCategoryFan] = TPL("Ventilator", tpl_fan),
    [IrdCategoryPresentation] = TPL("Praesentation", tpl_presentation),
    [IrdCategoryOther] = TPL("Sonstiges Geraet", tpl_other),
};

const char* ird_category_name(uint8_t category) {
    if(category >= IrdCategoryCount) return "Sonstiges";
    return templates[category].name;
}

void ird_template_apply(IrdRemote* remote, uint8_t category) {
    if(category >= IrdCategoryCount) category = IrdCategoryOther;
    const Template* t = &templates[category];
    for(uint8_t i = 0; i < t->count; i++) {
        const TplButton* b = &t->buttons[i];
        ird_remote_add_button_at(remote, b->label, b->icon, b->x, b->y, b->w, b->h);
    }
}
