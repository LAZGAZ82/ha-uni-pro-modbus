#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <gui/modules/validators.h>
#include <dialogs/dialogs.h>
#include <storage/storage.h>
#include <notification/notification_messages.h>
#include <infrared_worker.h>

#include "ird_layout.h"
#include "ird_templates.h"
#include "ird_icons.h"
#include "views/ird_remote_view.h"
#include "scenes/ird_scene.h"

#define IRD_APP_VERSION "1.0"
#define IRD_MAX_REMOTES 64
#define IRD_SIGNAL_LIST_MAX 250

typedef enum {
    IrdViewSubmenu,
    IrdViewTextInput,
    IrdViewDialogEx,
    IrdViewPopup,
    IrdViewVarItemList,
    IrdViewWidget,
    IrdViewRemote,
} IrdView;

/* Submenue-Indizes werden direkt als Custom-Event gesendet (kleine Werte);
 * alle anderen Ereignisse liegen oberhalb von 0x1000. */
typedef enum {
    IrdCustomEventTextEntered = 0x1000,
    IrdCustomEventTxStart,
    IrdCustomEventTxStop,
    IrdCustomEventMenu,
    IrdCustomEventMoveDone,
    IrdCustomEventSignalReceived,
    IrdCustomEventDialogLeft,
    IrdCustomEventDialogRight,
    IrdCustomEventPopupDone,
} IrdCustomEvent;

typedef enum {
    IrdNameModeNewRemote,
    IrdNameModeRenameRemote,
    IrdNameModeRenameButton,
    IrdNameModeNewButton,
} IrdNameMode;

typedef enum {
    IrdDeleteModeButton,
    IrdDeleteModeRemote,
} IrdDeleteMode;

typedef struct {
    Gui* gui;
    Storage* storage;
    DialogsApp* dialogs;
    NotificationApp* notifications;

    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Submenu* submenu;
    TextInput* text_input;
    DialogEx* dialog_ex;
    Popup* popup;
    VariableItemList* var_item_list;
    Widget* widget;
    IrdRemoteView* remote_view;

    ValidatorIsFile* validator;

    InfraredWorker* worker;
    bool is_transmitting;
    bool is_receiving;
    uint32_t last_tx_tick;
    IrdSignal* tx_signal; /* Arbeitskopie fuer das Senden */
    IrdSignal* pending_signal; /* neu gewaehltes / angelerntes Signal */

    IrdRemote* remote;
    uint8_t current_button;
    uint8_t new_category;
    IrdNameMode name_mode;
    IrdDeleteMode delete_mode;

    char text_buffer[IRD_NAME_LEN];
    FuriString* file_path;
    FuriString* tmp_str;

    /* Liste der gespeicherten Fernbedienungen (Startmenue) */
    char (*remote_names)[IRD_NAME_LEN];
    uint8_t remote_count;

    /* Parameter fuer die Meldungs-Szene */
    const char* msg_header;
    const char* msg_text;
    uint32_t msg_return_scene;
} IrdApp;

/* ird_ir.c */
bool ird_tx_start(IrdApp* app, uint8_t button);
void ird_tx_stop(IrdApp* app);
void ird_rx_start(IrdApp* app);
void ird_rx_stop(IrdApp* app);

/* ir_remote_designer.c */
bool ird_app_save_remote(IrdApp* app, const IrdSignal* new_signal, int new_signal_button);
void ird_app_show_message(IrdApp* app, const char* header, const char* text, uint32_t return_scene);
void ird_app_scan_remotes(IrdApp* app);
