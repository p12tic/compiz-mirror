#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "local-menus.h"
#include <X11/Xlib.h>
#include <gio/gio.h>

class GtkWindowDecoratorTestLocalMenu :
    public ::testing::Test
{
    public:

	WnckWindow * getWindow () { return mWindow; }
	GSettings  * getSettings () { return mSettings; }

	virtual void SetUp ()
	{
	    gtk_init (NULL, NULL);

	    mXDisplay = XOpenDisplay (NULL);
	    mXWindow = XCreateSimpleWindow (mXDisplay, DefaultRootWindow (mXDisplay), 0, 0, 100, 100, 0, 0, 0);

	    XMapRaised (mXDisplay, mXWindow);

	    XFlush (mXDisplay);

	    while (g_main_context_iteration (g_main_context_default (), FALSE));

	    mWindow = wnck_window_get (mXWindow);

	    g_setenv("GSETTINGS_BACKEND", "memory", true);
	    mSettings = g_settings_new ("com.canonical.indicator.appmenu");
	}

	virtual void TearDown ()
	{
	    XDestroyWindow (mXDisplay, mXWindow);
	    XCloseDisplay (mXDisplay);

	    g_object_unref (mSettings);
	}

    private:

	WnckWindow *mWindow;
	Window     mXWindow;
	Display    *mXDisplay;
	GSettings *mSettings;
};
