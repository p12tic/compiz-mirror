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
 * isGroupWindow
 *
 */

bool
GroupWindow::isGroupWindow ()
{
    if (window->overrideRedirect ())
	return false;

    if (window->type () & CompWindowTypeDesktopMask)
	return false;

    if (window->invisible ())
	return false;

    if (!GroupScreen::get (screen)->optionGetWindowMatch ().evaluate (window))
	return false;

    return true;
}

/*
 * dragHoverTimeout
 *
 * Description:
 * Activates a window after a certain time a slot has been dragged over it.
 *
 */
bool
GroupWindow::dragHoverTimeout ()
{
    GROUP_SCREEN (screen);

    if (gs->optionGetBarAnimations ())
    {
	GroupTabBar *bar = mGroup->mTabBar;

	bar->mBgAnimation = AnimationPulse;
	bar->mBgAnimationTime = gs->optionGetPulseTime () * 1000;
    }

    window->activate ();

    return false;
}

/*
 * checkWindowProperty
 *
 */
bool
GroupWindow::checkWindowProperty (CompWindow *w,
				       long int   *id,
				       bool       *tabbed,
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
	    return true;
	}
	else if (fmt != 0)
	    XFree (data);
    }

    return false;
}

/*
 * updateWindowProperty
 *
 */
