/**
 *
 * Compiz group plugin
 *
 * group.c
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
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
 **/

#include "group.h"

/*
 * groupIsGroupWindow
 *
 */

bool
GroupWindow::groupIsGroupWindow ()
{
    if (window->overrideRedirect ())
	return FALSE;

    if (window->type () & CompWindowTypeDesktopMask)
	return FALSE;

    if (window->invisible ())
	return FALSE;

    if (!GroupScreen::get (screen)->optionGetWindowMatch ().evaluate (window))
	return FALSE;

    return TRUE;
}

/*
 * groupDragHoverTimeout
 *
 * Description:
 * Activates a window after a certain time a slot has been dragged over it.
 *
 */
bool
GroupWindow::groupDragHoverTimeout ()
{
    GROUP_SCREEN (screen);

    if (gs->optionGetBarAnimations ())
    {
	GroupTabBar *bar = mGroup->mTabBar;

	bar->mBgAnimation = AnimationPulse;
	bar->mBgAnimationTime = gs->optionGetPulseTime () * 1000;
    }

    window->activate ();

    return FALSE;
}

/*
 * groupCheckWindowProperty
 *
 */
bool
GroupWindow::groupCheckWindowProperty (CompWindow *w,
				       long int   *id,
				       Bool       *tabbed,
				       GLushort   *color)
{
    Atom          type;
    int           retval, fmt;
    unsigned long nitems, exbyte;
    long int      *data;

    GROUP_SCREEN (screen);

    retval = XGetWindowProperty (screen->dpy (), window->id (),
	     			 gs->mGroupWinPropertyAtom, 0, 5, False,
	     			 XA_CARDINAL, &type, &fmt, &nitems, &exbyte,
	     			 (unsigned char **)&data);

    if (retval == Success)
    {
	if (type == XA_CARDINAL && fmt == 32 && nitems == 5)
	{
	    if (id)
		*id = data[0];
	    if (tabbed)
		*tabbed = (bool) data[1];
	    if (color) {
		color[0] = (GLushort) data[2];
		color[1] = (GLushort) data[3];
		color[2] = (GLushort) data[4];
	    }

	    XFree (data);
	    return TRUE;
	}
	else if (fmt != 0)
	    XFree (data);
    }

    return FALSE;
}

/*
 * groupUpdateWindowProperty
 *
 */
void
GroupWindow::groupUpdateWindowProperty ()
{
    GROUP_SCREEN (screen);

    // Do not change anything in this case
    if (mReadOnlyProperty)
	return;

    if (mGroup)
    {
	long int buffer[5];

	buffer[0] = mGroup->mIdentifier;
	buffer[1] = (mSlot) ? TRUE : FALSE;

	/* group color RGB */
	buffer[2] = mGroup->mColor[0];
	buffer[3] = mGroup->mColor[1];
	buffer[4] = mGroup->mColor[2];

	XChangeProperty (screen->dpy (), window->id (), gs->mGroupWinPropertyAtom,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) buffer, 5);
    }
    else
    {
	XDeleteProperty (screen->dpy (), window->id (), gs->mGroupWinPropertyAtom);
    }
}

unsigned int
GroupWindow::groupUpdateResizeRectangle (XRectangle *masterGeometry,
					 bool	    damage)
{
    XRectangle   newGeometry;
    unsigned int mask = 0;
    int          newWidth, newHeight;
    int          widthDiff, heightDiff;

    GROUP_SCREEN (screen);

    if (!mResizeGeometry || !gs->mResizeInfo)
	return 0;

    newGeometry.x = WIN_X (window) + (masterGeometry->x -
				 gs->mResizeInfo->mOrigGeometry.x);
    newGeometry.y = WIN_Y (window) + (masterGeometry->y -
				 gs->mResizeInfo->mOrigGeometry.y);

    widthDiff = masterGeometry->width - gs->mResizeInfo->mOrigGeometry.width;
    newGeometry.width = MAX (1, WIN_WIDTH (window) + widthDiff);
    heightDiff = masterGeometry->height - gs->mResizeInfo->mOrigGeometry.height;
    newGeometry.height = MAX (1, WIN_HEIGHT (window) + heightDiff);

    if (window->constrainNewWindowSize (newGeometry.width, newGeometry.height,
					&newWidth, &newHeight))
    {
	newGeometry.width  = newWidth;
	newGeometry.height = newHeight;
    }

    if (damage)
    {
	if (memcmp (&newGeometry, mResizeGeometry,
		    sizeof (newGeometry)) != 0)
	{
	    cWindow->addDamage ();
	}
    }

    if (newGeometry.x != mResizeGeometry->x)
    {
	mResizeGeometry->x = newGeometry.x;
	mask |= CWX;
    }
    if (newGeometry.y != mResizeGeometry->y)
    {
	mResizeGeometry->y = newGeometry.y;
	mask |= CWY;
    }
    if (newGeometry.width != mResizeGeometry->width)
    {
	mResizeGeometry->width = newGeometry.width;
	mask |= CWWidth;
    }
    if (newGeometry.height != mResizeGeometry->height)
    {
	mResizeGeometry->height = newGeometry.height;
	mask |= CWHeight;
    }

    return mask;
}

/*
 * groupGrabScreen
 *
 */
void
GroupScreen::groupGrabScreen (GroupScreenGrabState newState)
{
    if ((mGrabState != newState) && mGrabIndex)
    {
	screen->removeGrab (mGrabIndex, NULL);
	mGrabIndex = 0;
    }

    if (newState == ScreenGrabSelect)
    {
	mGrabIndex = screen->pushGrab (None, "group");
    }
    else if (newState == ScreenGrabTabDrag)
    {
	mGrabIndex = screen->pushGrab (None, "group-drag");
    }

    mGrabState = newState;
}

/*
 * groupRaiseWindows
 *
 */
void
GroupScreen::groupRaiseWindows (CompWindow     *top,
				GroupSelection *group)
{
    CompWindow **stack;
    int        count = 0, i;

    if (group->mNWins == 1)
	return;

    stack = (CompWindow **) malloc ((group->mNWins - 1) * sizeof (CompWindow *));
    if (!stack)
	return;

    foreach (CompWindow *w, screen->windows ())
    {
	GROUP_WINDOW (w);
	if ((w->id () != top->id ()) && (gw->mGroup == group))
	    stack[count++] = w;
    }

    for (i = 0; i < count; i++)
	stack[i]->restackBelow (top);

    free (stack);
}

/*
 * groupMinimizeWindows
 *
 */
