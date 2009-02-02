/*
 *
 * Compiz mouse position polling plugin
 *
 * mousepoll.c
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <core/core.h>
#include <core/privatehandler.h>
#include <core/timer.h>

#include "mousepoll.h"

typedef enum _MousepollOptions
{
    MP_DISPLAY_OPTION_MOUSE_POLL_INTERVAL,
    MP_DISPLAY_OPTION_NUM
} MousepollDisplayOptions;

#define MP_OPTION_MOUSE_POLL_INTERVAL 0
#define MP_OPTION_NUM 1

class MousepollScreen :
    public PrivateHandler <MousepollScreen, CompScreen, COMPIZ_MOUSEPOLL_ABI>
{
    public:

	CompOption::Vector opt;

	MousepollScreen (CompScreen *screen);

	std::list<MousePoller *> pollers;
	CompTimer		 timer;

	int posX;
	int posY;

	bool
	updatePosition ();

	bool
	getMousePosition ();

	bool
	addTimer (MousePoller *poller);

	void
	removeTimer (MousePoller *poller);

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

};

#define MOUSEPOLL_SCREEN(s)						        \
    MousepollScreen *ms = MousepollScreen::get (s)

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

class MousepollPluginVTable :
    public CompPlugin::VTableForScreen<MousepollScreen>
{
    public:

	bool init ();

	PLUGIN_OPTION_HELPER (MousepollScreen);

};

