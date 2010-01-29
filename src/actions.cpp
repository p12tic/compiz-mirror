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
    if (grabState == ScreenGrabSelect)
    {
        grabScreen (ScreenGrabNone);

        if (masterSelectionRect.x1 () != masterSelectionRect.x2 () &&
	    masterSelectionRect.y1 () != masterSelectionRect.y2 ())
        {
	    CompWindowList tempWindowList;
	    Selection	   sel = masterSelectionRect.toSelection ();

	    CompRegion reg (MIN (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) - 2,
			    MIN (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) - 2,
			    (MAX (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) -
	                     MIN (masterSelectionRect.x1 (), masterSelectionRect.x2 ()) + 4),
			    (MAX (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) -
	                     MIN (masterSelectionRect.y1 (), masterSelectionRect.y2 ()) + 4));

	    cScreen->damageRegion (reg);

	    masterSelection.push_back (sel);
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


bool
GroupScreen::groupWindows (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options)
{
    if (!masterSelection.empty ())
    {
	masterSelection.toGroup ();
    }

    return true;
}

/*
 * GroupScreen::ungroupWindows
 *
 */
bool
GroupScreen::ungroupWindows (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector options)
{
    CompWindow *w = screen->findTopLevelWindow (
			CompOption::getIntOptionNamed (options, "window", 0));
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    gw->group->destroy (false);
	}
    }

    return true;
}

/*
 * groupRemoveWindow
 *
 */
bool
GroupScreen::removeWindow (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options)
{
    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    gw->removeFromGroup ();
	}
    }

    return true;
}

/*
 * GroupScreen::closeWindows
 *
 */
bool
GroupScreen::closeWindows (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    int i;

	    foreach (CompWindow *w, gw->group->windows)
		w->close (screen->getCurrentTime ());
	}
    }

    return true;
}
#if 0
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
#endif

/*
 * GroupScreen::setIgnore
 *
 */
bool
GroupScreen::setIgnore (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options)
{
    ignoreMode = TRUE;

    if (state & CompAction::StateInitKey)
	action->setState (action->state () | CompAction::StateTermKey);

    return FALSE;
}

bool
GroupScreen::unsetIgnore (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options)
{
    ignoreMode = FALSE;

    action->setState (action->state () & ~CompAction::StateTermKey);

    return FALSE;
}


bool
GroupScreen::initTab (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    Bool       allowUntab = TRUE;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));
    if (!w)
	return true;

    GROUP_WINDOW (w);

    if (gw->inSelection)
    {
	groupWindows (action, state, options);
	/* If the window was selected, we don't want to
	   untab the group, because the user probably
	   wanted to tab the selected windows. */
	allowUntab = FALSE;
    }

    if (!gw->group)
	return TRUE;

    if (!gw->group->tabBar)
	gw->group->tab (w);
    else if (allowUntab)
	gw->group->untab ();

    cScreen->damageScreen ();

    return TRUE;
}

/*
 * GroupScreen::changeTabLeft
 *
 */
Bool
GroupScreen::changeTabLeft (CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector options)
{
    CompWindow *topTab;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    topTab = w;
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);

    if (!gw->tab || !gw->group)
	return TRUE;

    if (gw->group->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GroupWindow::get (topTab);
    
    std::list <Tab *> &tabs = gw->group->tabBar->tabs;
    std::list <Tab *>::iterator currentTab = std::find (tabs.begin (),
    							tabs.end (),
    							gw->group->topTab);

    /* FIXME: this is a bit of black magic */

    if (currentTab != tabs.end ()) // Did we find a tab?
    {        
        if (currentTab != tabs.begin ()) // Is this not the first
        {
            currentTab--;
	    return gw->group->tabBar->changeTab (*(currentTab),
					         TabBar::RotateLeft);
	}	
        else
            return gw->group->tabBar->changeTab (tabs.back (),
					         TabBar::RotateLeft);
    }

    return true;
}

/*
 * GroupScreen::changeTabRight
 *
 */
bool
GroupScreen::changeTabRight (CompAction         *action,
		     	     CompAction::State  state,
		     	     CompOption::Vector options)
{
    CompWindow *topTab;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    topTab = w;
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);

    if (!gw->tab || !gw->group || !gw->group->tabBar)
	return TRUE;

    if (gw->group->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GroupWindow::get (topTab);
    
    std::list <Tab *> &tabs = gw->group->tabBar->tabs;
    std::list <Tab *>::iterator currentTab = std::find (tabs.begin (),
    							tabs.end (),
    							gw->group->topTab);

    /* FIXME: this is a bit of black magic */

    if (currentTab != tabs.end ()) // Did we find a tab?
    {
        currentTab++;
        
        if (currentTab != tabs.end ()) // Is this not the last tab?
	    return gw->group->tabBar->changeTab (*(currentTab),
					         TabBar::RotateRight);
	
        else
            return gw->group->tabBar->changeTab (tabs.front (),
					     TabBar::RotateRight);
    }


    return true;
}