void
GroupScreen::groupMinimizeWindows (CompWindow     *top,
				   GroupSelection *group,
				   Bool           minimize)
{
    int i;
    for (i = 0; i < group->mNWins; i++)
    {
	CompWindow *w = group->mWindows[i];
	if (w->id () == top->id ())
	    continue;

	if (minimize)
	    w->minimize ();
	else
	    w->unminimize ();
    }
}

/*
 * groupShadeWindows
 *
 */
void
GroupScreen::groupShadeWindows (CompWindow     *top,
				GroupSelection *group,
				Bool           shade)
{
    int i;
    unsigned int state;

    for (i = 0; i < group->mNWins; i++)
    {
	CompWindow *w = group->mWindows[i];
	if (w->id () == top->id ())
	    continue;

	if (shade)
	    state = w->state () | CompWindowStateShadedMask;
	else
	    state = w->state () & ~CompWindowStateShadedMask;

	w->changeState (state);
	w->updateAttributes (CompStackingUpdateModeNone);
    }
}

/*
 * groupDeleteGroupWindow
 *
 */
void
GroupWindow::groupDeleteGroupWindow ()
{
    GroupSelection *group;

    GROUP_SCREEN (screen);

    if (!mGroup)
	return;

    group = mGroup;

    if (group->mTabBar && mSlot)
    {
	if (gs->mDraggedSlot && gs->mDragged &&
	    gs->mDraggedSlot->mWindow->id () == window->id ())
	{
	    gs->groupUnhookTabBarSlot (group->mTabBar, mSlot, FALSE);
	}
	else
	    gs->groupDeleteTabBarSlot (group->mTabBar, mSlot);
    }

    if (group->mNWins && group->mWindows)
    {
	CompWindow **buf = group->mWindows;

	if (group->mNWins > 1)
	{
	    int counter = 0;
	    int i;

	    group->mWindows = (CompWindow ** ) calloc (group->mNWins - 1, sizeof(CompWindow *));

	    for (i = 0; i < group->mNWins; i++)
	    {
		if (buf[i]->id () == window->id ())
		    continue;
		group->mWindows[counter++] = buf[i];
	    }
	    group->mNWins = counter;

	    if (group->mNWins == 1)
	    {
		/* Glow was removed from this window, too */
		CompositeWindow::get (group->mWindows[0])->damageOutputExtents ();
		group->mWindows[0]->updateWindowOutputExtents ();

		if (gs->optionGetAutoUngroup ())
		{
		    if (group->mChangeState != NoTabChange)
		    {
			/* a change animation is pending: this most
			   likely means that a window must be moved
			   back onscreen, so we do that here */
			CompWindow *lw = group->mWindows[0];

			GroupWindow::get (lw)->groupSetWindowVisibility (true);
		    }
		    if (!gs->optionGetAutotabCreate ())
			gs->groupDeleteGroup (group);
		}
	    }
	}
	else
	{
	    group->mWindows = NULL;
	    gs->groupDeleteGroup (group);
	}

	free (buf);

	cWindow->damageOutputExtents ();
	mGroup = NULL;
	window->updateWindowOutputExtents ();
	groupUpdateWindowProperty ();
    }
}

void
GroupWindow::groupRemoveWindowFromGroup ()
{
    GROUP_SCREEN (screen);

    if (!mGroup)
	return;

    if (mGroup->mTabBar && !(mAnimateState & IS_UNGROUPING) &&
	(mGroup->mNWins > 1))
    {
	GroupSelection *group = mGroup;

	/* if the group is tabbed, setup untabbing animation. The
	   window will be deleted from the group at the
	   end of the untabbing. */
	if (HAS_TOP_WIN (group))
	{
	    CompWindow *tw = TOP_TAB (group);
	    int        oldX = mOrgPos.x;
	    int        oldY = mOrgPos.y;

	    mOrgPos.x = WIN_CENTER_X (tw) - (WIN_WIDTH (window) / 2);
	    mOrgPos.y = WIN_CENTER_Y (tw) - (WIN_HEIGHT (window) / 2);

	    mDestination.x = mOrgPos.x + mMainTabOffset.x;
	    mDestination.y = mOrgPos.y + mMainTabOffset.y;

	    mMainTabOffset.x = oldX;
	    mMainTabOffset.y = oldY;

	    if (mTx || mTy)
	    {
		mTx -= (mOrgPos.x - oldX);
		mTy -= (mOrgPos.y - oldY);
	    }

	    mAnimateState = IS_ANIMATED;
	    mXVelocity = mYVelocity = 0.0f;
	}

	/* Although when there is no top-tab, it will never really
	   animate anything, if we don't start the animation,
	   the window will never get removed. */
	gs->groupStartTabbingAnimation (group, FALSE);

	groupSetWindowVisibility (TRUE);
	group->mUngroupState = UngroupSingle;
	mAnimateState |= IS_UNGROUPING;
    }
    else
    {
	/* no tab bar - delete immediately */
	groupDeleteGroupWindow ();

	if (gs->optionGetAutotabCreate () && groupIsGroupWindow ())
	{
	    groupAddWindowToGroup (NULL, 0);
	    gs->groupTabGroup (window);
	}
    }
}

/*
 * groupDeleteGroup
 *
 */
void
GroupScreen::groupDeleteGroup (GroupSelection *group)
{
    GroupSelection *next, *prev;

    if (group->mWindows)
    {
	int i;

	if (group->mTabBar)
	{
	    /* set up untabbing animation and delete the group
	       at the end of the animation */
	    groupUntabGroup (group);
	    group->mUngroupState = UngroupAll;
	    return;
	}

	for (i = 0; i < group->mNWins; i++)
	{
	    CompWindow *cw = group->mWindows[i];
	    GROUP_WINDOW (cw);

	    CompositeWindow::get (cw)->damageOutputExtents ();
	    gw->mGroup = NULL;
	    cw->updateWindowOutputExtents ();
	    gw->groupUpdateWindowProperty ();

	    if (optionGetAutotabCreate () && gw->groupIsGroupWindow ())
	    {
		gw->groupAddWindowToGroup (NULL, 0);
		groupTabGroup (cw);
	    }
	}

	free (group->mWindows);
	group->mWindows = NULL;
    }
    else if (group->mTabBar)
	groupDeleteTabBar (group);

    prev = group->mPrev;
    next = group->mNext;

    /* relink stack */
    if (prev || next)
    {
	if (prev)
	{
	    if (next)
		prev->mNext = next;
	    else
		prev->mNext = NULL;
	}
	if (next)
	{
	    if (prev)
		next->mPrev = prev;
	    else
	    {
		next->mPrev = NULL;
		mGroups = next;
	    }
	}
    }
    else
	mGroups = NULL;

    if (group == mLastHoveredGroup)
	mLastHoveredGroup = NULL;
    if (group == mLastRestackedGroup)
	mLastRestackedGroup = NULL;

    free (group);
}

