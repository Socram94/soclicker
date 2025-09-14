#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/widget.h>
#include <storage/storage.h>
#include <furi.h>
#include <furi/core/record.h>
#include <furi/core/string.h>
#include <furi/core/timer.h>
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
    Storage* storage;
    FuriString* save_path;
    
    // Timer et revenu passif
    FuriTimer* auto_timer;
    int32_t passive_income;       // Revenu par seconde
    int32_t income_per_click;     // Revenu généré par clic
    char income_text[64];         // Texte d'affichage du revenu
    
    // Upgrades
    int32_t upgrade_cost_income;  // Coût du prochain upgrade revenu
    int32_t upgrade_cost_mult;    // Coût du multiplicateur
    int32_t upgrade_cost_auto;    // Coût de l'auto-click
    int32_t multiplier;           // Multiplicateur de clics
    bool auto_click_enabled;      // Auto-click activé
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

// Ajouter cette fonction après soclicker_load_data()
// Créer le dossier de sauvegarde si nécessaire
static void soclicker_ensure_directory(SoclickerApp* app) {
    FuriString* dir_path = furi_string_alloc_set_str("/ext/apps_data/soclicker");
    storage_common_mkdir(app->storage, furi_string_get_cstr(dir_path));
    furi_string_free(dir_path);
}

