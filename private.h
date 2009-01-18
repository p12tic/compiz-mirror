/*
 * Compiz text plugin
 * Description: Adds text to pixmap support to Compiz.
 *
 * private.h
 *
 * Copyright: (C) 2006-2007 Patrick Niklaus, Danny Baumann, Dennis Kasprzyk
 * Authors: Patrick Niklaus <marex@opencompsiting.org>
 *	    Danny Baumann   <maniac@opencompositing.org>
 *	    Dennis Kasprzyk <onestone@opencompositing.org>
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
 *
 */

#include <core/core.h>
#include <core/privatehandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <cairo-xlib-xrender.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <X11/Xatom.h>

#include "text.h"

class PrivateTextScreen :
    public PrivateHandler <PrivateTextScreen, CompScreen, COMPIZ_TEXT_ABI>,
    public ScreenInterface,
    public GLScreenInterface
{
    public:

	PrivateTextScreen (CompScreen *);
	~PrivateTextScreen ();

	Atom visibleNameAtom;
	Atom utf8StringAtom;
	Atom wmNameAtom;

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);	

	CompOption::Vector opt;	

        CompScreen *screen;

};

class TextHelper
{
    public:
	int                  width;
	int                  height;

	cairo_t              *cr;
	cairo_surface_t      *surface;
	PangoLayout          *layout;
	Pixmap               pixmap;
	XRenderPictFormat    *format;
	PangoFontDescription *font;
	Screen               *screen;
	CompScreen		 *s;

	bool 		 failed;

	bool render          (const CompText::Attrib &attrib,
			      CompString     &text);

	~TextHelper		();
	TextHelper		(CompScreen *s);
};

#define TEXT_SCREEN(screen)						      \
    PrivateTextScreen *ts = PrivateTextScreen::get (screen);

class TextPluginVTable :
    public CompPlugin::VTableForScreen <PrivateTextScreen>
{
    public:

	bool
	init ();
};