/*
 * groupAddWindowToGroup
 *
 */
void
GroupWindow::groupAddWindowToGroup (GroupSelection *group,
				    long int       initialIdent)
{
    GROUP_SCREEN (screen);

    if (mGroup)
	return;

    if (group)
    {
	CompWindow *topTab = NULL;

	group->mWindows = (CompWindow **) realloc (group->mWindows,
				  sizeof (CompWindow *) * (group->mNWins + 1));
	group->mWindows[group->mNWins] = window;
	group->mNWins++;
	mGroup = group;

	window->updateWindowOutputExtents ();
	groupUpdateWindowProperty ();

	if (group->mNWins == 2)
	{
	    /* first window in the group got its glow, too */
	    group->mWindows[0]->updateWindowOutputExtents ();
	}

	if (group->mTabBar)
	{
	    if (HAS_TOP_WIN (group))
		topTab = TOP_TAB (group);
	    else if (HAS_PREV_TOP_WIN (group))
	    {
		topTab = PREV_TOP_TAB (group);
		group->mTopTab = group->mPrevTopTab;
		group->mPrevTopTab = NULL;
	    }

	    if (topTab)
	    {
		if (!mSlot)
		    gs->groupCreateSlot (group, window);

		mDestination.x = WIN_CENTER_X (topTab) - (WIN_WIDTH (window) / 2);
		mDestination.y = WIN_CENTER_Y (topTab) -
		                    (WIN_HEIGHT (window) / 2);
		mMainTabOffset.x = WIN_X (window) - mDestination.x;
		mMainTabOffset.y = WIN_Y (window) - mDestination.y;
		mOrgPos.x = WIN_X (window);
		mOrgPos.y = WIN_Y (window);

		mXVelocity = mYVelocity = 0.0f;

		mAnimateState = IS_ANIMATED;

		gs->groupStartTabbingAnimation (group, TRUE);

		cWindow->addDamage ();
	    }
	}
    }
    else
    {
	/* create new group */
	GroupSelection *g = (GroupSelection *) malloc (sizeof (GroupSelection));
	if (!g)
	    return;

	g->mWindows = (CompWindow **) malloc (sizeof (CompWindow *));
	if (!g->mWindows)
	{
	    free (g);
	    return;
	}

	g->mWindows[0] = window;
	g->mScreen     = screen;
	g->mNWins      = 1;

	g->mTopTab      = NULL;
	g->mPrevTopTab  = NULL;
	g->mNextTopTab  = NULL;

	g->mChangeAnimationTime      = 0;
	g->mChangeAnimationDirection = 0;

	g->mChangeState  = NoTabChange;
	g->mTabbingState = NoTabbing;
	g->mUngroupState = UngroupNone;

	g->mTabBar = NULL;

	g->mCheckFocusAfterTabChange = FALSE;

	g->mGrabWindow = None;
	g->mGrabMask   = 0;

	g->mInputPrevention = None;
	g->mIpwMapped       = FALSE;

	/* glow color */
	g->mColor[0] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
	g->mColor[1] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
	g->mColor[2] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
	g->mColor[3] = 0xffff;

	if (initialIdent)
	    g->mIdentifier = initialIdent;
	else
	{
	    /* we got no valid group Id passed, so find out a new valid
	       unique one */
	    GroupSelection *tg;
	    Bool           invalidID = FALSE;

	    g->mIdentifier = gs->mGroups ? gs->mGroups->mIdentifier : 0;
	    do
	    {
		invalidID = FALSE;
		for (tg = gs->mGroups; tg; tg = tg->mNext)
		{
		    if (tg->mIdentifier == g->mIdentifier)
		    {
			invalidID = TRUE;

			g->mIdentifier++;
			break;
		    }
		}
	    }
	    while (invalidID);
	}

	/* relink stack */
	if (gs->mGroups)
	    gs->mGroups->mPrev = g;

	g->mNext = gs->mGroups;
	g->mPrev = NULL;
	gs->mGroups = g;

	mGroup = g;

	groupUpdateWindowProperty ();
    }
}

/*
 * groupGroupWindows
 *
 */
bool
GroupScreen::groupGroupWindows (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector options)
{
    if (mTmpSel.mNWins > 0)
    {
        int            i;
        CompWindow     *cw;
        GroupSelection *group = NULL;
        Bool           tabbed = FALSE;

        for (i = 0; i < mTmpSel.mNWins; i++)
        {
	    cw = mTmpSel.mWindows[i];
	    GROUP_WINDOW (cw);

	    if (gw->mGroup)
	    {
	        if (!tabbed || group->mTabBar)
		    group = gw->mGroup;

	        if (group->mTabBar)
		    tabbed = TRUE;
	    }
        }

        /* we need to do one first to get the pointer of a new group */
        cw = mTmpSel.mWindows[0];
        GROUP_WINDOW (cw);

        if (gw->mGroup && (group != gw->mGroup))
	    gw->groupDeleteGroupWindow ();
        gw->groupAddWindowToGroup (group, 0);
        gw->cWindow->addDamage ();

        gw->mInSelection = FALSE;
        group = gw->mGroup;

        for (i = 1; i < mTmpSel.mNWins; i++)
        {
	    cw = mTmpSel.mWindows[i];
	    GROUP_WINDOW (cw);

	    if (gw->mGroup && (group != gw->mGroup))
	        gw->groupDeleteGroupWindow ();
	    gw->groupAddWindowToGroup (group, 0);
	    gw->cWindow->addDamage ();

	    gw->mInSelection = FALSE;
        }

        /* exit selection */
        free (mTmpSel.mWindows);
        mTmpSel.mWindows = NULL;
        mTmpSel.mNWins = 0;
    }

    return FALSE;
}

/*
 * groupUnGroupWindows
 *
 */
bool
GroupScreen::groupUnGroupWindows (CompAction          *action,
				  CompAction::State   state,
				  CompOption::Vector  options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->mGroup)
	    groupDeleteGroup (gw->mGroup);
    }

    return FALSE;
}

/*
 * groupRemoveWindow
 *
 */
bool
GroupScreen::groupRemoveWindow (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->mGroup)
	    gw->groupRemoveWindowFromGroup ();
    }

    return FALSE;
}

/*
 * groupCloseWindows
 *
 */