void
GroupWindow::updateWindowProperty ()
{
    GROUP_SCREEN (screen);

    // Do not change anything in this case
    if (mReadOnlyProperty)
	return;

    if (mGroup)
    {
	long int buffer[5];

	buffer[0] = mGroup->mIdentifier;
	buffer[1] = (mSlot) ? true : false;

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
GroupWindow::updateResizeRectangle (CompRect   masterGeometry,
					 bool	    damage)
{
    CompRect     newGeometry;
    unsigned int mask = 0;
    int          newWidth, newHeight;
    int          widthDiff, heightDiff;

    if (mResizeGeometry.isEmpty () || !mGroup->mResizeInfo)
	return 0;

    newGeometry.setX (WIN_X (window) + (masterGeometry.x () -
				 mGroup->mResizeInfo->mOrigGeometry.x ()));
    newGeometry.setY (WIN_Y (window) + (masterGeometry.y () -
				 mGroup->mResizeInfo->mOrigGeometry.y ()));

    widthDiff = masterGeometry.width () - mGroup->mResizeInfo->mOrigGeometry.width ();
    newGeometry.setWidth (MAX (1, WIN_WIDTH (window) + widthDiff));
    heightDiff = masterGeometry.height () - mGroup->mResizeInfo->mOrigGeometry.height ();
    newGeometry.setHeight (MAX (1, WIN_HEIGHT (window) + heightDiff));

    if (window->constrainNewWindowSize (newGeometry.width (), newGeometry.height (),
					&newWidth, &newHeight))
    {
	newGeometry.setSize (CompSize (newWidth, newHeight));
    }

    if (damage)
    {
	if (mResizeGeometry != newGeometry)
	{
	    cWindow->addDamage ();
	}
    }

    if (newGeometry.x () != mResizeGeometry.x ())
    {
	mResizeGeometry.setX (newGeometry.x ());
	mask |= CWX;
    }
    if (newGeometry.y () != mResizeGeometry.y ())
    {
	mResizeGeometry.setY (newGeometry.y ());
	mask |= CWY;
    }
    if (newGeometry.width () != mResizeGeometry.width ())
    {
	mResizeGeometry.setWidth (newGeometry.width ());
	mask |= CWWidth;
    }
    if (newGeometry.height () != mResizeGeometry.height ())
    {
	mResizeGeometry.setHeight (newGeometry.height ());
	mask |= CWHeight;
    }

    return mask;
}

/*
 * grabScreen
 *
 */
void
GroupScreen::grabScreen (GroupScreen::GrabState newState)
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
 * GroupSelection::raiseWindows
 *
 */
void
GroupSelection::raiseWindows (CompWindow     *top)
{
    CompWindowList stack;
    CompWindowList::iterator it;

    if (mWindows.size () == 1)
	return;

    stack.resize (mWindows.size () - 1);

    it = stack.begin ();

    foreach (CompWindow *w, screen->windows ())
    {
	GROUP_WINDOW (w);
	if ((w->id () != top->id ()) && (gw->mGroup == this))
	{
	    (*it) = w;
	    it++;
	}
    }

    foreach (CompWindow *cw, stack)
	cw->restackBelow (top);
}

/*
 * groupMinimizeWindows
 *
 */
void
GroupSelection::minimizeWindows (CompWindow     *top,
				 bool           minimize)
{
    foreach (CompWindow *w, mWindows)
    {
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
GroupSelection::shadeWindows (CompWindow     *top,
			      bool           shade)
{
    unsigned int state;

    foreach (CompWindow *w, mWindows)
    {
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
 * GroupSelection::moveWindows
 *
 */

void
GroupSelection::moveWindows (CompWindow *top,
			     int 	dx,
			     int 	dy,
			     bool 	immediate,
			     bool	viewportChange)
{
    foreach (CompWindow *cw, mWindows)
    {
	if (!cw)
	    continue;

	if (cw->id () == top->id ())
	    continue;

	GROUP_WINDOW (cw);

	if (cw->state () & MAXIMIZE_STATE)
	{
	    if (viewportChange)
		gw->enqueueMoveNotify (-dx, -dy, immediate, true);
	}
	else if (!viewportChange)
	{
	    gw->mNeedsPosSync = true;
	    gw->enqueueMoveNotify (dx, dy, immediate, true);
	}
    }
}

/*
 * GroupSelection::prepareResizeWindows
 * 
 * Description: Sets the resize geometry of this group
 * and makes windows appear "stretched".
 * 
 * Use it for animation or something. Currently used to
 * paint windows as stretched while the primary window
 * is resizing (eg through rectangle, outline or stretch mode)
 *
 */

void
GroupSelection::prepareResizeWindows (CompRect &rect)
{
    foreach (CompWindow *cw, mWindows)
    {
	GroupWindow *gcw;

	gcw = GroupWindow::get (cw);
	if (!gcw->mResizeGeometry.isEmpty ())
	{
	    if (gcw->updateResizeRectangle (rect, true))
	    {
		gcw->cWindow->addDamage ();
	    }
	}
    }
}

/*
 * GroupSelection::resizeWindows
 * 
 * Description: Configures windows according to set resize geometry
 * in prepareResizeWindows
 *
 */

void
GroupSelection::resizeWindows (CompWindow *top)
{
    CompRect   rect;
    
    GROUP_SCREEN (screen);
    
    gs->dequeueMoveNotifies ();

    if (mResizeInfo)
    {
	rect = CompRect (WIN_X (top),
			 WIN_Y (top),
			 WIN_WIDTH (top),
			 WIN_HEIGHT (top));
    }

    foreach (CompWindow *cw, mWindows)
    {
	if (!cw)
	    continue;

	if (cw->id () != top->id ())
	{
	    GROUP_WINDOW (cw);

	    if (!gw->mResizeGeometry.isEmpty ())
	    {
		unsigned int mask;

		gw->mResizeGeometry = CompRect (WIN_X (cw),
						WIN_Y (cw),
						WIN_WIDTH (cw),
						WIN_HEIGHT (cw));

		mask = gw->updateResizeRectangle (rect, false);
		if (mask)
		{
		    XWindowChanges xwc;
		    xwc.x      = gw->mResizeGeometry.x ();
		    xwc.y      = gw->mResizeGeometry.y ();
		    xwc.width  = gw->mResizeGeometry.width ();
		    xwc.height = gw->mResizeGeometry.height ();

		    if (top->mapNum () && (mask & (CWWidth | CWHeight)))
			top->sendSyncRequest ();

		    cw->configureXWindow (mask, &xwc);
		}
		else
		{
		    GroupWindow::get (top)->mResizeGeometry = CompRect (0, 0, 0, 0);
		}
	    }
	    if (GroupWindow::get (top)->mNeedsPosSync)
	    {
		cw->syncPosition ();
		GroupWindow::get (top)->mNeedsPosSync = false;
	    }
	    GroupWindow::get (top)->enqueueUngrabNotify ();
	}
    }

    if (mResizeInfo)
    {
	free (mResizeInfo);
	mResizeInfo = NULL;
    }

    mGrabWindow = None;
    mGrabMask = 0;
}

/*
 * GroupSelection::maximizeWindows
 *
 */

void
GroupSelection::maximizeWindows (CompWindow *top)
{
    foreach (CompWindow *cw, mWindows)
    {
	if (!cw)
	    continue;

	if (cw->id () == top->id ())
	    continue;

	cw->maximize (top->state () & MAXIMIZE_STATE);
    }
}

/*
 * deleteGroupWindow
 *
 */
void
GroupWindow::deleteGroupWindow ()
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
	    group->mTabBar->unhookTabBarSlot (mSlot, false);
	}
	else
	    group->mTabBar->deleteTabBarSlot (mSlot);
    }

    if (group->mWindows.size ())
    {
	if (group->mWindows.size () > 1)
	{
	    group->mWindows.remove (window);

	    if (group->mWindows.size () == 1)
	    {
		/* Glow was removed from this window, too */
		CompositeWindow::get (group->mWindows.front ())->damageOutputExtents ();
		group->mWindows.front ()->updateWindowOutputExtents ();

		if (gs->optionGetAutoUngroup ())
		{
		    if (group->mTabBar->mChangeState != NoTabChange)
		    {
			/* a change animation is pending: this most
			   likely means that a window must be moved
			   back onscreen, so we do that here */
			CompWindow *lw = group->mWindows.front ();

			GroupWindow::get (lw)->setWindowVisibility (true);
		    }
		    if (!gs->optionGetAutotabCreate ())
			group->fini ();
		}
	    }
	}
	else
	{
	    group->mWindows.clear ();
	    group->fini ();
	}

	mGroup = NULL;
	cWindow->damageOutputExtents ();
	window->updateWindowOutputExtents ();
	updateWindowProperty ();
    }
}

void
GroupWindow::removeWindowFromGroup ()
{
    GROUP_SCREEN (screen);

    if (!mGroup)
	return;

    if (mGroup->mTabBar && !(mAnimateState & IS_UNGROUPING) &&
	(mGroup->mWindows.size () > 1))
    {
	GroupSelection *group = mGroup;

	/* if the group is tabbed, setup untabbing animation. The
	   window will be deleted from the group at the
	   end of the untabbing. */
	if (HAS_TOP_WIN (group))
	{
	    CompWindow *tw = TOP_TAB (group);
	    int        oldX = mOrgPos.x ();
	    int        oldY = mOrgPos.y ();

	    mOrgPos =
	       CompPoint (WIN_CENTER_X (tw) - (WIN_WIDTH (window) / 2),
			  WIN_CENTER_Y (tw) - (WIN_HEIGHT (window) / 2));

	    mDestination = mOrgPos + mMainTabOffset;

	    mMainTabOffset = CompPoint (oldX, oldY);

	    if (mTx || mTy)
	    {
		mTx -= (mOrgPos.x () - oldX);
		mTy -= (mOrgPos.y () - oldY);
	    }

	    mAnimateState = IS_ANIMATED;
	    mXVelocity = mYVelocity = 0.0f;
	}

	/* Although when there is no top-tab, it will never really
	   animate anything, if we don't start the animation,
	   the window will never get removed. */
	group->startTabbingAnimation (false);

	setWindowVisibility (true);
	group->mUngroupState = UngroupSingle;
	mAnimateState |= IS_UNGROUPING;
    }
    else
    {
	/* no tab bar - delete immediately */
	deleteGroupWindow ();

	if (gs->optionGetAutotabCreate () && isGroupWindow ())
	{
	    addWindowToGroup (NULL, 0);
	    if (GroupWindow::get (window)->mGroup)
		GroupWindow::get (window)->mGroup->tabGroup (window);
	}
    }
}

GroupSelection::~GroupSelection ()
{
}

/*
 * GroupSelection::fini
 *
 */
void
GroupSelection::fini ()
{
    GROUP_SCREEN (screen);
	
    if (mWindows.size ())
    {
	if (mTabBar)
	{
	    /* set up untabbing animation and delete the group
	       at the end of the animation */
	    untabGroup ();
	    mUngroupState = UngroupAll;
	    return;
	}

	foreach (CompWindow *cw, mWindows)
	{
	    GROUP_WINDOW (cw);

	    CompositeWindow::get (cw)->damageOutputExtents ();
	    gw->mGroup = NULL;
	    cw->updateWindowOutputExtents ();
	    gw->updateWindowProperty ();

	    if (gs->optionGetAutotabCreate () && gw->isGroupWindow ())
	    {
		gw->addWindowToGroup (NULL, 0);
		if (GroupWindow::get (cw)->mGroup)
		    GroupWindow::get (cw)->mGroup->tabGroup (cw);
	    }
	}

	mWindows.clear ();
    }
    else if (mTabBar)
    {
	delete mTabBar;
	mTabBar = NULL;
    }

    gs->mGroups.remove (this);

    if (this == gs->mLastHoveredGroup)
	gs->mLastHoveredGroup = NULL;
    if (this == gs->mLastRestackedGroup)
	gs->mLastRestackedGroup = NULL;

    delete this;
}

/*
 * GroupSelection::GroupSelection
 * 
 */

GroupSelection::GroupSelection (CompWindow *startingWindow,
				long int initialIdent) :
    mScreen (screen),
    mTabBar (NULL),
    mTabbingState (NoTabbing),
    mUngroupState (UngroupNone),
    mGrabWindow (None),
    mGrabMask (0),
    mResizeInfo (NULL) 
{
    mWindows.push_back (startingWindow);
    
    mPoller.setCallback (boost::bind (&GroupSelection::handleHoverDetection,
				  this, _1));
    
    GROUP_SCREEN (screen);

    /* glow color */
    mColor[0] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    mColor[1] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    mColor[2] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    mColor[3] = 0xffff;

    if (initialIdent)
	mIdentifier = initialIdent;
    else
    {
	/* we got no valid group Id passed, so find out a new valid
	unique one */
	GroupSelection *tg;
	bool           invalidID = false;

	mIdentifier = gs->mGroups.size () ? gs->mGroups.front ()->mIdentifier : 0;
	do
	{
	    invalidID = false;
	    foreach (tg, gs->mGroups)
	    {
		if (tg->mIdentifier == mIdentifier)
		{
		    invalidID = true;

		    mIdentifier++;
		    break;
		}
	    }
	}
	while (invalidID);
    }

    gs->mGroups.push_front (this);
}

/*
 * addWindowToGroup
 *
 */
void
GroupWindow::addWindowToGroup (GroupSelection *group,
				    long int       initialIdent)
{
    if (mGroup)
	return;

    if (group)
    {
	CompWindow *topTab = NULL;
	
	mGroup = group;

	group->mWindows.push_back (window);

	window->updateWindowOutputExtents ();
	updateWindowProperty ();

	if (group->mWindows.size () == 2)
	{
	    /* first window in the group got its glow, too */
	    group->mWindows.front ()->updateWindowOutputExtents ();
	}

	if (group->mTabBar)
	{
	    if (HAS_TOP_WIN (group))
		topTab = TOP_TAB (group);
	    else if (HAS_PREV_TOP_WIN (group))
	    {
		topTab = PREV_TOP_TAB (group);
		group->mTabBar->mTopTab = group->mTabBar->mPrevTopTab;
		group->mTabBar->mPrevTopTab = NULL;
	    }

	    if (topTab)
	    {
		if (!mSlot)
		    group->mTabBar->createSlot (window);

		mDestination = CompPoint (WIN_CENTER_X (topTab) -
					  (WIN_WIDTH (window) / 2),
					  WIN_CENTER_Y (topTab) -
					  (WIN_HEIGHT (window) / 2));

		mMainTabOffset = CompPoint (WIN_X (window),
					    WIN_Y (window)) -
				 mDestination;

		mOrgPos = CompPoint (WIN_X (window), WIN_Y (window));

		mXVelocity = mYVelocity = 0.0f;

		mAnimateState = IS_ANIMATED;

		group->startTabbingAnimation (true);

		cWindow->addDamage ();
	    }
	}
    }
    else
    {
	/* create new group */
	GroupSelection *g = new GroupSelection (window, initialIdent);
	if (!g)
	    return;
	    
	mGroup = g;

	updateWindowProperty ();
    }
}

/*
 * groupWindows
 *
 */
bool
GroupScreen::groupWindows (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector options)
{
    if (mTmpSel.size () > 0)
    {
        CompWindowList::iterator it = mTmpSel.begin ();
	CompWindow	         *cw;
        GroupSelection *group = NULL;
        bool           tabbed = false;

	foreach (cw, mTmpSel)
	{
	    GROUP_WINDOW (cw);

	    if (gw->mGroup)
	    {
	        if (!tabbed || group->mTabBar)
		    group = gw->mGroup;

	        if (group->mTabBar)
		    tabbed = true;
	    }
        }

        /* we need to do one first to get the pointer of a new group */
        cw = *it;
        GROUP_WINDOW (cw);

        if (gw->mGroup && (group != gw->mGroup))
	    gw->deleteGroupWindow ();
        gw->addWindowToGroup (group, 0);
        gw->cWindow->addDamage ();

        gw->mInSelection = false;
        group = gw->mGroup;

        for (; it != mTmpSel.end (); it++)
        {
	    cw = *it;
	    GROUP_WINDOW (cw);

	    if (gw->mGroup && (group != gw->mGroup))
	        gw->deleteGroupWindow ();
	    gw->addWindowToGroup (group, 0);
	    gw->cWindow->addDamage ();

	    gw->mInSelection = false;
        }

        /* exit selection */
        mTmpSel.clear ();
    }

    return false;
}

/*
 * ungroupWindows
 *
 */
bool
GroupScreen::ungroupWindows (CompAction          *action,
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
	    gw->mGroup->fini ();
    }

    return false;
}

/*
 * removeWindow
 *
 */
bool
GroupScreen::removeWindow (CompAction         *action,
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
	    gw->removeWindowFromGroup ();
    }

    return false;
}

/*
 * closeWindows
 *
 */
bool
GroupScreen::closeWindows (CompAction           *action,
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
	    foreach (CompWindow *cw, gw->mGroup->mWindows)
		cw->close (screen->getCurrentTime ());
	}
    }

    return false;
}

