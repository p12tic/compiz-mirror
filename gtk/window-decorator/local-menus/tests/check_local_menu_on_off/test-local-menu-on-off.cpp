#include "test-local-menu.h"

TEST_F (GtkWindowDecoratorTestLocalMenu, TestOn)
{
    g_settings_set_boolean (getSettings (), "force-local-menus", TRUE);
    gboolean result = gwd_window_should_have_local_menu (getWindow ());

    EXPECT_TRUE (result);
}

TEST_F (GtkWindowDecoratorTestLocalMenu, TestOff)
{
    g_settings_set_boolean (getSettings (), "force-local-menus", FALSE);
    gboolean result = gwd_window_should_have_local_menu (getWindow ());

    EXPECT_FALSE (result);
}