bool
GroupScreen::groupCloseWindows (CompAction           *action,
				CompAction::State    state,
				CompOption::Vector   options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->mGroup)
	{
	    int i;

	    for (i = 0; i < gw->mGroup->mNWins; i++)
		gw->mGroup->mWindows[i]->close (screen->getCurrentTime ());
	}
    }

    return FALSE;
}

/*
 * groupChangeColor
 *
 */
bool
GroupScreen::groupChangeColor (CompAction           *action,
			       CompAction::State    state,
			       CompOption::Vector   options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->mGroup)
	{
	    GLushort *color = gw->mGroup->mColor;
	    float    factor = ((float)RAND_MAX + 1) / 0xffff;

	    color[0] = (int)(rand () / factor);
	    color[1] = (int)(rand () / factor);
	    color[2] = (int)(rand () / factor);

	    groupRenderTopTabHighlight (gw->mGroup);
	    cScreen->damageScreen ();
	}
    }

    return FALSE;
}

/*
 * groupSetIgnore
 *
 */
bool
GroupScreen::groupSetIgnore (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector options)
{
    mIgnoreMode = TRUE;

    if (state & CompAction::StateInitKey)
	action->setState (state | CompAction::StateTermKey);

    return FALSE;
}

/*
 * groupUnsetIgnore
 *
 */
bool
GroupScreen::groupUnsetIgnore (CompAction          *action,
			       CompAction::State   state,
			       CompOption::Vector  options)
{
    mIgnoreMode = FALSE;

    action->setState (state & ~CompAction::StateTermKey);

    return FALSE;
}

/*
 * groupHandleButtonPressEvent
 *
 */
void
GroupScreen::groupHandleButtonPressEvent (XEvent *event)
{
    GroupSelection *group;
    int            xRoot, yRoot, button;

    xRoot  = event->xbutton.x_root;
    yRoot  = event->xbutton.y_root;
    button = event->xbutton.button;

    for (group = mGroups; group; group = group->mNext)
    {
	if (group->mInputPrevention != event->xbutton.window)
	    continue;

	if (!group->mTabBar)
	    continue;

	switch (button) {
	case Button1:
	    {
		GroupTabBarSlot *slot;

		for (slot = group->mTabBar->mSlots; slot; slot = slot->mNext)
		{
		    if (XPointInRegion (slot->mRegion, xRoot, yRoot))
		    {
			mDraggedSlot = slot;
			/* The slot isn't dragged yet */
			mDragged = FALSE;
			mPrevX = xRoot;
			mPrevY = yRoot;

			if (!screen->otherGrabExist ("group", "group-drag", NULL))
			    groupGrabScreen (ScreenGrabTabDrag);
		    }
		}
	    }
	    break;

	case Button4:
	case Button5:
	    {
		CompWindow  *topTab = NULL;
		GroupWindow *gw;

		if (group->mNextTopTab)
		    topTab = NEXT_TOP_TAB (group);
		else if (group->mTopTab)
		{
		    /* If there are no tabbing animations,
		       topTab is never NULL. */
		    topTab = TOP_TAB (group);
		}

		if (!topTab)
		    return;

		gw = GroupWindow::get (topTab);

		if (button == Button4)
		{
		    if (gw->mSlot->mPrev)
			groupChangeTab (gw->mSlot->mPrev, RotateLeft);
		    else
			groupChangeTab (gw->mGroup->mTabBar->mRevSlots,
					RotateLeft);
		}
		else
		{
		    if (gw->mSlot->mNext)
			groupChangeTab (gw->mSlot->mNext, RotateRight);
		    else
			groupChangeTab (gw->mGroup->mTabBar->mSlots, RotateRight);
		}
		break;
	    }
	}

	break;
    }
}

/*
 * groupHandleButtonReleaseEvent
 *
 */
