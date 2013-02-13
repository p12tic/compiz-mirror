/*
* Copyright © 2013 Sam Spilsbury
*
* Permission to use, copy, modify, distribute, and sell this software
* and its documentation for any purpose is hereby granted without
* fee, provided that the above copyright notice appear in all copies
* and that both that copyright notice and this permission notice
* appear in supporting documentation, and that the name of
* Novell, Inc. not be used in advertising or publicity pertaining to
* distribution of the software without specific, written prior permission.
* Novell, Inc. makes no representations about the suitability of this
* software for any purpose. It is provided "as is" without express or
* implied warranty.
*
* NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
* INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
* NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
* CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
* OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
* WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
* Author: Sam Spilsbury <smspillaz@gmail.com>
*/
#ifndef _COMPIZ_TESTHELPER_H
#define _COMPIZ_TESTHELPER_H

#include <X11/Xatom.h>

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include "testhelper_options.h"

class TestHelperScreen :
    public PluginClassHandler <TestHelperScreen, CompScreen>,
    public ScreenInterface,
    public TesthelperOptions
{
    public:

	TestHelperScreen (CompScreen *);

	void handleEvent (XEvent *event);

    private:

	CompScreen *screen;
};

class TestHelperWindow :
    public PluginClassHandler <TestHelperWindow, CompWindow>,
    public WindowInterface
{
    public:

	TestHelperWindow (CompWindow *);

	void handleEvent (XEvent *event);

    private:

	CompWindow *window;
};

class TestHelperPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <TestHelperScreen, TestHelperWindow>
{
    public:

	bool init ();
};

#endif