/*
 * changeColor
 *
 */
bool
GroupScreen::changeColor (CompAction           *action,
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
	    CompSize size (gw->mGroup->mTabBar->mTopTab->mRegion.boundingRect ().width (),
			   gw->mGroup->mTabBar->mTopTab->mRegion.boundingRect ().height ());

	    color[0] = (int)(rand () / factor);
	    color[1] = (int)(rand () / factor);
	    color[2] = (int)(rand () / factor);

	    gw->mGroup->mTabBar->mSelectionLayer =
	       SelectionLayer::rebuild (gw->mGroup->mTabBar->mSelectionLayer,
					size);
	    if (gw->mGroup->mTabBar->mSelectionLayer)
		gw->mGroup->mTabBar->mSelectionLayer->render ();
	    cScreen->damageScreen ();
	}
    }

    return false;
}

/*
 * setIgnore
 *
 */
bool
GroupScreen::setIgnore (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector options)
{
    mIgnoreMode = true;

    if (state & CompAction::StateInitKey)
	action->setState (state | CompAction::StateTermKey);

    return false;
}

/*
 * unsetIgnore
 *
 */
bool
GroupScreen::unsetIgnore (CompAction          *action,
			       CompAction::State   state,
			       CompOption::Vector  options)
{
    mIgnoreMode = false;

    action->setState (state & ~CompAction::StateTermKey);

    return false;
}

