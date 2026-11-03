#pragma once

#include <furi.h>
#include <storage/storage.h>
#include "ird_signal.h"

/* Grenzen des Datenmodells ------------------------------------------------ */
#define IRD_NAME_LEN     24 /* Name der Fernbedienung (= Dateiname ohne Endung) */
#define IRD_LABEL_LEN    12 /* Beschriftung eines Knopfes                       */
#define IRD_SOURCE_LEN   40 /* Herkunft des Signals, z.B. "Samsung_TV/Power"   */
#define IRD_MAX_BUTTONS  40
#define IRD_GRID_COLS    4
#define IRD_GRID_ROWS    12
#define IRD_MAX_BTN_W    4
#define IRD_MAX_BTN_H    2

#define IRD_REMOTES_DIR  APP_DATA_PATH("remotes")
#define IRD_REMOTE_EXT   ".ird"
#define IRD_FILE_TYPE    "IR Remote Designer Layout"
#define IRD_FILE_VERSION 1

#define IRD_IR_LIBRARY_DIR EXT_PATH("infrared")
#define IRD_IR_EXT         ".ir"

typedef struct {
    char label[IRD_LABEL_LEN];
    char source[IRD_SOURCE_LEN];
    uint8_t icon;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    bool has_signal;
    /* Index des Signalblocks in der gespeicherten Layout-Datei (-1 = keiner). */
    int16_t file_signal_index;
} IrdButton;

typedef struct {
    char name[IRD_NAME_LEN];
    uint8_t category;
    uint8_t button_count;
    IrdButton buttons[IRD_MAX_BUTTONS];
    FuriString* path;
} IrdRemote;

IrdRemote* ird_remote_alloc(void);
void ird_remote_free(IrdRemote* remote);

/** Setzt die Fernbedienung zurueck (ohne Knoepfe) und berechnet den Dateipfad. */
void ird_remote_reset(IrdRemote* remote, const char* name, uint8_t category);

/** Dateipfad fuer einen Fernbedienungsnamen. */
void ird_remote_path_for_name(FuriString* out, const char* name);

/** Legt das Datenverzeichnis an. */
bool ird_storage_ensure_dirs(Storage* storage);

bool ird_remote_load(IrdRemote* remote, Storage* storage, const char* path);

/**
 * Speichert die Fernbedienung. Vorhandene Signale werden aus der alten Datei
 * uebernommen. new_signal (optional) wird dem Knopf new_signal_button zugewiesen.
 */
bool ird_remote_save(
    IrdRemote* remote,
    Storage* storage,
    const IrdSignal* new_signal,
    int new_signal_button);

/** Laedt das Signal eines Knopfes aus der Layout-Datei. */
bool ird_remote_load_signal(
    const IrdRemote* remote,
    Storage* storage,
    uint8_t button,
    IrdSignal* out);

bool ird_remote_delete_file(IrdRemote* remote, Storage* storage);
bool ird_remote_rename(IrdRemote* remote, Storage* storage, const char* new_name);

/** Fuegt einen Knopf an der ersten freien Stelle ein. Rueckgabe: Index oder -1. */
int ird_remote_add_button(
    IrdRemote* remote,
    const char* label,
    uint8_t icon,
    uint8_t w,
    uint8_t h);

/** Fuegt einen Knopf an einer festen Position ein (fuer Vorlagen). */
int ird_remote_add_button_at(
    IrdRemote* remote,
    const char* label,
    uint8_t icon,
    uint8_t x,
    uint8_t y,
    uint8_t w,
    uint8_t h);

void ird_remote_remove_button(IrdRemote* remote, uint8_t index);
void ird_remote_clear_signal(IrdRemote* remote, uint8_t index);

bool ird_remote_is_area_free(
    const IrdRemote* remote,
    uint8_t x,
    uint8_t y,
    uint8_t w,
    uint8_t h,
    int ignore_index);

bool ird_remote_find_free_cell(
    const IrdRemote* remote,
    uint8_t w,
    uint8_t h,
    uint8_t* out_x,
    uint8_t* out_y);

/** Hoechste belegte Zeile + 1 (Anzahl Zeilen des Layouts). */
uint8_t ird_remote_row_count(const IrdRemote* remote);

