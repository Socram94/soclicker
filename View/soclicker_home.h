#pragma once
#include <gui/view.h>
#include "../soclicker.h"

// Allocation / libération de la vue Home
View* soclicker_home_alloc(Soclicker* app);
void soclicker_home_free(View* home_view);