void
GroupScreen::handleButtonReleaseEvent (XEvent *event)
{
    GroupSelection *group;
    int            vx, vy;
    Region         newRegion;
    Bool           inserted = FALSE;
    Bool           wasInTabBar = FALSE;

    if (event->xbutton.button != 1)
	return;

    if (!mDraggedSlot)
	return;

    if (!mDragged)
    {
	groupChangeTab (mDraggedSlot, RotateUncertain);
	mDraggedSlot = NULL;

	if (mGrabState == ScreenGrabTabDrag)
	    groupGrabScreen (ScreenGrabNone);

	return;
    }

    GROUP_WINDOW (mDraggedSlot->mWindow);

    newRegion = XCreateRegion ();
    if (!newRegion)
	return;

    XUnionRegion (newRegion, mDraggedSlot->mRegion, newRegion);

    groupGetDrawOffsetForSlot (mDraggedSlot, vx, vy);
    XOffsetRegion (newRegion, vx, vy);

    for (group = mGroups; group; group = group->mNext)
    {
	Bool            inTabBar;
	Region          clip, buf;
	GroupTabBarSlot *slot;

	if (!group->mTabBar || !HAS_TOP_WIN (group))
	    continue;

	/* create clipping region */
	clip = GroupWindow::get (TOP_TAB (group))->groupGetClippingRegion ();
	if (!clip)
	    continue;

	buf = XCreateRegion ();
	if (!buf)
	{
	    XDestroyRegion (clip);
	    continue;
	}

	XIntersectRegion (newRegion, group->mTabBar->mRegion, buf);
	XSubtractRegion (buf, clip, buf);
	XDestroyRegion (clip);

	inTabBar = !XEmptyRegion (buf);
	XDestroyRegion (buf);

	if (!inTabBar)
	    continue;

	wasInTabBar = TRUE;

	for (slot = group->mTabBar->mSlots; slot; slot = slot->mNext)
	{
	    GroupTabBarSlot *tmpDraggedSlot;
	    GroupSelection  *tmpGroup;
	    Region          slotRegion, buf;
	    XRectangle      rect;
	    Bool            inSlot;

	    if (slot == mDraggedSlot)
		continue;

	    slotRegion = XCreateRegion ();
	    if (!slotRegion)
		continue;

	    if (slot->mPrev && slot->mPrev != mDraggedSlot)
	    {
		rect.x = slot->mPrev->mRegion->extents.x2;
	    }
	    else if (slot->mPrev && slot->mPrev == mDraggedSlot &&
		     mDraggedSlot->mPrev)
	    {
		rect.x = mDraggedSlot->mPrev->mRegion->extents.x2;
	    }
	    else
		rect.x = group->mTabBar->mRegion->extents.x1;

	    rect.y = slot->mRegion->extents.y1;

	    if (slot->mNext && slot->mNext != mDraggedSlot)
	    {
		rect.width = slot->mNext->mRegion->extents.x1 - rect.x;
	    }
	    else if (slot->mNext && slot->mNext == mDraggedSlot &&
		     mDraggedSlot->mNext)
	    {
		rect.width = mDraggedSlot->mNext->mRegion->extents.x1 - rect.x;
	    }
	    else
		rect.width = group->mTabBar->mRegion->extents.x2;

	    rect.height = slot->mRegion->extents.y2 - slot->mRegion->extents.y1;

	    XUnionRectWithRegion (&rect, slotRegion, slotRegion);

	    buf = XCreateRegion ();
	    if (!buf)
		continue;

	    XIntersectRegion (newRegion, slotRegion, buf);
	    inSlot = !XEmptyRegion (buf);

	    XDestroyRegion (buf);
	    XDestroyRegion (slotRegion);

	    if (!inSlot)
		continue;

	    tmpDraggedSlot = mDraggedSlot;

	    if (group != gw->mGroup)
	    {
		CompWindow     *w = mDraggedSlot->mWindow;
		GroupSelection *tmpGroup = gw->mGroup;
		int            oldPosX = WIN_CENTER_X (w);
		int            oldPosY = WIN_CENTER_Y (w);

		/* if the dragged window is not the top tab,
		   move it onscreen */
		if (tmpGroup->mTopTab && !IS_TOP_TAB (w, tmpGroup))
		{
		    CompWindow *tw = TOP_TAB (tmpGroup);

		    oldPosX = WIN_CENTER_X (tw) + gw->mMainTabOffset.x;
		    oldPosY = WIN_CENTER_Y (tw) + gw->mMainTabOffset.y;

		    GroupWindow::get (w)->groupSetWindowVisibility (TRUE);
		}

		/* Change the group. */
		GroupWindow::get (mDraggedSlot->mWindow)->groupDeleteGroupWindow ();
		GroupWindow::get (mDraggedSlot->mWindow)->groupAddWindowToGroup (group, 0);

		/* we saved the original center position in oldPosX/Y before -
		   now we should apply that to the new main tab offset */
		if (HAS_TOP_WIN (group))
		{
		    CompWindow *tw = TOP_TAB (group);
		    gw->mMainTabOffset.x = oldPosX - WIN_CENTER_X (tw);
		    gw->mMainTabOffset.y = oldPosY - WIN_CENTER_Y (tw);
		}
	    }
	    else
		groupUnhookTabBarSlot (group->mTabBar, mDraggedSlot, TRUE);

	    mDraggedSlot = NULL;
	    mDragged = FALSE;
	    inserted = TRUE;

	    if ((tmpDraggedSlot->mRegion->extents.x1 +
		 tmpDraggedSlot->mRegion->extents.x2 + (2 * vx)) / 2 >
		(slot->mRegion->extents.x1 + slot->mRegion->extents.x2) / 2)
	    {
		groupInsertTabBarSlotAfter (group->mTabBar,
					    tmpDraggedSlot, slot);
	    }
	    else
		groupInsertTabBarSlotBefore (group->mTabBar,
					     tmpDraggedSlot, slot);

	    groupDamageTabBarRegion (group);

	    /* Hide tab-bars. */
	    for (tmpGroup = mGroups; tmpGroup; tmpGroup = tmpGroup->mNext)
	    {
		if (group == tmpGroup)
		    groupTabSetVisibility (tmpGroup, TRUE, 0);
		else
		    groupTabSetVisibility (tmpGroup, FALSE, PERMANENT);
	    }

	    break;
	}

	if (inserted)
	    break;
    }

    XDestroyRegion (newRegion);

    if (!inserted)
    {
	CompWindow     *draggedSlotWindow = mDraggedSlot->mWindow;
	GroupSelection *tmpGroup;

	for (tmpGroup = mGroups; tmpGroup; tmpGroup = tmpGroup->mNext)
	    groupTabSetVisibility (tmpGroup, FALSE, PERMANENT);

	mDraggedSlot = NULL;
	mDragged = FALSE;

	if (optionGetDndUngroupWindow () && !wasInTabBar)
	{
	    GroupWindow::get(draggedSlotWindow)->groupRemoveWindowFromGroup ();
	}
	else if (gw->mGroup && gw->mGroup->mTopTab)
	{
	    groupRecalcTabBarPos (gw->mGroup,
				  (gw->mGroup->mTabBar->mRegion->extents.x1 +
				   gw->mGroup->mTabBar->mRegion->extents.x2) / 2,
				  gw->mGroup->mTabBar->mRegion->extents.x1,
				  gw->mGroup->mTabBar->mRegion->extents.x2);
	}

	/* to remove the painted slot */
	cScreen->damageScreen ();
    }

    if (mGrabState == ScreenGrabTabDrag)
	groupGrabScreen (ScreenGrabNone);

    if (mDragHoverTimeoutHandle.active ())
    {
	mDragHoverTimeoutHandle.stop ();
    }
}

/*
 * groupHandleMotionEvent
 *
 */

/* the radius to determine if it was a click or a drag */
#define RADIUS 5

void
GroupScreen::groupHandleMotionEvent (int xRoot,
				     int yRoot)
{
    if (mGrabState == ScreenGrabTabDrag)
    {
	int    dx, dy;
	int    vx, vy;
	REGION reg;
	Region draggedRegion = mDraggedSlot->mRegion;

	reg.rects = &reg.extents;
	reg.numRects = 1;

	dx = xRoot - mPrevX;
	dy = yRoot - mPrevY;

	if (mDragged || abs (dx) > RADIUS || abs (dy) > RADIUS)
	{
	    CompRegion cReg;
	    mPrevX = xRoot;
	    mPrevY = yRoot;

	    if (!mDragged)
	    {
		GroupSelection *group;
		BoxRec         *box;

		GROUP_WINDOW (mDraggedSlot->mWindow);

		mDragged = TRUE;

		for (group = mGroups; group; group = group->mNext)
		    groupTabSetVisibility (group, TRUE, PERMANENT);

		box = &gw->mGroup->mTabBar->mRegion->extents;
		groupRecalcTabBarPos (gw->mGroup, (box->x1 + box->x2) / 2,
				      box->x1, box->x2);
	    }

	    groupGetDrawOffsetForSlot (mDraggedSlot, vx, vy);

	    reg.extents.x1 = draggedRegion->extents.x1 + vx;
	    reg.extents.y1 = draggedRegion->extents.y1 + vy;
	    reg.extents.x2 = draggedRegion->extents.x2 + vx;
	    reg.extents.y2 = draggedRegion->extents.y2 + vy;

	    cReg = CompRegion (reg.extents.x1, reg.extents.y1,
			       reg.extents.x2 - reg.extents.x1,
			       reg.extents.y2 - reg.extents.y2);

	    cScreen->damageRegion (cReg);

	    XOffsetRegion (mDraggedSlot->mRegion, dx, dy);
	    mDraggedSlot->mSpringX =
		(mDraggedSlot->mRegion->extents.x1 +
		 mDraggedSlot->mRegion->extents.x2) / 2;

	    reg.extents.x1 = draggedRegion->extents.x1 + vx;
	    reg.extents.y1 = draggedRegion->extents.y1 + vy;
	    reg.extents.x2 = draggedRegion->extents.x2 + vx;
	    reg.extents.y2 = draggedRegion->extents.y2 + vy;

	    cReg = CompRegion (reg.extents.x1, reg.extents.y1,
			       reg.extents.x2 - reg.extents.x1,
			       reg.extents.y2 - reg.extents.y2);

	    cScreen->damageRegion (cReg);
	}
    }
    else if (mGrabState == ScreenGrabSelect)
    {
	groupDamageSelectionRect (xRoot, yRoot);
    }
}

