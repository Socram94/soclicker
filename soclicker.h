#pragma once
#include <stdint.h>
#include <gui/view_dispatcher.h>
#include <gui/gui.h>
#include <gui/view.h>

// Les index de vues que ton app utilisera
typedef enum {
    ViewIndexHome,
    ViewIndexShop,
    ViewIndexStats,
    ViewIndexSettings,
} SoclickerViewIndex;

// Ta structure principale
typedef struct {
    uint64_t count;
    Gui* gui;                   // <--- Ajouté
    ViewDispatcher* view_dispatcher;
    View* home_view;            // <--- Ajouté
} Soclicker;

// Prototypes de fonctions de soclicker.c
Soclicker* soclicker_alloc(void);
void soclicker_free(Soclicker* app);
void soclicker_run(Soclicker* app);
