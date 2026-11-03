#pragma once

#include <stdint.h>
#include "ird_layout.h"

typedef enum {
    IrdCategoryTv,
    IrdCategoryProjector,
    IrdCategoryAc,
    IrdCategoryDvd,
    IrdCategoryHifi,
    IrdCategorySoundbar,
    IrdCategoryReceiver,
    IrdCategoryLed,
    IrdCategoryFan,
    IrdCategoryPresentation,
    IrdCategoryOther,
    IrdCategoryCount,
} IrdCategory;

const char* ird_category_name(uint8_t category);

/** Fuellt eine leere Fernbedienung mit der Vorlage der Kategorie. */
void ird_template_apply(IrdRemote* remote, uint8_t category);