/*
 * groupHandleEvent
 *
 */
void
GroupScreen::handleEvent (XEvent      *event)
{
    CompWindow *w;

    switch (event->type) {
    case MotionNotify:
	groupHandleMotionEvent (pointerX, pointerY);
	break;

    case ButtonPress:
	groupHandleButtonPressEvent (event);
	break;

    case ButtonRelease:
	handleButtonReleaseEvent (event);
	break;

    case MapNotify:
	w = screen->findWindow (event->xmap.window);
	if (w)
	{
	    foreach (CompWindow *cw, screen->windows ())
	    {
		if (w->id () == cw->frame ())
		{
		    /* Should not unmap frame window here */
		    //if (gw->mWindowHideInfo)
			//XUnmapWindow (screen->dpy (), cw->frame ());
		}
	    }
	}
	break;

    case UnmapNotify:
	w = screen->findWindow (event->xunmap.window);
	if (w)
	{
	    GROUP_WINDOW (w);

	    if (w->pendingUnmaps ())
	    {
		if (w->shaded ())
		{
		    gw->mWindowState = WindowShaded;

		    if (gw->mGroup && optionGetShadeAll ())
			groupShadeWindows (w, gw->mGroup, TRUE);
		}
		else if (w->minimized ())
		{
		    gw->mWindowState = WindowMinimized;

		    if (gw->mGroup && optionGetMinimizeAll ())
			groupMinimizeWindows (w, gw->mGroup, TRUE);
		}
	    }

	    if (gw->mGroup)
	    {
		if (gw->mGroup->mTabBar && IS_TOP_TAB (w, gw->mGroup))
		{
		    /* on unmap of the top tab, hide the tab bar and the
		       input prevention window */
		    groupTabSetVisibility (gw->mGroup, FALSE, PERMANENT);
		}
		if (!w->pendingUnmaps ())
		{
		    /* close event */
		    if (!(gw->mAnimateState & IS_UNGROUPING))
		    {
			gw->groupDeleteGroupWindow ();
			cScreen->damageScreen ();
		    }
		}
	    }
	}
	break;

    case ClientMessage:
	if (event->xclient.message_type == Atoms::winActive)
	{
	    w = screen->findWindow (event->xclient.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->mGroup && gw->mGroup->mTabBar &&
		    !IS_TOP_TAB (w, gw->mGroup))
		{
		    gw->mGroup->mCheckFocusAfterTabChange = TRUE;
		    groupChangeTab (gw->mSlot, RotateUncertain);
		}
	    }
	}
	else if (event->xclient.message_type == mResizeNotifyAtom)
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xclient.window);

	    if (w && mResizeInfo && (w == mResizeInfo->mResizedWindow))
	    {
		GROUP_WINDOW (w);

		if (gw->mGroup)
		{			
		    int        i;
		    XRectangle rect;

		    rect.x      = event->xclient.data.l[0];
		    rect.y      = event->xclient.data.l[1];
		    rect.width  = event->xclient.data.l[2];
		    rect.height = event->xclient.data.l[3];

		    for (i = 0; i < gw->mGroup->mNWins; i++)
		    {
			CompWindow  *cw = gw->mGroup->mWindows[i];
			GroupWindow *gcw;

			gcw = GroupWindow::get (cw);
			if (gcw->mResizeGeometry)
			{
			    if (gcw->groupUpdateResizeRectangle (&rect, TRUE))
			    {
				gcw->cWindow->addDamage ();
			    }
			}
		    }
		}
	    }
	}
	break;

    default:
	if (event->type == screen->shapeEvent () + ShapeNotify)
	{
	    XShapeEvent *se = (XShapeEvent *) event;
	    if (se->kind == ShapeInput)
	    {
		CompWindow *w;
		w = screen->findWindow (se->window);
		if (w)
		{
		    GROUP_WINDOW (w);

		    if (gw->mWindowHideInfo)
			gw->groupClearWindowInputShape (gw->mWindowHideInfo);
		}
	    }
	}
	break;
    }

    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == Atoms::wmName)
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->mGroup && gw->mGroup->mTabBar &&
		    gw->mGroup->mTabBar->mTextSlot    &&
		    gw->mGroup->mTabBar->mTextSlot->mWindow == w)
		{
		    /* make sure we are using the updated name */
		    groupRenderWindowTitle (gw->mGroup);
		    groupDamageTabBarRegion (gw->mGroup);
		}
	    }
	}
	break;

    case EnterNotify:
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xcrossing.window);
	    
	    groupUpdateTabBars (event->xcrossing.window);
	    
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (mShowDelayTimeoutHandle.active ())
		    mShowDelayTimeoutHandle.stop ();

		if (gw->mGroup)
		{
		    if (mDraggedSlot && mDragged &&
			IS_TOP_TAB (w, gw->mGroup))
		    {
			int hoverTime;
			hoverTime = optionGetDragHoverTime () * 1000;
			if (mDragHoverTimeoutHandle.active ())
			    mDragHoverTimeoutHandle.stop ();

			if (hoverTime > 0)
			{
			    mDragHoverTimeoutHandle.setCallback (
				boost::bind (
				    &GroupWindow::groupDragHoverTimeout,
				    gw));
			    mDragHoverTimeoutHandle.setTimes (hoverTime, hoverTime * 1.2);
			    mDragHoverTimeoutHandle.start ();
			}
		    }
		}
	    }
	}
	break;

    case ConfigureNotify:
	{
	    CompWindow *w;

	    w = screen->findWindow (event->xconfigure.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->mGroup && gw->mGroup->mTabBar &&
		    IS_TOP_TAB (w, gw->mGroup)      &&
		    gw->mGroup->mInputPrevention && gw->mGroup->mIpwMapped)
		{
		    XWindowChanges xwc;

		    xwc.stack_mode = Above;
		    xwc.sibling = w->id ();

		    XConfigureWindow (screen->dpy (),
				      gw->mGroup->mInputPrevention,
				      CWSibling | CWStackMode, &xwc);
		}

		if (event->xconfigure.above != None)
		{
		    if (gw->mGroup && !gw->mGroup->mTabBar &&
			(gw->mGroup != mLastRestackedGroup))
		    {
			if (optionGetRaiseAll ())
			    groupRaiseWindows (w, gw->mGroup);
		    }
		    if (w->managed () && !w->overrideRedirect ())
			mLastRestackedGroup = gw->mGroup;
		}
	    }
	}
	break;

    default:
	break;
    }
}

