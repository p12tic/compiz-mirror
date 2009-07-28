/*
 * Compiz autoresize plugin
 * Author : Sam "SmSpillaz" Spilsbury
 * Email  : smspillaz@gmail.com
 *
 * Copyright (C) 2009 Sam Spilsbury
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>

class AutoresizeScreen :
    public ScreenInterface,
    public PluginClassHandler <AutoresizeScreen, CompScreen>
{
    public:
	AutoresizeScreen (CompScreen *s);

	void resizeAllWindows ();

	void handleEvent (XEvent *event);

    private:
	int mWidth;
	int mHeight;
};

#define AUTORESIZE_SCREEN(s) \
    AutoresizeScreen *as = AutoresizeScreen::get (s)

#define AUTORESIZE_WINDOW(w) \
    AutoresizeWindow *aw = AutoresizeWindow::get (w)

class AutoresizePluginVTable :
    public CompPlugin::VTableForScreen <AutoresizeScreen>
{
    public:
	bool init ();
};

