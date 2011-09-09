/*
 * Copyright © 2011 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "privatestackdebugger.h"
#include "privatewindow.h"

namespace
{
    StackDebugger * gStackDebugger = NULL;
}

StackDebugger *
StackDebugger::Default ()
{
    return gStackDebugger;
}

void
StackDebugger::SetDefault (StackDebugger *dbg)
{
    if (gStackDebugger)
	delete gStackDebugger;

    gStackDebugger = dbg;
}

StackDebugger::StackDebugger (Display *dpy, Window root) :
    mServerNChildren (0),
    mServerChildren (NULL),
    mHandledConfigureEvent (false),
    mRoot (root),
    mDpy (dpy)
{
}

StackDebugger::~StackDebugger ()
{
    if (mServerChildren)
    {
	XFree (mServerChildren);
	mServerChildren = NULL;
	mServerNChildren = 0;
    }
}

void
StackDebugger::loadStack ()
{
    Window rootRet, parentRet;

    if (mServerChildren)
	XFree (mServerChildren);

    XQueryTree (mDpy, mRoot, &rootRet, &parentRet,
		&mServerChildren, &mServerNChildren);

    mHandledConfigureEvent = false;
    mDestroyedFrames.clear ();
}

void
StackDebugger::handleEvent (XEvent *event)
{
    switch (event->type)
    {
	case ConfigureNotify:
	    if (event->xconfigure.above)
		mHandledConfigureEvent = true;
	    break;
	default:
	    break;
    }
}

void
StackDebugger::addDestroyedFrame (Window f)
{
    mDestroyedFrames.push_back (f);
}

bool
StackDebugger::stackChange ()
{
    return mHandledConfigureEvent;
}

bool
StackDebugger::cmpStack (CompWindowList &windows,
			 CompWindowList &serverWindows,
			 bool           verbose)
{
    std::vector <Window>             serverSideWindows;
    CompWindowList::reverse_iterator lrrit = windows.rbegin ();
    CompWindowList::reverse_iterator lsrit = serverWindows.rbegin ();
    unsigned int                     i = 0;
    bool                             err = false;

    for (unsigned int n = 0; n < mServerNChildren; n++)
    {
	if (std::find (mDestroyedFrames.begin (),
		       mDestroyedFrames.end (), mServerChildren[n])
		== mDestroyedFrames.end ())
	    serverSideWindows.push_back (mServerChildren[n]);
    }

    if (verbose)
	fprintf (stderr, "sent       | recv       | server     |\n\n");

    for (;(lrrit != windows.rend () ||
	   lsrit != serverWindows.rend () ||
	   i != serverSideWindows.size ());)
    {
	Window lrXid = 0;
	Window lsXid = 0;
	Window sXid = 0;

	if (lrrit != windows.rend ())
	    lrXid = (*lrrit)->frame () ? (*lrrit)->priv->frame : (*lrrit)->id ();

	if (lsrit != serverWindows.rend ())
	    lsXid = (*lsrit)->frame () ? (*lsrit)->priv->frame : (*lsrit)->id ();

	if (i != serverSideWindows.size ())
	    sXid = serverSideWindows[serverSideWindows.size () - (i + 1)];

	if (verbose)
	    fprintf (stderr, "id 0x%x id 0x%x id 0x%x\n",
		     (unsigned int) lsXid, (unsigned int) lrXid,
		     (unsigned int) sXid);

	if (lrXid != sXid)
	{
	    if (verbose)
		compLogMessage ("core", CompLogLevelDebug, "stacking error - "\
				"0x%x does not equal 0x%x!\n", (unsigned int) lrXid,
				(unsigned int) sXid);
	    err = true;
	}

	if (lrrit != windows.rend ())
	    lrrit++;

	if (lsrit != serverWindows.rend())
	    lsrit++;

	if (i != serverSideWindows.size ())
	    i++;
    }

    return err;
}
