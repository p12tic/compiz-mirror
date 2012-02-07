#include "test-local-menu.h"

#define GLOBAL 0
#define LOCAL 1

TEST_F (GtkWindowDecoratorTestLocalMenu, TestOn)
{
    g_settings_set_enum (getSettings (), "menu-mode", LOCAL);
    gboolean result = gwd_window_should_have_local_menu (getWindow ());

    EXPECT_TRUE (result);
}

TEST_F (GtkWindowDecoratorTestLocalMenu, TestOff)
{
    g_settings_set_enum (getSettings (), "menu-mode", GLOBAL);
    gboolean result = gwd_window_should_have_local_menu (getWindow ());

    EXPECT_FALSE (result);
}