/*
 * handleButtonPressEvent
 *
 */
void
GroupScreen::handleButtonPressEvent (XEvent *event)
{
    GroupSelection *group;
    int            xRoot, yRoot, button;

    xRoot  = event->xbutton.x_root;
    yRoot  = event->xbutton.y_root;
    button = event->xbutton.button;

    foreach (group, mGroups)
    {
	if (!group->mTabBar)
	    continue;

	if (group->mTabBar->mInputPrevention != event->xbutton.window)
	    continue;

	switch (button) {
	case Button1:
	    {
		GroupTabBarSlot *slot;

		foreach (slot, group->mTabBar->mSlots)
		{
		    if (slot->mRegion.contains (CompPoint (xRoot,
							   yRoot)))
		    {
			mDraggedSlot = slot;
			/* The slot isn't dragged yet */
			mDragged = false;
			mPrevX = xRoot;
			mPrevY = yRoot;

			if (!screen->otherGrabExist ("group", "group-drag", NULL))
			    grabScreen (ScreenGrabTabDrag);
		    }
		}
	    }
	    break;

	case Button4:
	case Button5:
	    {
		CompWindow  *topTab = NULL;
		GroupWindow *gw;

		if (group->mTabBar->mNextTopTab)
		    topTab = NEXT_TOP_TAB (group);
		else if (group->mTabBar->mTopTab)
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
			changeTab (gw->mSlot->mPrev, RotateLeft);
		    else
			changeTab (gw->mGroup->mTabBar->mSlots.back (),
					RotateLeft);
		}
		else
		{
		    if (gw->mSlot->mNext)
			changeTab (gw->mSlot->mNext, RotateRight);
		    else
			changeTab (gw->mGroup->mTabBar->mSlots.front (), RotateRight);
		}
		break;
	    }
	}

	break;
    }
}

