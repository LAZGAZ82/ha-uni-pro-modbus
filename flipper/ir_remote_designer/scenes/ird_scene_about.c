#include "../ird_app_i.h"

void ird_scene_about_on_enter(void* context) {
    IrdApp* app = context;
    Widget* widget = app->widget;
    widget_reset(widget);

    widget_add_text_scroll_element(
        widget,
        0,
        0,
        128,
        64,
        "\e#IR Remote Designer v" IRD_APP_VERSION "\n"
        "Eigene IR-Fernbedienungen\n"
        "mit frei belegbaren Tasten.\n"
        "\n"
        "\e#Bedienung\n"
        "Pfeile: Knopf waehlen\n"
        "OK: Signal senden\n"
        "(halten = wiederholen)\n"
        "Zurueck kurz: verlassen\n"
        "Zurueck LANG: Menue fuer\n"
        "den gewaehlten Knopf\n"
        "\n"
        "\e#Knopf-Menue\n"
        "- Signal aus Bibliothek:\n"
        "  .ir-Datei und Signal aus\n"
        "  /ext/infrared waehlen\n"
        "- Signal anlernen: Original-\n"
        "  Fernbedienung auf den\n"
        "  Flipper richten\n"
        "- Umbenennen, Symbol,\n"
        "  Groesse, Verschieben,\n"
        "  Loeschen\n"
        "- Neuer Knopf, FB umbenennen,\n"
        "  FB loeschen\n"
        "\n"
        "\e#Verschieben\n"
        "Pfeile schieben den Knopf\n"
        "im Raster, OK = fertig.\n"
        "\n"
        "\e#Hinweise\n"
        "'?' am Knopf = noch kein\n"
        "Signal zugewiesen.\n"
        "Raster: 4 Spalten, bis zu\n"
        "12 Zeilen (scrollt).\n"
        "Layouts liegen unter\n"
        "/ext/apps_data/\n"
        "ir_remote_designer/remotes\n"
        "\n"
        "Nur fuer Geraete nutzen, die\n"
        "du steuern darfst.\n");

    view_dispatcher_switch_to_view(app->view_dispatcher, IrdViewWidget);
}

bool ird_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ird_scene_about_on_exit(void* context) {
    IrdApp* app = context;
    widget_reset(app->widget);
}