// Sauvegarder les données
static void soclicker_save_data(SoclickerApp* app) {
    FuriString* data = furi_string_alloc();
    furi_string_printf(data, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d", 
        (long)app->counter,
        (long)app->passive_income,
        (long)app->income_per_click,
        (long)app->upgrade_cost_income,
        (long)app->upgrade_cost_mult,
        (long)app->upgrade_cost_auto,
        (long)app->multiplier,
        app->auto_click_enabled ? 1 : 0);
    
    File* file = storage_file_alloc(app->storage);
    if(storage_file_open(file, furi_string_get_cstr(app->save_path), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, furi_string_get_cstr(data), furi_string_size(data));
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_string_free(data);
}

// Charger les données
static void soclicker_load_data(SoclickerApp* app) {
    File* file = storage_file_alloc(app->storage);
    if(storage_file_open(file, furi_string_get_cstr(app->save_path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buffer[128];
        uint16_t bytes_read = storage_file_read(file, buffer, sizeof(buffer) - 1);
        buffer[bytes_read] = '\0';
        
        // Parser les données séparées par virgules
        sscanf(buffer, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d", 
            (long*)&app->counter,
            (long*)&app->passive_income,
            (long*)&app->income_per_click,
            (long*)&app->upgrade_cost_income,
            (long*)&app->upgrade_cost_mult,
            (long*)&app->upgrade_cost_auto,
            (long*)&app->multiplier,
            (int*)&app->auto_click_enabled);
        
        storage_file_close(file);
        
        // Si on a chargé des données, s'assurer que multiplier est initialisé
        if(app->multiplier == 0) {
            app->multiplier = 1;
        }
        
        // Limiter le multiplicateur à une valeur raisonnable
        if(app->multiplier > 100) {
            app->multiplier = 100;
        }
        
        // Si le compteur est trop élevé, le limiter
        if(app->counter > 1000000) {
            app->counter = 1000000;
        }
    } else {
        // Valeurs par défaut
        app->counter = 0;
        app->passive_income = 0;
        app->income_per_click = 10;  // Augmenté de 1 à 10 pour être plus visible
        app->upgrade_cost_income = 10;
        app->upgrade_cost_mult = 50;
        app->upgrade_cost_auto = 200;
        app->multiplier = 1;
        app->auto_click_enabled = false;
    }
    storage_file_free(file);
}

// Déclarations des fonctions (forward declarations)
static void soclicker_update_display(SoclickerApp* app);
static void soclicker_button_callback(GuiButtonType button_type, InputType input_type, void* context);
static void soclicker_upgrade_income_callback(GuiButtonType button_type, InputType input_type, void* context);
static void soclicker_upgrade_mult_callback(GuiButtonType button_type, InputType input_type, void* context);
static void soclicker_upgrade_auto_callback(GuiButtonType button_type, InputType input_type, void* context);

// Callback du timer de revenu passif
static void soclicker_timer_callback(void* context) {
    furi_assert(context);
    SoclickerApp* app = context;
    
    if(app->passive_income > 0) {
        app->counter += app->passive_income;
    }
    
    // Auto-click si activé
    if(app->auto_click_enabled) {
        app->counter += 10 * app->multiplier; // Même logique que les clics manuels
    }
    
    // Mettre à jour l'affichage seulement (pas de sauvegarde pour éviter les conflits)
    soclicker_update_display(app);
}

// Mettre à jour l'affichage
static void soclicker_update_display(SoclickerApp* app) {
    snprintf(app->display_text, sizeof(app->display_text), "Clicks: %ld", (long)app->counter);
    snprintf(app->income_text, sizeof(app->income_text), "Income: +%ld/sec", (long)app->passive_income);
    
    widget_reset(app->widget);
    
    // Affichage principal - mieux espacé
    widget_add_string_multiline_element(
        app->widget, 64, 15, AlignCenter, AlignCenter, FontPrimary, app->display_text);
    widget_add_string_multiline_element(
        app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, app->income_text);
    
    // Bouton principal
    widget_add_button_element(
        app->widget,
        GuiButtonTypeCenter,
        "Click!",
        soclicker_button_callback,
        app);
    
    // Boutons d'upgrades - mieux positionnés
    char upgrade_text[32];
    snprintf(upgrade_text, sizeof(upgrade_text), "+1/sec (%ld)", (long)app->upgrade_cost_income);
    widget_add_button_element(
        app->widget,
        GuiButtonTypeLeft,
        upgrade_text,
        soclicker_upgrade_income_callback,
        app);
    
    snprintf(upgrade_text, sizeof(upgrade_text), "x2 (%ld)", (long)app->upgrade_cost_mult);
    widget_add_button_element(
        app->widget,
        GuiButtonTypeRight,
        upgrade_text,
        soclicker_upgrade_mult_callback,
        app);
    
    // Afficher l'auto-click comme texte si pas encore acheté
    if(!app->auto_click_enabled) {
        snprintf(upgrade_text, sizeof(upgrade_text), "Auto: %ld (Long press)", (long)app->upgrade_cost_auto);
        widget_add_string_multiline_element(
            app->widget, 64, 50, AlignCenter, AlignCenter, FontSecondary, upgrade_text);
    } else {
        widget_add_string_multiline_element(
            app->widget, 64, 50, AlignCenter, AlignCenter, FontSecondary, "Auto: ON");
    }
}

// Callback upgrade revenu passif
static void soclicker_upgrade_income_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    SoclickerApp* app = context;
    
    // Debug: vérifier que le callback est appelé
    char debug_text[64];
    snprintf(debug_text, sizeof(debug_text), "CALLBACK LEFT: %d %d", (int)button_type, (int)input_type);
    
    // Afficher le debug
    widget_add_string_multiline_element(
        app->widget, 64, 80, AlignCenter, AlignCenter, FontSecondary, debug_text);

    if(button_type == GuiButtonTypeLeft && input_type == InputTypeShort) {
        // Debug: afficher le coût et le compteur
        char debug_text[64];
        snprintf(debug_text, sizeof(debug_text), "UPGRADE: %ld >= %ld", (long)app->counter, (long)app->upgrade_cost_income);
        
        // Afficher le debug temporairement
        widget_add_string_multiline_element(
            app->widget, 64, 60, AlignCenter, AlignCenter, FontSecondary, debug_text);
        
        if(app->counter >= app->upgrade_cost_income) {
            app->counter -= app->upgrade_cost_income;
            app->passive_income++;
            app->upgrade_cost_income = app->upgrade_cost_income * 2; // Coût double
            soclicker_update_display(app);
            soclicker_save_data(app);
        } else {
            // Pas assez de points
            widget_add_string_multiline_element(
                app->widget, 64, 70, AlignCenter, AlignCenter, FontSecondary, "Pas assez!");
        }
    }
}

// Callback upgrade multiplicateur
static void soclicker_upgrade_mult_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    SoclickerApp* app = context;
    
    // Debug: vérifier que le callback est appelé
    char debug_text[64];
    snprintf(debug_text, sizeof(debug_text), "CALLBACK RIGHT: %d %d", (int)button_type, (int)input_type);

    if(button_type == GuiButtonTypeRight && input_type == InputTypeShort) {
        // Debug: afficher le coût et le compteur
        char debug_text[64];
        snprintf(debug_text, sizeof(debug_text), "UPGRADE x2: %ld >= %ld", (long)app->counter, (long)app->upgrade_cost_mult);
        
        // Afficher le debug temporairement
        widget_add_string_multiline_element(
            app->widget, 64, 60, AlignCenter, AlignCenter, FontSecondary, debug_text);
        
        if(app->counter >= app->upgrade_cost_mult) {
            app->counter -= app->upgrade_cost_mult;
            app->multiplier *= 2;
            app->upgrade_cost_mult = app->upgrade_cost_mult * 3; // Coût triple
            soclicker_update_display(app);
            soclicker_save_data(app);
        } else {
            // Pas assez de points
            widget_add_string_multiline_element(
                app->widget, 64, 70, AlignCenter, AlignCenter, FontSecondary, "Pas assez!");
        }
    }
}

// Callback upgrade auto-click (appelé par le bouton central long press)
static void soclicker_upgrade_auto_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    SoclickerApp* app = context;

    if(button_type == GuiButtonTypeCenter && input_type == InputTypeLong) {
        if(!app->auto_click_enabled && app->counter >= app->upgrade_cost_auto) {
            app->counter -= app->upgrade_cost_auto;
            app->auto_click_enabled = true;
            soclicker_update_display(app);
            soclicker_save_data(app);
        }
    }
}