/*
 * handleButtonReleaseEvent
 *
 */
void
GroupScreen::handleButtonReleaseEvent (XEvent *event)
{
    GroupSelection *group;
    int            vx, vy;
    CompRegion     newRegion;
    bool           inserted = false;
    bool           wasInTabBar = false;

    if (event->xbutton.button != 1)
	return;

    if (!mDraggedSlot)
	return;

    if (!mDragged)
    {
	changeTab (mDraggedSlot, RotateUncertain);
	mDraggedSlot = NULL;

	if (mGrabState == ScreenGrabTabDrag)
	    grabScreen (ScreenGrabNone);

	return;
    }

    GROUP_WINDOW (mDraggedSlot->mWindow);

    newRegion = mDraggedSlot->mRegion;

    mDraggedSlot->getDrawOffset (vx, vy);
    newRegion.translate (vx, vy);

    foreach (group, mGroups)
    {
	bool            inTabBar;
	CompRegion      clip, buf;
	GroupTabBarSlot *slot;

	if (!group->mTabBar || !HAS_TOP_WIN (group))
	    continue;

	/* create clipping region */
	clip = GroupWindow::get (TOP_TAB (group))->getClippingRegion ();

	buf = newRegion.intersected (group->mTabBar->mRegion);
	buf = buf.subtracted (clip);

	inTabBar = !buf.isEmpty ();

	if (!inTabBar)
	    continue;

	wasInTabBar = true;

	foreach (slot, group->mTabBar->mSlots)
	{
	    GroupTabBarSlot *tmpDraggedSlot;
	    GroupSelection  *tmpGroup;
	    CompRegion      slotRegion;
	    CompRect	    rect;
	    bool            inSlot;

	    if (slot == mDraggedSlot)
		continue;

	    if (slot->mPrev && slot->mPrev != mDraggedSlot)
	    {
		rect.setX (slot->mPrev->mRegion.boundingRect ().x2 ());
	    }
	    else if (slot->mPrev && slot->mPrev == mDraggedSlot &&
		     mDraggedSlot->mPrev)
	    {
		rect.setX (mDraggedSlot->mPrev->mRegion.boundingRect ().x2 ());
	    }
	    else
		rect.setX (group->mTabBar->mRegion.boundingRect ().x1 ());

	    rect.setY (slot->mRegion.boundingRect ().y1 ());

	    if (slot->mNext && slot->mNext != mDraggedSlot)
	    {
		rect.setWidth (slot->mNext->mRegion.boundingRect ().x1 () - rect.x ());
	    }
	    else if (slot->mNext && slot->mNext == mDraggedSlot &&
		     mDraggedSlot->mNext)
	    {
		rect.setWidth (mDraggedSlot->mNext->mRegion.boundingRect ().x1 () - rect.x ());
	    }
	    else
		rect.setWidth (group->mTabBar->mRegion.boundingRect ().x2 ());

	    rect.setY (slot->mRegion.boundingRect ().y1 ());
	    rect.setHeight (slot->mRegion.boundingRect ().y2 () - slot->mRegion.boundingRect ().y1 ());

	    slotRegion = CompRegion (rect);

	    inSlot = slotRegion.intersects (newRegion);

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
		if (tmpGroup->mTabBar->mTopTab && !IS_TOP_TAB (w, tmpGroup))
		{
		    CompWindow *tw = TOP_TAB (tmpGroup);

		    oldPosX = WIN_CENTER_X (tw) + gw->mMainTabOffset.x ();
		    oldPosY = WIN_CENTER_Y (tw) + gw->mMainTabOffset.y ();

		    GroupWindow::get (w)->setWindowVisibility (true);
		}

		/* Change the group. */
		GroupWindow::get (mDraggedSlot->mWindow)->deleteGroupWindow ();
		GroupWindow::get (mDraggedSlot->mWindow)->addWindowToGroup (group, 0);

		/* we saved the original center position in oldPosX/Y before -
		   now we should apply that to the new main tab offset */
		if (HAS_TOP_WIN (group))
		{
		    CompWindow *tw = TOP_TAB (group);
		    gw->mMainTabOffset.setX (oldPosX - WIN_CENTER_X (tw));
		    gw->mMainTabOffset.setY (oldPosY - WIN_CENTER_Y (tw));
		}
	    }
	    else
		group->mTabBar->unhookTabBarSlot (mDraggedSlot, true);

	    mDraggedSlot = NULL;
	    mDragged = false;
	    inserted = true;

	    if ((tmpDraggedSlot->mRegion.boundingRect ().x1 () +
		 tmpDraggedSlot->mRegion.boundingRect ().x2 () + (2 * vx)) / 2 >
		(slot->mRegion.boundingRect ().x1 () + slot->mRegion.boundingRect ().x2 ()) / 2)
	    {
		group->mTabBar->insertTabBarSlotAfter (tmpDraggedSlot, slot);
	    }
	    else
		group->mTabBar->insertTabBarSlotBefore (tmpDraggedSlot, slot);

	    group->mTabBar->damageRegion ();

	    /* Hide tab-bars. */
	    foreach (tmpGroup, mGroups)
	    {
		if (group == tmpGroup)
		    tmpGroup->tabSetVisibility (true, 0);
		else
		    tmpGroup->tabSetVisibility (false, PERMANENT);
	    }

	    break;
	}

	if (inserted)
	    break;
    }

    if (!inserted)
    {
	CompWindow     *draggedSlotWindow = mDraggedSlot->mWindow;
	GroupSelection *tmpGroup;

	foreach (tmpGroup, mGroups)
	    tmpGroup->tabSetVisibility (false, PERMANENT);

	mDraggedSlot = NULL;
	mDragged = false;

	if (optionGetDndUngroupWindow () && !wasInTabBar)
	{
	    GroupWindow::get(draggedSlotWindow)->removeWindowFromGroup ();
	}
	else if (gw->mGroup && gw->mGroup->mTabBar->mTopTab)
	{
	    gw->mGroup->mTabBar->recalcTabBarPos ((gw->mGroup->mTabBar->mRegion.boundingRect ().x1 () +
				   gw->mGroup->mTabBar->mRegion.boundingRect ().x2 ()) / 2,
				  gw->mGroup->mTabBar->mRegion.boundingRect ().x1 (),
				  gw->mGroup->mTabBar->mRegion.boundingRect ().x2 ());
	}

	/* to remove the painted slot */
	cScreen->damageScreen ();
    }

    if (mGrabState == ScreenGrabTabDrag)
	grabScreen (ScreenGrabNone);

    if (mDragHoverTimeoutHandle.active ())
    {
	mDragHoverTimeoutHandle.stop ();
    }
}

