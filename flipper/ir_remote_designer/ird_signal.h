#pragma once

/*
 * Eigene, schlanke Signal-Implementierung. Die Firmware-Funktionen
 * infrared_signal_* sind fuer externe Apps (FAP) nicht freigegeben, daher
 * werden .ir-Signalbloecke hier selbst gelesen/geschrieben. Das Dateiformat
 * ist identisch zur offiziellen Infrarot-App.
 */

#include <furi.h>
#include <infrared.h>
#include <flipper_format/flipper_format.h>
#include <furi_hal_infrared.h>

#define IRD_MAX_TIMINGS 1024U

typedef struct {
    bool is_raw;
    InfraredMessage message;
    uint32_t* timings;
    size_t timings_size;
    uint32_t frequency;
    float duty_cycle;
} IrdSignal;

IrdSignal* ird_signal_alloc(void);
void ird_signal_free(IrdSignal* signal);

bool ird_signal_is_valid(const IrdSignal* signal);

void ird_signal_set_message(IrdSignal* signal, const InfraredMessage* message);
void ird_signal_set_raw(
    IrdSignal* signal,
    const uint32_t* timings,
    size_t timings_size,
    uint32_t frequency,
    float duty_cycle);

/** Liest den naechsten "name:"-Eintrag ab der aktuellen Position. */
bool ird_signal_read_name(FlipperFormat* ff, FuriString* name);

/** Liest den Signalkoerper (type/protocol/... bzw. frequency/duty_cycle/data). */
bool ird_signal_read_body(IrdSignal* signal, FlipperFormat* ff);

/** Ueberspringt `index` Signale ab der aktuellen Position und liest das naechste. */
bool ird_signal_search_by_index_and_read(IrdSignal* signal, FlipperFormat* ff, size_t index);

/** Schreibt einen kompletten Signalblock (# / name / type / ...). */
bool ird_signal_save(const IrdSignal* signal, FlipperFormat* ff, const char* name);

/** Kurzbeschreibung, z.B. "NEC A:04 C:08" oder "RAW 68 Flanken". */
void ird_signal_describe(const IrdSignal* signal, FuriString* out);