/*
 * groupGetOutputExtentsForWindow
 *
 */
void
GroupWindow::getOutputExtents (CompWindowExtents &output)
{
    GROUP_SCREEN (screen);

    window->getOutputExtents (output);

    if (mGroup && mGroup->mNWins > 1)
    {
	int glowSize = gs->optionGetGlowSize ();
	int glowType = gs->optionGetGlowType ();
	int glowTextureSize = gs->mGlowTextureProperties[glowType].textureSize;
	int glowOffset = gs->mGlowTextureProperties[glowType].glowOffset;

	glowSize = glowSize * (glowTextureSize - glowOffset) / glowTextureSize;

	/* glowSize is the size of the glow outside the window decoration
	 * (w->input), while w->output includes the size of w->input
	 * this is why we have to add w->input here */
	output.left   = MAX (output.left, glowSize + window->input ().left);
	output.right  = MAX (output.right, glowSize + window->input ().right);
	output.top    = MAX (output.top, glowSize + window->input ().top);
	output.bottom = MAX (output.bottom, glowSize + window->input ().bottom);
    }
}

/*
 * groupWindowResizeNotify
 *
 */
void
GroupWindow::resizeNotify (int dx,
			   int dy,
			   int dwidth,
			   int dheight)
{
    GROUP_SCREEN (screen);

    if (mResizeGeometry)
    {
	free (mResizeGeometry);
	mResizeGeometry = NULL;
    }

    window->resizeNotify (dx, dy, dwidth, dheight);

    if (mGlowQuads)
    {
	/* FIXME: we need to find a more multitexture friendly way
	 * of doing this */
	GLTexture::Matrix tMat = gs->mGlowTexture.at (0)->matrix ();
	groupComputeGlowQuads (&tMat);
    }

    if (mGroup && mGroup->mTabBar && IS_TOP_TAB (window, mGroup))
    {
	if (mGroup->mTabBar->mState != PaintOff)
	{
	    gs->groupRecalcTabBarPos (mGroup, pointerX,
				  WIN_X (window), WIN_X (window) + WIN_WIDTH (window));
	}
    }
}

/*
 * groupWindowMoveNotify
 *
 */
void
GroupWindow::moveNotify (int    dx,
			 int    dy,
			 bool   immediate)
{
    Bool       viewportChange;
    int        i;

    GROUP_SCREEN (screen);

    window->moveNotify (dx, dy, immediate);

    if (mGlowQuads)
    {
	/* FIXME: we need to find a more multitexture friendly way
	 * of doing this */
	GLTexture::Matrix tMat = gs->mGlowTexture.at (0)->matrix ();
	groupComputeGlowQuads (&tMat);
    }

    if (!mGroup || gs->mQueued)
	return;

    /* FIXME: we need a reliable, 100% safe way to detect window
       moves caused by viewport changes here */
    viewportChange = ((dx && !(dx % screen->width ())) ||
		      (dy && !(dy % screen->height ())));

    if (viewportChange && (mAnimateState & IS_ANIMATED))
    {
	mDestination.x += dx;
	mDestination.y += dy;
    }

    if (mGroup->mTabBar && IS_TOP_TAB (window, mGroup))
    {
	GroupTabBarSlot *slot;
	GroupTabBar     *bar = mGroup->mTabBar;

	bar->mRightSpringX += dx;
	bar->mLeftSpringX += dx;

	gs->groupMoveTabBarRegion (mGroup, dx, dy, TRUE);

	for (slot = bar->mSlots; slot; slot = slot->mNext)
	{
	    XOffsetRegion (slot->mRegion, dx, dy);
	    slot->mSpringX += dx;
	}
    }

    if (!gs->optionGetMoveAll () || gs->mIgnoreMode ||
	(mGroup->mTabbingState != NoTabbing) ||
	(mGroup->mGrabWindow != window->id ()) ||
	!(mGroup->mGrabMask & CompWindowGrabMoveMask))
    {
	return;
    }

    for (i = 0; i < mGroup->mNWins; i++)
    {
	CompWindow *cw = mGroup->mWindows[i];
	if (!cw)
	    continue;

	if (cw->id () == window->id ())
	    continue;

	GROUP_WINDOW (cw);

	if (cw->state () & MAXIMIZE_STATE)
	{
	    if (viewportChange)
		gw->groupEnqueueMoveNotify (-dx, -dy, immediate, TRUE);
	}
	else if (!viewportChange)
	{
	    gw->mNeedsPosSync = TRUE;
	    gw->groupEnqueueMoveNotify (dx, dy, immediate, FALSE);
	}
    }
}

