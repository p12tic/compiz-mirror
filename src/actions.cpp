#include "group.h"

/*
 * groupSelectSingle
 *
 */
bool
GroupScreen::selectSingle (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;

    fprintf (stderr, "select invoked\n");

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    /* Adds the window to the master selection */

    if (w)
	GroupWindow::get (w)->select ();

    return TRUE;
}

/*
 * groupSelect
 *
 */
bool
GroupScreen::select (CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	if (grabState == ScreenGrabNone)
	{
	    fprintf (stderr, "started selection\n");

	    grabScreen (ScreenGrabSelect);

	    if (state & CompAction::StateInitKey)
		action->setState (action->state () | CompAction::StateTermKey);

	    if (state & CompAction::StateInitButton)
		action->setState (action->state () | CompAction::StateTermButton);

	    masterSelectionRect.setX (pointerX);
	    masterSelectionRect.setY (pointerY);
	    masterSelectionRect.setWidth (0);
	    masterSelectionRect.setHeight (0);
	}

	return TRUE;
    }

    return FALSE;
}

/*
 * groupSelectTerminate
 *
 */
bool
GroupScreen::selectTerminate (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector options)
{
    fprintf (stderr, "terminate called\n");

    if (grabState == ScreenGrabSelect)
    {
	fprintf (stderr, "releasing grab and selectionofying\n");

        grabScreen (ScreenGrabNone);

        if (masterSelectionRect.x1 () != masterSelectionRect.x2 () &&
	    masterSelectionRect.y1 () != masterSelectionRect.y2 ())
        {
	    CompWindowList tempWindowList;

	    CompRegion reg (MIN (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) - 2,
			    MIN (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) - 2,
			    (MAX (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) -
	                     MIN (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) + 4),
			    (MAX (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) -
	                     MIN (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) + 4));

	    cScreen->damageRegion (reg);

	    masterSelection.push_back (masterSelectionRect.toSelection ());
#if 0
		
	        ws = findWindowsInRegion (reg)
	        if (!ws.empty ())
	        {
		    /* select windows */
		    for (int i = 0; i < count; i++)
		        GroupWindow::get (groupSelectWindow (ws[i]);

		    if (optionGetAutoGroup ())
		        selection->toGroup ();

		    free (ws);
	        }
#endif
        }
    }


    action->setState (action->state () & ~(CompAction::StateTermKey |
					   CompAction::StateTermButton));

    return true;
}

#if 0
/*
 * groupGroupWindows
 *
 */
bool
groupGroupWindows (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	GROUP_SCREEN (s);

	if (gs->tmpSel.nWins > 0)
	{
	    int            i;
	    CompWindow     *cw;
	    GroupSelection *group = NULL;
	    Bool           tabbed = FALSE;

	    for (i = 0; i < gs->tmpSel.nWins; i++)
	    {
		cw = gs->tmpSel.windows[i];
		GROUP_WINDOW (cw);

		if (gw->group)
		{
		    if (!tabbed || group->tabBar)
			group = gw->group;

		    if (group->tabBar)
			tabbed = TRUE;
		}
	    }

	    /* we need to do one first to get the pointer of a new group */
	    cw = gs->tmpSel.windows[0];
	    GROUP_WINDOW (cw);

	    if (gw->group && (group != gw->group))
		groupDeleteGroupWindow (cw);
	    groupAddWindowToGroup (cw, group, 0);
	    addWindowDamage (cw);

	    gw->inSelection = FALSE;
	    group = gw->group;

	    for (i = 1; i < gs->tmpSel.nWins; i++)
	    {
		cw = gs->tmpSel.windows[i];
		GROUP_WINDOW (cw);

		if (gw->group && (group != gw->group))
		    groupDeleteGroupWindow (cw);
		groupAddWindowToGroup (cw, group, 0);
		addWindowDamage (cw);

		gw->inSelection = FALSE;
	    }

	    /* exit selection */
	    free (gs->tmpSel.windows);
	    gs->tmpSel.windows = NULL;
	    gs->tmpSel.nWins = 0;
	}
    }

    return FALSE;
}

/*
 * groupUnGroupWindows
 *
 */
Bool
groupUnGroupWindows (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findTopLevelWindowAtDisplay (d, xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	    groupDeleteGroup (gw->group);
    }

    return FALSE;
}

/*
 * groupRemoveWindow
 *
 */
Bool
groupRemoveWindow (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	    groupRemoveWindowFromGroup (w);
    }

    return FALSE;
}

/*
 * groupCloseWindows
 *
 */
Bool
groupCloseWindows (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    int i;

	    for (i = 0; i < gw->group->nWins; i++)
		closeWindow (gw->group->windows[i],
			     getCurrentTimeFromDisplay (d));
	}
    }

    return FALSE;
}

/*
 * groupChangeColor
 *
 */
Bool
groupChangeColor (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    GLushort *color = gw->group->color;
	    float    factor = ((float)RAND_MAX + 1) / 0xffff;

	    color[0] = (int)(rand () / factor);
	    color[1] = (int)(rand () / factor);
	    color[2] = (int)(rand () / factor);

	    groupRenderTopTabHighlight (gw->group);
	    damageScreen (w->screen);
	}
    }

    return FALSE;
}

/*
 * groupSetIgnore
 *
 */
Bool
groupSetIgnore (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    GROUP_DISPLAY (d);

    gd->ignoreMode = TRUE;

    if (state & CompActionStateInitKey)
	action->state |= CompActionStateTermKey;

    return FALSE;
}

/*
 * groupUnsetIgnore
 *
 */
Bool
groupUnsetIgnore (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int             nOption)
{
    GROUP_DISPLAY (d);

    gd->ignoreMode = FALSE;

    action->state &= ~CompActionStateTermKey;

    return FALSE;
}


/*
 * groupInitTab
 *
 */
Bool
groupInitTab (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    Window     xid;
    CompWindow *w;
    Bool       allowUntab = TRUE;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);

    if (gw->inSelection)
    {
	groupGroupWindows (d, action, state, option, nOption);
	/* If the window was selected, we don't want to
	   untab the group, because the user probably
	   wanted to tab the selected windows. */
	allowUntab = FALSE;
    }

    if (!gw->group)
	return TRUE;

    if (!gw->group->tabBar)
	groupTabGroup (w);
    else if (allowUntab)
	groupUntabGroup (gw->group);

    damageScreen (w->screen);

    return TRUE;
}

/*
 * groupChangeTabLeft
 *
 */
Bool
groupChangeTabLeft (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    Window     xid;
    CompWindow *w, *topTab;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = topTab = findWindowAtDisplay (d, xid);
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);
    GROUP_SCREEN (w->screen);

    if (!gw->slot || !gw->group)
	return TRUE;

    if (gw->group->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GET_GROUP_WINDOW (topTab, gs);

    if (gw->slot->prev)
	return groupChangeTab (gw->slot->prev, RotateLeft);
    else
	return groupChangeTab (gw->group->tabBar->revSlots, RotateLeft);
}

/*
 * groupChangeTabRight
 *
 */
Bool
groupChangeTabRight (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    Window     xid;
    CompWindow *w, *topTab;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = topTab = findWindowAtDisplay (d, xid);
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);
    GROUP_SCREEN (w->screen);

    if (!gw->slot || !gw->group)
	return TRUE;

    if (gw->group->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GET_GROUP_WINDOW (topTab, gs);

    if (gw->slot->next)
	return groupChangeTab (gw->slot->next, RotateRight);
    else
	return groupChangeTab (gw->group->tabBar->slots, RotateRight);
}
#endif
