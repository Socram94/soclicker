/**
 * @file soclicker.c
 * @author Socram94
 * @brief A simple clicker application for Flipper Zero.
 * @version 0.1
 * @date 30/08/2025
 */


#include <gui/gui.h>
#include <gui/view_dispatcher.h>

typedef enum {
    ViewIndexHome,
    ViewIndexShop,
    ViewIndexStats,
    ViewIndexSettings,
}

typedef struct {
    uint64_t count;
    ViewDispatcher* view_dispatcher;
} Soclicker;

static Soclicker* soclicker_alloc(){
    Soclicker* app = (Soclicker*)malloc(sizeof(Soclicker));
    memset(app, 0, sizeof(SocappViewDispatcherApp));
    app->count = 0;
    app->view_dispatcher = view_dispatcher_alloc();
    

    return app;
}

static void soclicker_free(Soclicker* app){
    

    free(app);
}

static void soclciker_run(Soclicker* app){
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIndexHome);
    view_dispatcher_run(app->view_dispatcher);
}

int32_t soclicker(void* arg){

    soclicker_alloc();
    soclciker_run();
    soclicker_free();
    return 0;
    
};