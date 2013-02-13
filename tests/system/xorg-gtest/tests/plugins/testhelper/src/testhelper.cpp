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

#include "testhelper.h"

COMPIZ_PLUGIN_20090315 (testhelper, TestHelperPluginVTable)

void
TestHelperScreen::handleEvent (XEvent *event)
{
    screen->handleEvent (event);
}

TestHelperWindow::TestHelperWindow (CompWindow *w) :
    PluginClassHandler <TestHelperWindow, CompWindow> (w),
    window (w)
{
    WindowInterface::setHandler (w);
}

TestHelperScreen::TestHelperScreen (CompScreen *s) :
    PluginClassHandler <TestHelperScreen, CompScreen> (s),
    screen (s)
{
    ScreenInterface::setHandler (s);
}

bool
TestHelperPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