// Callback pour les boutons
static void soclicker_button_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    SoclickerApp* app = context;

    if(button_type == GuiButtonTypeCenter) {
        if(input_type == InputTypeShort) {
            // Clic court = cliquer normalement
            int32_t click_value = 1 * app->multiplier; // Valeur de base * multiplicateur
            app->counter += click_value;
            
            // Debug: afficher la valeur du clic et le multiplicateur
            char debug_text[64];
            snprintf(debug_text, sizeof(debug_text), "Click: +%ld (x%ld)", (long)click_value, (long)app->multiplier);
            
            soclicker_update_display(app);
            soclicker_save_data(app);
        } else if(input_type == InputTypeLong) {
            // Clic long = acheter auto-click
            soclicker_upgrade_auto_callback(button_type, input_type, context);
        }
    }
}

// Constructeur de l'application
static SoclickerApp* soclicker_alloc() {
    SoclickerApp* app = malloc(sizeof(SoclickerApp));
    memset(app, 0, sizeof(SoclickerApp));

    // Initialiser le storage et le chemin de sauvegarde EN PREMIER
    app->storage = furi_record_open(RECORD_STORAGE);
    app->save_path = furi_string_alloc_set_str("/ext/apps_data/soclicker/counter.txt");

    // Créer le dossier si nécessaire
    soclicker_ensure_directory(app);

    // Charger les données sauvegardées
    soclicker_load_data(app);
    
    // Obtenir le GUI
    Gui* gui = furi_record_open(RECORD_GUI);

    // Créer le view dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    // Créer le widget
    app->widget = widget_alloc();
    
    // Démarrer le timer (toutes les 1 seconde)
    app->auto_timer = furi_timer_alloc(soclicker_timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->auto_timer, 500); // 1 seconde
    
    // Mettre à jour l'affichage initial
    soclicker_update_display(app);

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
    // Sauvegarder une dernière fois
    soclicker_save_data(app);
    
    // Arrêter et libérer le timer
    furi_timer_stop(app->auto_timer);
    furi_timer_free(app->auto_timer);
    
    view_dispatcher_remove_view(app->view_dispatcher, ViewIndexWidget);
    view_dispatcher_free(app->view_dispatcher);
    widget_free(app->widget);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(app->save_path);
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