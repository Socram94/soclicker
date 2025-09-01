/**
 * @file soclicker.c
 * @author Socram94
 * @brief A simple clicker application for Flipper Zero.
 * @version 0.1
 * @date 30/08/2025
 */

#include "soclicker.h"
#include "View/soclicker_home.h"
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <furi.h>  // pour furi_record_open/close
#include <gui/canvas.h>
#include <gui/elements.h>

Soclicker* soclicker_alloc() {
    Soclicker* app = (Soclicker*)malloc(sizeof(Soclicker));
    memset(app, 0, sizeof(Soclicker));

    // 1. Ouvrir le GUI
    app->gui = furi_record_open(RECORD_GUI);

    // 2. Allouer le view dispatcher
    app->view_dispatcher = view_dispatcher_alloc();

    // 3. Attacher le dispatcher au GUI
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // 4. Créer et ajouter la vue Home
    app->home_view = view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ViewIndexHome, app->home_view);

    // 5. Initialiser ton compteur
    app->count = 0;

    return app;
}


void soclicker_free(Soclicker* app) {
    if(!app) return;

    // 1. Retirer la vue Home si elle existe
    if(app->home_view) {
        view_dispatcher_remove_view(app->view_dispatcher, ViewIndexHome);
        view_free(app->home_view);  // Libère la mémoire de la vue
        app->home_view = NULL;
    }

    // 2. Libérer le view_dispatcher
    if(app->view_dispatcher) {
        view_dispatcher_free(app->view_dispatcher);
        app->view_dispatcher = NULL;
    }

    // 3. Fermer le GUI
    if(app->gui) {
        furi_record_close(RECORD_GUI);
        app->gui = NULL;
    }

    // 4. Libérer la structure principale
    free(app);
}

static void soclciker_run(Soclicker* app){
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIndexHome);
    view_dispatcher_run(app->view_dispatcher);
}

int32_t soclicker(void* arg) {
    UNUSED(arg);
    Soclicker* app = soclicker_alloc(); // Récupérer l'app
    soclciker_run(app);                // Passer app
    soclicker_free(app);               // Libérer app
    return 0;
}