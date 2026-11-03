#pragma once

#include <gui/view.h>
#include "../ird_layout.h"

typedef enum {
    IrdRemoteViewEventPress, /* OK gedrueckt: Senden starten */
    IrdRemoteViewEventRelease, /* OK losgelassen: Senden stoppen */
    IrdRemoteViewEventMenu, /* Zurueck lang gedrueckt: Bearbeitungsmenue */
    IrdRemoteViewEventMoveDone, /* Verschieben beendet: Layout speichern */
} IrdRemoteViewEvent;

typedef void (*IrdRemoteViewCallback)(IrdRemoteViewEvent event, void* context);

typedef struct IrdRemoteView IrdRemoteView;

IrdRemoteView* ird_remote_view_alloc(void);
void ird_remote_view_free(IrdRemoteView* view);
View* ird_remote_view_get_view(IrdRemoteView* view);

void ird_remote_view_set_callback(IrdRemoteView* view, IrdRemoteViewCallback callback, void* ctx);

/** Zu zeigende Fernbedienung (Zeiger bleibt im Besitz der App). */
void ird_remote_view_set_remote(IrdRemoteView* view, IrdRemote* remote);

void ird_remote_view_set_focus(IrdRemoteView* view, uint8_t index);
uint8_t ird_remote_view_get_focus(IrdRemoteView* view);

void ird_remote_view_set_move_mode(IrdRemoteView* view, bool enabled);
bool ird_remote_view_get_move_mode(IrdRemoteView* view);

void ird_remote_view_set_transmitting(IrdRemoteView* view, bool transmitting);

/** Kurze Statusmeldung in der Kopfzeile (NULL = keine). */
void ird_remote_view_set_status(IrdRemoteView* view, const char* status);

/** Neu zeichnen (nach Aenderungen am Layout durch die App). */
void ird_remote_view_refresh(IrdRemoteView* view);