void
GroupWindow::grabNotify (int          x,
			 int	      y,
			 unsigned int state,
			 unsigned int mask)
{
    GROUP_SCREEN (screen);
    
    gs->mLastGrabbedWindow = window->id ();

    if (mGroup && !gs->mIgnoreMode && !gs->mQueued)
    {
	Bool doResizeAll;
	int  i;

	doResizeAll = gs->optionGetResizeAll () &&
	              (mask & CompWindowGrabResizeMask);

	if (mGroup->mTabBar)
	    gs->groupTabSetVisibility (mGroup, FALSE, 0);

	for (i = 0; i < mGroup->mNWins; i++)
	{
	    CompWindow *cw = mGroup->mWindows[i];
	    if (!cw)
		continue;

	    if (cw->id () != window->id ())
	    {
		GroupWindow *gcw = GroupWindow::get (cw);

		gcw->groupEnqueueGrabNotify (x, y, state, mask);

		if (doResizeAll && !(cw->state () & MAXIMIZE_STATE))
		{
		    if (!gcw->mResizeGeometry)
		    {
			gcw->mResizeGeometry = (XRectangle *) malloc (sizeof (XRectangle));
		    }
		    if (gcw->mResizeGeometry)
		    {
			gcw->mResizeGeometry->x      = WIN_X (cw);
			gcw->mResizeGeometry->y      = WIN_Y (cw);
			gcw->mResizeGeometry->width  = WIN_WIDTH (cw);
			gcw->mResizeGeometry->height = WIN_HEIGHT (cw);
		    }
		}
	    }
	}

	if (doResizeAll)
	{
	    if (!gs->mResizeInfo)
		gs->mResizeInfo = (GroupResizeInfo *) malloc (sizeof (GroupResizeInfo));

	    if (gs->mResizeInfo)
	    {
		gs->mResizeInfo->mResizedWindow       = window;
		gs->mResizeInfo->mOrigGeometry.x      = WIN_X (window);
		gs->mResizeInfo->mOrigGeometry.y      = WIN_Y (window);
		gs->mResizeInfo->mOrigGeometry.width  = WIN_WIDTH (window);
		gs->mResizeInfo->mOrigGeometry.height = WIN_HEIGHT (window);
	    }
	}

	mGroup->mGrabWindow = window->id ();
	mGroup->mGrabMask = mask;
    }

    window->grabNotify (x, y, state, mask);
}

void
GroupWindow::ungrabNotify ()
{
    GROUP_SCREEN (screen);
    
    gs->mLastGrabbedWindow = None;

    if (mGroup && !gs->mIgnoreMode && !gs->mQueued)
    {
	int        i;
	XRectangle rect;

	gs->groupDequeueMoveNotifies ();

	if (gs->mResizeInfo)
	{
	    rect.x      = WIN_X (window);
	    rect.y      = WIN_Y (window);
	    rect.width  = WIN_WIDTH (window);
	    rect.height = WIN_HEIGHT (window);
	}

	for (i = 0; i < mGroup->mNWins; i++)
	{
	    CompWindow *cw = mGroup->mWindows[i];
	    if (!cw)
		continue;

	    if (cw->id () != window->id ())
	    {
		GROUP_WINDOW (cw);

		if (gw->mResizeGeometry)
		{
		    unsigned int mask;

		    gw->mResizeGeometry->x      = WIN_X (cw);
		    gw->mResizeGeometry->y      = WIN_Y (cw);
		    gw->mResizeGeometry->width  = WIN_WIDTH (cw);
		    gw->mResizeGeometry->height = WIN_HEIGHT (cw);

		    mask = gw->groupUpdateResizeRectangle (&rect, FALSE);
		    if (mask)
		    {
			XWindowChanges xwc;
			xwc.x      = gw->mResizeGeometry->x;
			xwc.y      = gw->mResizeGeometry->y;
			xwc.width  = gw->mResizeGeometry->width;
			xwc.height = gw->mResizeGeometry->height;

			if (window->mapNum () && (mask & (CWWidth | CWHeight)))
			    window->sendSyncRequest ();

			cw->configureXWindow (mask, &xwc);
		    }
		    else
		    {
			free (mResizeGeometry);
			gw->mResizeGeometry =  NULL;
		    }
		}
		if (mNeedsPosSync)
		{
		    cw->syncPosition ();
		    mNeedsPosSync = FALSE;
		}
		groupEnqueueUngrabNotify ();
	    }
	}

	if (gs->mResizeInfo)
	{
	    free (gs->mResizeInfo);
	    gs->mResizeInfo = NULL;
	}

	mGroup->mGrabWindow = None;
	mGroup->mGrabMask = 0;
    }

    window->ungrabNotify ();
}

void
GroupWindow::windowNotify (CompWindowNotify n)
{
    return window->windowNotify (n);
}

bool
GroupWindow::damageRect (bool	        initial,
			 const CompRect &rect)
{
    bool       status;

    GROUP_SCREEN (screen);

    status = cWindow->damageRect (initial, rect);

    if (initial)
    {
	if (gs->optionGetAutotabCreate () && groupIsGroupWindow ())
	{
	    if (!mGroup && (mWindowState == WindowNormal))
	    {
		groupAddWindowToGroup (NULL, 0);
		gs->groupTabGroup (window);
	    }
	}

	if (mGroup)
	{
	    if (mWindowState == WindowMinimized)
	    {
		if (gs->optionGetMinimizeAll ())
		    gs->groupMinimizeWindows (window, mGroup, FALSE);
	    }
	    else if (mWindowState == WindowShaded)
	    {
		if (gs->optionGetShadeAll ())
		    gs->groupShadeWindows (window, mGroup, FALSE);
	    }
	}

	mWindowState = WindowNormal;
    }

    if (mResizeGeometry)
    {
	BoxRec box;
	float  dummy = 1;

	groupGetStretchRectangle (&box, dummy, dummy);
	gs->groupDamagePaintRectangle (&box);
    }

    if (mSlot)
    {
	int    vx, vy;
	Region reg;
	CompRegion cReg;

	gs->groupGetDrawOffsetForSlot (mSlot, vx, vy);
	if (vx || vy)
	{
	    reg = XCreateRegion ();
	    XUnionRegion (reg, mSlot->mRegion, reg);
	    XOffsetRegion (reg, vx, vy);
	}
	else
	    reg = mSlot->mRegion;

	cReg = CompRegion (reg->extents.x1, reg->extents.y1,
		           reg->extents.x2 - reg->extents.x1,
		           reg->extents.y2 - reg->extents.y2);

	if (vx || vy)
	    XDestroyRegion (reg);
    }

    return status;
}

void
GroupWindow::stateChangeNotify (unsigned int lastState)
{
    GROUP_SCREEN (screen);

    if (mGroup && !gs->mIgnoreMode)
    {
	if (((lastState & MAXIMIZE_STATE) != (window->state () & MAXIMIZE_STATE)) &&
	    gs->optionGetMaximizeUnmaximizeAll ())
	{
	    int i;
	    for (i = 0; i < mGroup->mNWins; i++)
	    {
		CompWindow *cw = mGroup->mWindows[i];
		if (!cw)
		    continue;

		if (cw->id () == window->id ())
		    continue;

		cw->maximize (window->state () & MAXIMIZE_STATE);
	    }
	}
    }

    window->stateChangeNotify (lastState);
}

void
GroupWindow::activate ()
{
    GROUP_SCREEN (screen);

    if (mGroup && mGroup->mTabBar && !IS_TOP_TAB (window, mGroup))
	gs->groupChangeTab (mSlot, RotateUncertain);

    window->activate ();
}