/*
 * handleMotionEvent
 *
 */

/* the radius to determine if it was a click or a drag */
#define RADIUS 5

void
GroupScreen::handleMotionEvent (int xRoot,
				     int yRoot)
{
    if (mGrabState == ScreenGrabTabDrag)
    {
	int    dx, dy;
	int    vx, vy;
	int    x1, x2, y1, y2;
	CompRegion &draggedRegion = mDraggedSlot->mRegion;

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

		GROUP_WINDOW (mDraggedSlot->mWindow);

		mDragged = true;

		foreach (group, mGroups)
		    group->tabSetVisibility (true, PERMANENT);

		CompRect box = gw->mGroup->mTabBar->mRegion.boundingRect ();
		gw->mGroup->mTabBar->recalcTabBarPos ((box.x1 () + box.x2 ()) / 2,
				      box.x1 (), box.x2 ());
	    }

	    mDraggedSlot->getDrawOffset (vx, vy);

	    x1 = draggedRegion.boundingRect ().x1 () + vx;
	    y1 = draggedRegion.boundingRect ().y1 () + vy;
	    x2 = draggedRegion.boundingRect ().x2 () + vx;
	    y2 = draggedRegion.boundingRect ().y2 () + vy;

	    cReg = CompRegion (x1, y1,
			       x2 - x1,
			       y2 - y1);

	    cScreen->damageRegion (cReg);

	    mDraggedSlot->mRegion.translate (dx, dy);
	    mDraggedSlot->mSpringX =
		(mDraggedSlot->mRegion.boundingRect ().x1 () +
		 mDraggedSlot->mRegion.boundingRect ().x2 ()) / 2;

	    x1 = draggedRegion.boundingRect ().x1 () + vx;
	    y1 = draggedRegion.boundingRect ().y1 () + vy;
	    x2 = draggedRegion.boundingRect ().x2 () + vx;
	    y2 = draggedRegion.boundingRect ().y2 () + vy;

	    cReg = CompRegion (x1, y1,
			       x2 - x1,
			       y2 - y1);

	    cScreen->damageRegion (cReg);
	}
    }
    else if (mGrabState == ScreenGrabSelect)
    {
	mTmpSel.damage (xRoot, yRoot);
    }
}

