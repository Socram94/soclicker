#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/widget.h>
#include <furi.h>
#include <furi/core/record.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Enumeration des vues
typedef enum {
    ViewIndexWidget,
    ViewIndexCount,
} ViewIndex;

// Structure principale de l'application
typedef struct {
    ViewDispatcher* view_dispatcher;
    Widget* widget;
    int32_t counter;
    char display_text[64];
} SoclickerApp;

// Callback pour le bouton retour
static bool soclicker_navigation_callback(void* context) {
    furi_assert(context);
    SoclickerApp* app = context;
    view_dispatcher_stop(app->view_dispatcher);
    return true;
}

// Callback pour les événements personnalisés
static bool soclicker_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    SoclickerApp* app = context;
    
    if(event < ViewIndexCount) {
        view_dispatcher_switch_to_view(app->view_dispatcher, event);
    }
    return true;
}

// Callback pour les boutons
static void soclicker_button_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    SoclickerApp* app = context;

    if(button_type == GuiButtonTypeCenter && input_type == InputTypeShort) {
        app->counter++;
        snprintf(app->display_text, sizeof(app->display_text), "Clicks: %ld", (long)app->counter);
        
        // Mettre à jour l'affichage
        widget_reset(app->widget);
        widget_add_string_multiline_element(
            app->widget, 64, 32, AlignCenter, AlignCenter, FontSecondary, app->display_text);
        widget_add_button_element(
            app->widget,
            GuiButtonTypeCenter,
            "Click!",
            soclicker_button_callback,
            app);
    }
}

// Constructeur de l'application
static SoclickerApp* soclicker_alloc() {
    SoclickerApp* app = malloc(sizeof(SoclickerApp));
    memset(app, 0, sizeof(SoclickerApp));

    // Initialiser le compteur
    app->counter = 0;
    snprintf(app->display_text, sizeof(app->display_text), "Clicks: %ld", (long)app->counter);

    // Obtenir le GUI
    Gui* gui = furi_record_open(RECORD_GUI);

    // Créer le view dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    // Créer le widget
    app->widget = widget_alloc();
    widget_add_string_multiline_element(
        app->widget, 64, 32, AlignCenter, AlignCenter, FontSecondary, app->display_text);
    widget_add_button_element(
        app->widget,
        GuiButtonTypeCenter,
        "Click!",
        soclicker_button_callback,
        app);

    // Enregistrer la vue
    view_dispatcher_add_view(app->view_dispatcher, ViewIndexWidget, widget_get_view(app->widget));

    // Configurer les callbacks
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, soclicker_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, soclicker_navigation_callback);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    return app;
}

// Destructeur de l'application
static void soclicker_free(SoclickerApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, ViewIndexWidget);
    view_dispatcher_free(app->view_dispatcher);
    widget_free(app->widget);
    furi_record_close(RECORD_GUI);
    free(app);
}

// Fonction de lancement
static void soclicker_run(SoclickerApp* app) {
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIndexWidget);
    view_dispatcher_run(app->view_dispatcher);
}

// Point d'entrée de l'application
int32_t soclicker(void* arg) {
    UNUSED(arg);

    SoclickerApp* app = soclicker_alloc();
    soclicker_run(app);
    soclicker_free(app);

    return 0;
}