/*
 * handleEvent
 *
 */
void
GroupScreen::handleEvent (XEvent      *event)
{
    CompWindow *w;

    switch (event->type) {
    case MotionNotify:
	handleMotionEvent (pointerX, pointerY);
	break;

    case ButtonPress:
	handleButtonPressEvent (event);
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
		    gw->mWindowState = GroupWindow::WindowShaded;

		    if (gw->mGroup && optionGetShadeAll ())
			gw->mGroup->shadeWindows (w, true);
		}
		else if (w->minimized ())
		{
		    gw->mWindowState = GroupWindow::WindowMinimized;

		    if (gw->mGroup && optionGetMinimizeAll ())
			gw->mGroup->minimizeWindows (w, true);
		}
	    }

	    if (gw->mGroup)
	    {
		if (gw->mGroup->mTabBar && IS_TOP_TAB (w, gw->mGroup))
		{
		    /* on unmap of the top tab, hide the tab bar and the
		       input prevention window */
		    gw->mGroup->tabSetVisibility (false, PERMANENT);
		}
		if (!w->pendingUnmaps ())
		{
		    /* close event */
		    if (!(gw->mAnimateState & IS_UNGROUPING))
		    {
			gw->deleteGroupWindow ();
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
		    gw->mGroup->mTabBar->mCheckFocusAfterTabChange = true;
		    changeTab (gw->mSlot, RotateUncertain);
		}
	    }
	}
	else if (event->xclient.message_type == mResizeNotifyAtom)
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xclient.window);

	    if (!w)
		break;
		
	    foreach (GroupSelection *group, mGroups)
	    {
		if (!(group->mResizeInfo &&
		      w == group->mResizeInfo->mResizedWindow))
		      continue;

		if (group)
		{			
		    CompRect rect (event->xclient.data.l[0],
			      	   event->xclient.data.l[1],
			      	   event->xclient.data.l[2],
			      	   event->xclient.data.l[3]);

		    group->prepareResizeWindows (rect);
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
			gw->clearWindowInputShape (gw->mWindowHideInfo);
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
		    gw->mGroup->mTabBar->mTextLayer = 
		        TextLayer::rebuild (gw->mGroup->mTabBar->mTextLayer);

		    if (gw->mGroup->mTabBar->mTextLayer)
			gw->mGroup->mTabBar->mTextLayer->render ();
		    gw->mGroup->mTabBar->damageRegion ();
		}
	    }
	}
	break;

    case EnterNotify:
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xcrossing.window);
	    
	    updateTabBars (event->xcrossing.window);
	    
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
				    &GroupWindow::dragHoverTimeout,
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
		    gw->mGroup->mTabBar->mInputPrevention &&
		    gw->mGroup->mTabBar->mIpwMapped)
		{
		    XWindowChanges xwc;

		    xwc.stack_mode = Above;
		    xwc.sibling = w->id ();

		    XConfigureWindow (screen->dpy (),
				      gw->mGroup->mTabBar->mInputPrevention,
				      CWSibling | CWStackMode, &xwc);
		}

		if (event->xconfigure.above != None)
		{
		    if (gw->mGroup && !gw->mGroup->mTabBar &&
			(gw->mGroup != mLastRestackedGroup))
		    {
			if (optionGetRaiseAll ())
			    gw->mGroup->raiseWindows (w);
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

    if (mGroup && mGroup->mWindows.size () > 1)
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

    if (!mResizeGeometry.isEmpty ())
    {
	mResizeGeometry = CompRect (0, 0, 0, 0);
    }

    window->resizeNotify (dx, dy, dwidth, dheight);

    if (mGlowQuads)
    {
	/* FIXME: we need to find a more multitexture friendly way
	 * of doing this */
	GLTexture::Matrix tMat = gs->mGlowTexture.at (0)->matrix ();
	computeGlowQuads (&tMat);
    }

    if (mGroup && mGroup->mTabBar && IS_TOP_TAB (window, mGroup))
    {
	if (mGroup->mTabBar->mState != PaintOff)
	{
	    mGroup->mTabBar->recalcTabBarPos (pointerX,
				  WIN_X (window), WIN_X (window) + WIN_WIDTH (window));
	}
    }
}

/*
 * GroupWindow::moveNotify
 *
 */
void
GroupWindow::moveNotify (int    dx,
			 int    dy,
			 bool   immediate)
{
    bool       viewportChange;

    GROUP_SCREEN (screen);

    window->moveNotify (dx, dy, immediate);

    if (mGlowQuads)
    {
	/* FIXME: we need to find a more multitexture friendly way
	 * of doing this */
	GLTexture::Matrix tMat = gs->mGlowTexture.at (0)->matrix ();
	computeGlowQuads (&tMat);
    }

    if (!mGroup || gs->mQueued)
	return;

    /* FIXME: we need a reliable, 100% safe way to detect window
       moves caused by viewport changes here */
    viewportChange = ((dx && !(dx % screen->width ())) ||
		      (dy && !(dy % screen->height ())));

    if (viewportChange && (mAnimateState & IS_ANIMATED))
    {
	mDestination += CompPoint (dx, dy);
    }

    if (mGroup->mTabBar && IS_TOP_TAB (window, mGroup))
    {
	GroupTabBarSlot *slot;
	GroupTabBar     *bar = mGroup->mTabBar;

	bar->mRightSpringX += dx;
	bar->mLeftSpringX += dx;

	bar->moveTabBarRegion (dx, dy, true);

	foreach (slot, bar->mSlots)
	{
	    slot->mRegion.translate (dx, dy);
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

    mGroup->moveWindows (window, dx, dy, immediate, viewportChange);
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
	bool doResizeAll;

	doResizeAll = gs->optionGetResizeAll () &&
	              (mask & CompWindowGrabResizeMask);

	if (mGroup->mTabBar)
	    mGroup->tabSetVisibility (false, 0);

	foreach (CompWindow *cw, mGroup->mWindows)
	{
	    if (!cw)
		continue;

	    if (cw->id () != window->id ())
	    {
		GroupWindow *gcw = GroupWindow::get (cw);

		gcw->enqueueGrabNotify (x, y, state, mask);

		if (doResizeAll && !(cw->state () & MAXIMIZE_STATE))
		{
		    if (gcw->mResizeGeometry.isEmpty ())
		    {
			gcw->mResizeGeometry = CompRect (WIN_X (cw),
							 WIN_Y (cw),
							 WIN_WIDTH (cw),
							 WIN_HEIGHT (cw));
		    }
		}
	    }
	}

	if (doResizeAll)
	{
	    if (!mGroup->mResizeInfo)
		mGroup->mResizeInfo = (GroupResizeInfo *) malloc (sizeof (GroupResizeInfo));

	    if (mGroup->mResizeInfo)
	    {
		mGroup->mResizeInfo->mResizedWindow       = window;
		mGroup->mResizeInfo->mOrigGeometry = CompRect (WIN_X (window),
								WIN_Y (window),
								WIN_WIDTH (window),
								WIN_HEIGHT (window));
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
	mGroup->resizeWindows (window); // should really include the size info here
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
	if (gs->optionGetAutotabCreate () && isGroupWindow ())
	{
	    if (!mGroup && (mWindowState == WindowNormal))
	    {
		addWindowToGroup (NULL, 0);
		if (GroupWindow::get (window)->mGroup)
		    GroupWindow::get (window)->mGroup->tabGroup (window);
	    }
	}

	if (mGroup)
	{
	    if (mWindowState == WindowMinimized)
	    {
		if (gs->optionGetMinimizeAll ())
		    mGroup->minimizeWindows (window, false);
	    }
	    else if (mWindowState == WindowShaded)
	    {
		if (gs->optionGetShadeAll ())
		    mGroup->shadeWindows (window, false);
	    }
	}

	mWindowState = WindowNormal;
    }

    if (!mResizeGeometry.isEmpty ())
    {
	CompRect box;
	float  dummy = 1;

	getStretchRectangle (box, dummy, dummy);
	gs->damagePaintRectangle (box);
    }

    if (mSlot)
    {
	int    vx, vy;
	CompRegion reg;

	mSlot->getDrawOffset (vx, vy);
	if (vx || vy)
	{
	    reg = reg.united (mSlot->mRegion);
	    reg.translate (vx, vy);
	}
	else
	    reg = mSlot->mRegion;

	gs->cScreen->damageRegion (reg);
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
	    mGroup->maximizeWindows (window);
	}
    }

    window->stateChangeNotify (lastState);
}

void
GroupWindow::activate ()
{
    GROUP_SCREEN (screen);

    if (mGroup && mGroup->mTabBar && !IS_TOP_TAB (window, mGroup))
	gs->changeTab (mSlot, RotateUncertain);

    window->activate ();
}
