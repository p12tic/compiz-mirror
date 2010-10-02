/**
 *
 * Compiz group plugin
 *
 * tab.c
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
 * getCurrentMousePosition
 *
 * Description:
 * Return the current function of the pointer at the given screen.
 * The position is queried trough XQueryPointer directly from the xserver.
 *
 */
bool
GroupScreen::getCurrentMousePosition (int &x, int &y)
{
    MousePoller poller;
    CompPoint pos = poller.getCurrentPosition ();
    
    x = pos.x ();
    y = pos.y ();

    return (x != 0 && y != 0);
}

/*
 * getClippingRegion
 *
 * Description:
 * This function returns a clipping region which is used to clip
 * several events involving window stack such as hover detection
 * in the tab bar or Drag'n'Drop. It creates the clipping region
 * with getting the region of every window above the given window
 * and then adds this region to the clipping region using
 * XUnionRectWithRegion. w->region won't work since it doesn't include
 * the window decoration.
 *
 */
CompRegion
GroupWindow::getClippingRegion ()
{
    CompWindow *cw;
    CompRegion     clip;

    for (cw = window->next; cw; cw = cw->next)
    {
	if (!cw->invisible () && !(cw->state () & CompWindowStateHiddenMask))
	{
	    CompRect rect;
	    CompRegion     buf;

	    rect = CompRect (WIN_REAL_X (cw),
			     WIN_REAL_Y (cw),
			     WIN_REAL_WIDTH (cw),
			     WIN_REAL_HEIGHT (cw));

	    buf = buf.united (rect);
	    clip = buf.united (clip);
	}
    }

    return clip;
}


/*
 * clearWindowInputShape
 *
 */
void
GroupWindow::clearWindowInputShape (GroupWindowHideInfo *hideInfo)
{
    XRectangle  *rects;
    int         count = 0, ordering;

    rects = XShapeGetRectangles (screen->dpy (), window->id (), ShapeInput,
				 &count, &ordering);

    if (count == 0)
	return;

    /* check if the returned shape exactly matches the window shape -
       if that is true, the window currently has no set input shape */
    if ((count == 1) &&
	(rects[0].x == -window->serverGeometry ().border ()) &&
	(rects[0].y == -window->serverGeometry ().border ()) &&
	(rects[0].width == (window->serverWidth () +
			    window->serverGeometry ().border ())) &&
	(rects[0].height == (window->serverHeight () +
			     window->serverGeometry ().border ())))
    {
	count = 0;
    }

    if (hideInfo->mInputRects)
	XFree (hideInfo->mInputRects);

    hideInfo->mInputRects = rects;
    hideInfo->mNInputRects = count;
    hideInfo->mInputRectOrdering = ordering;

    XShapeSelectInput (screen->dpy (), hideInfo->mShapeWindow, NoEventMask);

    XShapeCombineRectangles (screen->dpy (), hideInfo->mShapeWindow, ShapeInput, 0, 0,
			     NULL, 0, ShapeSet, 0);

    XShapeSelectInput (screen->dpy (), hideInfo->mShapeWindow, ShapeNotify);
}

/*
 * setWindowVisibility
 *
 */
void
GroupWindow::setWindowVisibility (bool visible)
{
    if (!visible && !mWindowHideInfo)
    {
	GroupWindowHideInfo *info;

	mWindowHideInfo = info = (GroupWindowHideInfo *) malloc (sizeof (GroupWindowHideInfo));
	if (!mWindowHideInfo)
	    return;

	info->mInputRects = NULL;
	info->mNInputRects = 0;
	info->mShapeMask = XShapeInputSelected (screen->dpy (), window->id ());

	/* We are a reparenting window manager now, which means that we either
	 * shape the frame window, or if it does not exist, shape the window
	 */
	
	if (window->frame ())
	{
	    info->mShapeWindow = window->frame ();
	}
	else
	    info->mShapeWindow = window->id ();

	clearWindowInputShape (info);

	info->mSkipState = window->state () & (CompWindowStateSkipPagerMask |
				              CompWindowStateSkipTaskbarMask);

	window->changeState (window->state () |
			   CompWindowStateSkipPagerMask |
			   CompWindowStateSkipTaskbarMask);
    }
    else if (visible && mWindowHideInfo)
    {
	GroupWindowHideInfo *info = mWindowHideInfo;

	if (info->mNInputRects)
	{
	    XShapeCombineRectangles (screen->dpy (), info->mShapeWindow, ShapeInput, 
				     0, 0, info->mInputRects, info->mNInputRects,
				     ShapeSet, info->mInputRectOrdering);
	}
	else
	{
	    XShapeCombineMask (screen->dpy (), info->mShapeWindow, ShapeInput,
			       0, 0, None, ShapeSet);
	}

	if (info->mInputRects)
	    XFree (info->mInputRects);

	XShapeSelectInput (screen->dpy (), info->mShapeWindow, info->mShapeMask);

	window->changeState ((window->state () &
				 ~(CompWindowStateSkipPagerMask |
				   CompWindowStateSkipTaskbarMask)) |
			      info->mSkipState);

	free (info);
	mWindowHideInfo = NULL;
    }
}

/*
 * groupTabBarTimeout
 *
 * Description:
 * This function is called when the time expired (== timeout).
 * We use this to realize a delay with the bar hiding after tab change.
 * handleAnimation sets up a timer after the animation has finished.
 * This function itself basically just sets the tab bar to a PaintOff status
 * through calling groupSetTabBarVisibility.
 * The PERMANENT mask allows you to force hiding even of
 * PaintPermanentOn tab bars.
 *
 */
bool
GroupSelection::tabBarTimeout ()
{
    tabSetVisibility (false, PERMANENT);

    return false;	/* This will free the timer. */
}

/*
 * groupShowDelayTimeout
 *
 */
bool
GroupSelection::showDelayTimeout ()
{
    int            mouseX, mouseY;
    CompWindow     *topTab;
    
    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (this))
    {
	gs->mShowDelayTimeoutHandle.stop ();
	return false;	/* This will free the timer. */
    }

    topTab = TOP_TAB (this);

    gs->getCurrentMousePosition (mouseX, mouseY);

    mTabBar->recalcTabBarPos (mouseX, WIN_REAL_X (topTab),
			  WIN_REAL_X (topTab) + WIN_REAL_WIDTH (topTab));

    tabSetVisibility (true, 0);

    gs->mShowDelayTimeoutHandle.stop ();
    return false;	/* This will free the timer. */
}

/*
 * GroupSelection::tabSetVisibility
 *
 * Description:
 * This function is used to set the visibility of the tab bar.
 * The "visibility" is indicated through the PaintState, which
 * can be PaintOn, PaintOff, PaintFadeIn, PaintFadeOut
 * and PaintPermantOn.
 * Currently the mask paramater is mostely used for the PERMANENT mask.
 * This mask affects how the visible parameter is handled, for example if
 * visibule is set to true and the mask to PERMANENT state it will set
 * PaintPermanentOn state for the tab bar. When visibile is false, mask 0
 * and the current state of the tab bar is PaintPermanentOn it won't do
 * anything because its not strong enough to disable a
 * Permanent-State, for those you need the mask.
 *
 */
void
GroupSelection::tabSetVisibility (bool           visible,
				  unsigned int   mask)
{
    GroupTabBar *bar;
    CompWindow  *topTab;
    PaintState  oldState;
    
    GROUP_SCREEN (screen);

    if (!mWindows.size () || !mTabBar || !HAS_TOP_WIN (this))
	return;

    bar = mTabBar;
    topTab = TOP_TAB (this);
    oldState = bar->mState;

    if (visible)
	mPoller.start ();
    else
	mPoller.stop ();

    /* hide tab bars for invisible top windows */
    if ((topTab->state () & CompWindowStateHiddenMask) || topTab->invisible ())
    {
	bar->mState = PaintOff;
	gs->switchTopTabInput (this, true);
    }
    else if (visible && bar->mState != PaintPermanentOn && (mask & PERMANENT))
    {
	bar->mState = PaintPermanentOn;
	gs->switchTopTabInput (this, false);
    }
    else if (visible && bar->mState == PaintPermanentOn && !(mask & PERMANENT))
    {
	bar->mState = PaintOn;
    }
    else if (visible && (bar->mState == PaintOff || bar->mState == PaintFadeOut))
    {
	if (gs->optionGetBarAnimations ())
	{
	    bar->mBgAnimation = AnimationReflex;
	    bar->mBgAnimationTime = gs->optionGetReflexTime () * 1000.0;
	}
	bar->mState = PaintFadeIn;
	gs->switchTopTabInput (this, false);
    }
    else if (!visible &&
	     (bar->mState != PaintPermanentOn || (mask & PERMANENT)) &&
	     (bar->mState == PaintOn || bar->mState == PaintPermanentOn ||
	      bar->mState == PaintFadeIn))
    {
	bar->mState = PaintFadeOut;
	gs->switchTopTabInput (this, true);
    }

    if (bar->mState == PaintFadeIn || bar->mState == PaintFadeOut)
	bar->mAnimationTime = (gs->optionGetFadeTime () * 1000) - bar->mAnimationTime;

    if (bar->mState != oldState)
	bar->damageRegion ();
}

/*
 * GroupTabBarSlot::getDrawOffset ()
 *
 * Description:
 * Its used when the draggedSlot is dragged to another viewport.
 * It calculates a correct offset to the real slot position.
 *
 */
void
GroupTabBarSlot::getDrawOffset (int &hoffset,
				int &voffset)
{
    CompWindow *w, *topTab;
    int        x, y, vx, vy;
    CompPoint  vp;
    CompWindow::Geometry winGeometry;

    if (!mWindow)
	return;

    w = mWindow;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    if (this != gs->mDraggedSlot || !gw->mGroup)
    {
	hoffset = 0;
	voffset = 0;

	return;
    }

    if (HAS_TOP_WIN (gw->mGroup))
	topTab = TOP_TAB (gw->mGroup);
    else if (HAS_PREV_TOP_WIN (gw->mGroup))
	topTab = PREV_TOP_TAB (gw->mGroup);
    else
    {
	hoffset = 0;
	voffset = 0;
	return;
    }

    x = WIN_CENTER_X (topTab) - WIN_WIDTH (w) / 2;
    y = WIN_CENTER_Y (topTab) - WIN_HEIGHT (w) / 2;

    winGeometry = CompWindow::Geometry (x, y, w->serverWidth (),
					      w->serverHeight (),
					w->serverGeometry ().border ());

    screen->viewportForGeometry (winGeometry, vp);

    vx = vp.x ();
    vy = vp.y ();

    hoffset = ((screen->vp ().x () - vx) % screen->vpSize ().width ()) * screen->width ();

    voffset = ((screen->vp ().y () - vy) % screen->vpSize ().height ()) * screen->height ();
}

/*
 * GroupSelection::handleHoverDetection
 *
 * Description:
 * This function is called from groupPreparePaintScreen to handle
 * the hover detection. This is needed for the text showing up,
 * when you hover a thumb on the thumb bar.
 *
 * FIXME: we should better have a timer for that ...
 */
void
GroupSelection::handleHoverDetection (const CompPoint &p)
{
    GroupTabBar *bar = mTabBar;
    CompWindow  *topTab = TOP_TAB (this);
    bool        inLastSlot;
    
    GROUP_SCREEN (screen);
    
    if ((bar->mState != PaintOff) && !HAS_TOP_WIN (this))
	return;

    /* then check if the mouse is in the last hovered slot --
       this saves a lot of CPU usage */
    inLastSlot = bar->mHoveredSlot &&
		 bar->mHoveredSlot->mRegion.contains (p);

    if (!inLastSlot)
    {
	CompRegion      clip;
	GroupTabBarSlot *slot;

	bar->mHoveredSlot = NULL;
	clip = GroupWindow::get (topTab)->getClippingRegion ();

	foreach (slot, bar->mSlots)
	{
	    /* We need to clip the slot region with the clip region first.
	       This is needed to respect the window stack, so if a window
	       covers a port of that slot, this part won't be used
	       for in-slot-detection. */
	    CompRegion reg = slot->mRegion.subtracted (clip);

	    if (reg.contains (p))
	    {
		bar->mHoveredSlot = slot;
		break;
	    }
	}

	if (bar->mTextLayer)
	{
	    /* trigger a FadeOut of the text */
	    if ((bar->mHoveredSlot != bar->mTextSlot) &&
		(bar->mTextLayer->mState == PaintFadeIn ||
		 bar->mTextLayer->mState == PaintOn))
	    {
		bar->mTextLayer->mAnimationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    bar->mTextLayer->mAnimationTime;
		bar->mTextLayer->mState = PaintFadeOut;
	    }

	    /* or trigger a FadeIn of the text */
	    else if ((bar->mTextLayer->mState == PaintFadeOut  ||
		      bar->mTextLayer->mState == PaintOff) &&
		      bar->mHoveredSlot == bar->mTextSlot && bar->mHoveredSlot)
	    {
		bar->mTextLayer->mAnimationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    bar->mTextLayer->mAnimationTime;
		bar->mTextLayer->mState = PaintFadeIn;
	    }
	}
    }
    
    return;
}

/*
 * GroupTabBar::handleTabBarFade
 *
 * Description:
 * This function is called from groupPreparePaintScreen
 * to handle the tab bar fade. It checks the animationTime and updates it,
 * so we can calculate the alpha of the tab bar in the painting code with it.
 *
 */
void
GroupTabBar::handleTabBarFade (int		   msSinceLastPaint)
{
    mAnimationTime -= msSinceLastPaint;

    if (mAnimationTime < 0)
	mAnimationTime = 0;

    /* Fade finished */
    if (mAnimationTime == 0)
    {
	if (mState == PaintFadeIn)
	{
	    mState = PaintOn;
	}
	else if (mState == PaintFadeOut)
	{
	    mState = PaintOff;

	    if (mTextLayer)
	    {
		/* Tab-bar is no longer painted, clean up
		   text animation variables. */
		mTextLayer->mAnimationTime = 0;
		mTextLayer->mState = PaintOff;
		mTextSlot = mHoveredSlot = NULL;

		mTextLayer = TextLayer::rebuild (mTextLayer);
	
		if (mTextLayer)
		    mTextLayer->render ();
	    }
	}
    }
}

/*
 * GroupTabBar::handleTextFade
 *
 * Description:
 * This function is called from groupPreparePaintScreen
 * to handle the text fade. It checks the animationTime and updates it,
 * so we can calculate the alpha of the text in the painting code with it.
 *
 */
void
GroupTabBar::handleTextFade (int	       msSinceLastPaint)
{
    TextLayer *textLayer = mTextLayer;

    /* Fade in progress... */
    if ((textLayer->mState == PaintFadeIn || textLayer->mState == PaintFadeOut) &&
	textLayer->mAnimationTime > 0)
    {
	textLayer->mAnimationTime -= msSinceLastPaint;

	if (textLayer->mAnimationTime < 0)
	    textLayer->mAnimationTime = 0;

	/* Fade has finished. */
	if (textLayer->mAnimationTime == 0)
	{
	    if (textLayer->mState == PaintFadeIn)
		textLayer->mState = PaintOn;

	    else if (textLayer->mState == PaintFadeOut)
		textLayer->mState = PaintOff;
	}
    }

    if (textLayer->mState == PaintOff && mHoveredSlot)
    {
	/* Start text animation for the new hovered slot. */
	mTextSlot = mHoveredSlot;
	textLayer->mState = PaintFadeIn;
	textLayer->mAnimationTime =
	    (GroupScreen::get (screen)->optionGetFadeTextTime () * 1000);

	mTextLayer = textLayer = TextLayer::rebuild (textLayer);
	
	if (textLayer)
	    mTextLayer->render ();
    }

    else if (textLayer->mState == PaintOff && mTextSlot)
    {
	/* Clean Up. */
	mTextSlot = NULL;
	mTextLayer = textLayer = TextLayer::rebuild (textLayer);
	
	if (textLayer)
	    mTextLayer->render ();
    }
}

/*
 * GroupTabBar::handleTabBarAnimation
 *
 * Description: Handles the different animations for the tab bar defined in
 * GroupAnimationType. Basically that means this function updates
 * tabBar->animation->time as well as checking if the animation is already
 * finished.
 *
 */
void
GroupTabBar::handleTabBarAnimation (int            msSinceLastPaint)
{
    mBgAnimationTime -= msSinceLastPaint;

    if (mBgAnimationTime <= 0)
    {
	mBgAnimationTime = 0;
	mBgAnimation = AnimationNone;

	mBgLayer->render ();
    }
}
/*
 * tabChangeActivateEvent
 *
 * Description: Creates a compiz event to let other plugins know about
 * the starting and ending point of the tab changing animation
 */
void
GroupScreen::tabChangeActivateEvent (bool activating)
{
    CompOption::Vector o;

    CompOption opt ("root", CompOption::TypeInt);
    opt.value ().set ((int) screen->root ());

    o.push_back (opt);

    CompOption opt2 ("active", CompOption::TypeBool);
    opt2.value ().set (activating);

    o.push_back (opt2);

    screen->handleCompizEvent ("group", "tabChangeActivate", o);
}

/*
 * GroupSelection::handleAnimation
 *
 * Description:
 * This function handles the change animation. It's called
 * from handleChanges. Don't let the changeState
 * confuse you, PaintFadeIn equals with the start of the
 * rotate animation and PaintFadeOut is the end of these
 * animation.
 *
 */
void
GroupSelection::handleAnimation ()
{
    GROUP_SCREEN (screen);
	
    if (mTabBar->mChangeState == GroupTabBar::TabChangeOldOut)
    {
	CompWindow      *top = TOP_TAB (this);
	bool            activate;

	/* recalc here is needed (for y value)! */
	mTabBar->recalcTabBarPos ((mTabBar->mRegion.boundingRect ().x1 () +
			  mTabBar->mRegion.boundingRect ().x2 ()) / 2,
			  WIN_REAL_X (top),
			  WIN_REAL_X (top) + WIN_REAL_WIDTH (top));

	mTabBar->mChangeAnimationTime += gs->optionGetChangeAnimationTime () * 500;

	if (mTabBar->mChangeAnimationTime <= 0)
	    mTabBar->mChangeAnimationTime = 0;

	mTabBar->mChangeState = GroupTabBar::TabChangeNewIn;

	activate = !mTabBar->mCheckFocusAfterTabChange;
	if (!activate)
	{
/*
	    CompFocusResult focus;
	    focus    = allowWindowFocus (top, NO_FOCUS_MASK, s->x, s->y, 0);
	    activate = focus == CompFocusAllowed;
*/
	}

	if (activate)
	    top->activate ();

	mTabBar->mCheckFocusAfterTabChange = false;
    }

    if (mTabBar->mChangeState == GroupTabBar::TabChangeNewIn &&
	mTabBar->mChangeAnimationTime <= 0)
    {
	int oldChangeAnimationTime = mTabBar->mChangeAnimationTime;

	gs->tabChangeActivateEvent (false);

	if (mTabBar->mPrevTopTab)
	    GroupWindow::get (PREV_TOP_TAB (this))->setWindowVisibility (false);

	mTabBar->mPrevTopTab = mTabBar->mTopTab;
	mTabBar->mChangeState = GroupTabBar::NoTabChange;

	if (mTabBar->mNextTopTab)
	{
	    GroupTabBarSlot *next = mTabBar->mNextTopTab;
	    mTabBar->mNextTopTab = NULL;

	    gs->changeTab (next, mTabBar->mNextDirection);

	    if (mTabBar->mChangeState == GroupTabBar::TabChangeOldOut)
	    {
		/* If a new animation was started. */
		mTabBar->mChangeAnimationTime += oldChangeAnimationTime;
	    }
	}

	if (mTabBar->mChangeAnimationTime <= 0)
	{
	    mTabBar->mChangeAnimationTime = 0;
	}
	else if (gs->optionGetVisibilityTime () != 0.0f &&
		 mTabBar->mChangeState == GroupTabBar::NoTabChange)
	{
	    tabSetVisibility (true, PERMANENT |
				    SHOW_BAR_INSTANTLY_MASK);

	    if (mTabBar->mTimeoutHandle.active ())
		mTabBar->mTimeoutHandle.stop ();

	    mTabBar->mTimeoutHandle.setTimes (gs->optionGetVisibilityTime () * 1000,
					      gs->optionGetVisibilityTime () * 1200);

	    mTabBar->mTimeoutHandle.setCallback (
			   boost::bind (&GroupSelection::tabBarTimeout,
					this));

	    mTabBar->mTimeoutHandle.start ();
	}
    }
}

/* adjust velocity for each animation step (adapted from the scale plugin) */
int
GroupWindow::adjustTabVelocity ()
{
    float dx, dy, adjust, amount;
    float x1, y1;

    x1 = mDestination.x ();
    y1 = mDestination.y ();

    dx = x1 - (mOrgPos.x () + mTx);
    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    mXVelocity = (amount * mXVelocity + adjust) / (amount + 1.0f);

    dy = y1 - (mOrgPos.y () + mTy);
    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    mYVelocity = (amount * mYVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (mXVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (mYVelocity) < 0.2f)
    {
	mXVelocity = mYVelocity = 0.0f;
	mTx = x1 - window->serverX ();
	mTy = y1 - window->serverY ();

	return 0;
    }
    return 1;
}

void
GroupSelection::finishTabbing ()
{
    GROUP_SCREEN (screen);

    /* Complete untabbing case, delete the tab bar */
    if (mTabbingState == Untabbing &&
	mUngroupState != UngroupSingle)
    {
	delete mTabBar;
	mTabBar = NULL;
    }
	
    mTabbingState = NoTabbing;
    
    gs->tabChangeActivateEvent (false);

    if (mTabBar)
    {
	/* tabbing case - hide all non-toptab windows */
	GroupTabBarSlot *slot;

	foreach (slot, mTabBar->mSlots)
	{
	    CompWindow *w = slot->mWindow;
	    if (!w)
		continue;

	    GROUP_WINDOW (w);

	    if (slot == mTabBar->mTopTab || (gw->mAnimateState & IS_UNGROUPING))
		continue;

	    gw->setWindowVisibility (false);
	}
	mTabBar->mPrevTopTab = mTabBar->mTopTab;
    }

    for (CompWindowList::iterator it = mWindows.begin ();
	 it != mWindows.end ();
	 it++)
    {
	CompWindow *w = *it;
	GROUP_WINDOW (w);

	/* move window to target position */
	gs->mQueued = true;
	w->move (gw->mDestination.x () - WIN_X (w),
		 gw->mDestination.y () - WIN_Y (w), true);
	gs->mQueued = false;
	w->syncPosition ();

	if (mUngroupState == UngroupSingle &&
	    (gw->mAnimateState & IS_UNGROUPING))
	{
	    /* Possibility of stack breakage here, stop here */
	    gw->removeWindowFromGroup ();
	    it = mWindows.end ();	    
	}

	gw->mAnimateState = 0;
	gw->mTx = gw->mTy = gw->mXVelocity = gw->mYVelocity = 0.0f;
    }

    if (mUngroupState == UngroupAll)
	fini ();
    else
	mUngroupState = UngroupNone;
}

/*
 * GroupSelection::drawTabAnimation
 *
 * Description:
 * This function is called from groupPreparePaintScreen, to move
 * all the animated windows, with the required animation step.
 * The function goes through all grouped animated windows, calculates
 * the required step using adjustTabVelocity, moves the window,
 * and then checks if the animation is finished for that window.
 *
 */
void
GroupSelection::drawTabAnimation (int	      msSinceLastPaint)
{
    int        steps;
    float      amount, chunk;
    bool       doTabbing;
    
    GROUP_SCREEN (screen);

    amount = msSinceLastPaint * 0.05f * gs->optionGetTabbingSpeed ();
    steps = amount / (0.5f * gs->optionGetTabbingTimestep ());
    if (!steps)
	steps = 1;
    chunk = amount / (float)steps;

    while (steps--)
    {
	doTabbing = false;

	foreach (CompWindow *cw, mWindows)
	{
	    if (!cw)
		continue;

	    GROUP_WINDOW (cw);

	    if (!(gw->mAnimateState & IS_ANIMATED))
		continue;

	    if (!gw->adjustTabVelocity ())
	    {
		gw->mAnimateState |= FINISHED_ANIMATION;
		gw->mAnimateState &= ~IS_ANIMATED;
	    }

	    gw->mTx += gw->mXVelocity * chunk;
	    gw->mTy += gw->mYVelocity * chunk;

	    doTabbing |= (gw->mAnimateState & IS_ANIMATED);
	}

	if (!doTabbing)
	{
	    /* tabbing animation finished */
	    finishTabbing ();
	    break;
	}
    }
}

/*
 * GroupScreen::updateTabBars
 *
 * Description:
 * This function is responsible for showing / unshowing the tab-bars,
 * when the title-bars / tab-bars are hovered.
 * The function is called whenever a new window is entered,
 * checks if the entered window is a window frame (and if the title
 * bar part of that frame was hovered) or if it was the input
 * prevention window of a tab bar, and sets tab-bar visibility
 * according to that.
 *
 */
void
GroupScreen::updateTabBars (Window enteredWin)
{
    CompWindow     *w = NULL;
    GroupSelection *hoveredGroup = NULL;

    /* do nothing if the screen is grabbed, as the frame might be drawn
       transformed */
    if (!screen->otherGrabExist ("group", "group-drag", NULL))
    {
	/* first check if the entered window is a frame */
	foreach (w, screen->windows ())
	{
	    if (w->frame () == enteredWin)
		break;
	}
    }

    if (w)
    {
	/* is the window the entered frame belongs to inside
	   a tabbed group? if no, it's not interesting for us */
	GROUP_WINDOW (w);

	if (gw->mGroup && gw->mGroup->mTabBar)
	{
	    int mouseX, mouseY;
	    /* it is grouped and tabbed, so now we have to
	       check if we hovered the title bar or the frame */
	    if (getCurrentMousePosition (mouseX, mouseY))
	    {
		CompRect rect;
		CompRegion     reg;

		rect = CompRect (WIN_X (w) - w->input ().left,
				 WIN_Y (w) - w->input ().top,
				 WIN_WIDTH (w) + w->input ().right,
				 WIN_Y (w) - (WIN_Y (w) - w->input ().top));

		reg = reg.united (rect);

		if (reg.contains (CompPoint (mouseX, mouseY)))
		{
		    hoveredGroup = gw->mGroup;
		}
	    }
	}
    }

    /* if we didn't hover a title bar, check if we hovered
       a tab bar (means: input prevention window) */
    if (!hoveredGroup)
    {
	GroupSelection *group;

	foreach (group, mGroups)
	{
	    if (group->mTabBar && 
		group->mTabBar->mInputPrevention == enteredWin)
	    {
		/* only accept it if the IPW is mapped */
		if (group->mTabBar->mIpwMapped)
		{
		    hoveredGroup = group;
		    break;
		}
	    }
	}
    }

    /* if we found a hovered tab bar different than the last one
       (or left a tab bar), hide the old one */
    if (mLastHoveredGroup && (hoveredGroup != mLastHoveredGroup))
	mLastHoveredGroup->tabSetVisibility (false, 0);

    /* if we entered a tab bar (or title bar), show the tab bar */
    if (hoveredGroup && HAS_TOP_WIN (hoveredGroup) &&
	!TOP_TAB (hoveredGroup)->grabbed ())
    {	    
	GroupTabBar *bar = hoveredGroup->mTabBar;

	if (bar && ((bar->mState == PaintOff) || (bar->mState == PaintFadeOut)))
	{
	    int showDelayTime = optionGetTabbarShowDelay () * 1000;

	    /* Show the tab-bar after a delay,
	       only if the tab-bar wasn't fading out. */
	    if (showDelayTime > 0 && (bar->mState == PaintOff))
	    {
		if (mShowDelayTimeoutHandle.active ())
		    mShowDelayTimeoutHandle.stop ();

		mShowDelayTimeoutHandle.setTimes (showDelayTime, showDelayTime * 1.2);

		mShowDelayTimeoutHandle.setCallback (
			boost::bind (&GroupSelection::showDelayTimeout,
				     hoveredGroup));
		mShowDelayTimeoutHandle.start ();
	    }
	    else
		hoveredGroup->showDelayTimeout ();
	}
    }

    mLastHoveredGroup = hoveredGroup;
}



/*
 * getConstrainRegion
 *
 */
CompRegion
GroupScreen::getConstrainRegion ()
{
    CompRegion     region;
    CompRect       r;

    for (unsigned int i = 0;i < screen->outputDevs ().size (); i++)
	region = CompRegion (screen->outputDevs ()[i]).united (region);

    foreach (CompWindow *w, screen->windows ())
    {
	if (!w->mapNum ())
	    continue;

	if (w->struts ())
	{
	    r = CompRect (w->struts ()->top.x,
			  w->struts ()->top.y,
			  w->struts ()->top.width,
			  w->struts ()->top.height);

	    region = region.subtracted (r);

	    r = CompRect (w->struts ()->bottom.x,
			  w->struts ()->bottom.y,
			  w->struts ()->bottom.width,
			  w->struts ()->bottom.height);

	    region = region.subtracted (r);

	    r = CompRect (w->struts ()->left.x,
			  w->struts ()->left.y,
			  w->struts ()->left.width,
			  w->struts ()->left.height);

	    region = region.subtracted (r);

	    r = CompRect (w->struts ()->right.x,
			  w->struts ()->right.y,
			  w->struts ()->right.width,
			  w->struts ()->right.height);

	    region = region.subtracted (r);
	}
    }

    return region;
}

/*
 * constrainMovement
 *
 */
bool
GroupWindow::constrainMovement (CompRegion constrainRegion,
				     int        dx,
				     int        dy,
				     int        &new_dx,
				     int        &new_dy)
{
    int status, xStatus;
    int origDx = dx, origDy = dy;
    int x, y, width, height;
    CompWindow *w = window;

    if (!mGroup)
	return false;

    if (!dx && !dy)
	return false;

    x = mOrgPos.x () - w->input ().left + dx;
    y = mOrgPos.y ()- w->input ().top + dy;
    width = WIN_REAL_WIDTH (w);
    height = WIN_REAL_HEIGHT (w);

    status = constrainRegion.contains (CompRect (x, y, width, height));

    xStatus = status;
    while (dx && (xStatus != RectangleIn))
    {
	xStatus = constrainRegion.contains (CompRect (x, y - dy, width, height));

	if (xStatus != RectangleIn)
	    dx += (dx < 0) ? 1 : -1;

	x = mOrgPos.x () - w->input ().left + dx;
    }

    while (dy && (status != RectangleIn))
    {
	status = constrainRegion.contains (CompRect (x, y, width, height));

	if (status != RectangleIn)
	    dy += (dy < 0) ? 1 : -1;

	y = mOrgPos.y () - w->input ().top + dy;
    }

    new_dx = dx;

    new_dy = dy;

    return ((dx != origDx) || (dy != origDy));
}

/*
 * groupApplyConstraining
 *
 */
void
GroupSelection::applyConstraining (CompRegion	    constrainRegion,
				   Window	    constrainedWindow,
				   int	    	    dx,
				   int	    	    dy)
{
    if (!dx && !dy)
	return;

    foreach (CompWindow *w, mWindows)
    {
	GROUP_WINDOW (w);

	/* ignore certain windows: we don't want to apply the constraining
	   results on the constrained window itself, nor do we want to
	   change the target position of unamimated windows and of
	   windows which already are constrained */
	if (w->id () == constrainedWindow)
	    continue;

	if (!(gw->mAnimateState & IS_ANIMATED))
	    continue;

	if (gw->mAnimateState & DONT_CONSTRAIN)
	    continue;

	if (!(gw->mAnimateState & CONSTRAINED_X))
	{
	    int dummy;
	    gw->mAnimateState |= IS_ANIMATED;

	    /* applying the constraining result of another window
	       might move the window offscreen, too, so check
	       if this is not the case */
	    if (gw->constrainMovement (constrainRegion, dx, 0, dx, dummy))
		gw->mAnimateState |= CONSTRAINED_X;

	    gw->mDestination.setX (gw->mDestination.x () + dx);
	}

	if (!(gw->mAnimateState & CONSTRAINED_Y))
	{
	    int dummy;
	    gw->mAnimateState |= IS_ANIMATED;

	    /* analog to X case */
	    if (gw->constrainMovement (constrainRegion, 0, dy, dummy, dy))
		gw->mAnimateState |= CONSTRAINED_Y;

	    gw->mDestination.setY (gw->mDestination.y () + dy);
	}
    }
}

/*
 * groupStartTabbingAnimation
 *
 */
void
GroupSelection::startTabbingAnimation (bool           tab)
{
    int        dx, dy;
    int        constrainStatus;

    GROUP_SCREEN (screen);

    if ((mTabbingState != NoTabbing))
	return;

    mTabbingState = (tab) ? Tabbing : Untabbing;
    gs->tabChangeActivateEvent (true);

    if (!tab)
    {
	/* we need to set up the X/Y constraining on untabbing */
	CompRegion constrainRegion = gs->getConstrainRegion ();
	bool   constrainedWindows = true;

	/* reset all flags */
	foreach (CompWindow *cw, mWindows)
	{
	    GROUP_WINDOW (cw);
	    gw->mAnimateState &= ~(CONSTRAINED_X | CONSTRAINED_Y |
				  DONT_CONSTRAIN);
	}

	/* as we apply the constraining in a flat loop,
	   we may need to run multiple times through this
	   loop until all constraining dependencies are met */
	while (constrainedWindows)
	{
	    constrainedWindows = false;
	    /* loop through all windows and try to constrain their
	       animation path (going from gw->mOrgPos to
	       gw->mDestination) to the active screen area */
	    foreach (CompWindow *w, mWindows)
	    {
		GroupWindow *gw = GroupWindow::get (w);
		CompRect   statusRect (gw->mOrgPos.x () - w->input ().left,
				       gw->mOrgPos.y () - w->input ().top,
				       WIN_REAL_WIDTH (w),
				       WIN_REAL_HEIGHT (w));

		/* ignore windows which aren't animated and/or
		   already are at the edge of the screen area */
		if (!(gw->mAnimateState & IS_ANIMATED))
		    continue;

		if (gw->mAnimateState & DONT_CONSTRAIN)
		    continue;

		/* is the original position inside the screen area? */
		constrainStatus = constrainRegion.contains (statusRect);

		/* constrain the movement */
		if (gw->constrainMovement (constrainRegion,
					    gw->mDestination.x () - gw->mOrgPos.x (),
					    gw->mDestination.y () - gw->mOrgPos.y (),
					    dx, dy))
		{
		    /* handle the case where the window is outside the screen
		       area on its whole animation path */
		    if (constrainStatus != RectangleIn && !dx && !dy)
		    {
			gw->mAnimateState |= DONT_CONSTRAIN;
			gw->mAnimateState |= CONSTRAINED_X | CONSTRAINED_Y;

			/* use the original position as last resort */
			gw->mDestination = gw->mMainTabOffset;
		    }
		    else
		    {
			/* if we found a valid target position, apply
			   the change also to other windows to retain
			   the distance between the windows */
			gw->mGroup->applyConstraining (constrainRegion, w->id (),
						dx - gw->mDestination.x () +
						gw->mOrgPos.x (),
						dy - gw->mDestination.y () +
						gw->mOrgPos.y ());

			/* if we hit constraints, adjust the mask and the
			   target position accordingly */
			if (dx != (gw->mDestination.x () - gw->mOrgPos.x ()))
			{
			    gw->mAnimateState |= CONSTRAINED_X;
			    gw->mDestination.setX (gw->mOrgPos.x () + dx);
			}

			if (dy != (gw->mDestination.y () - gw->mOrgPos.y ()))
			{
			    gw->mAnimateState |= CONSTRAINED_Y;
			    gw->mDestination.setY (gw->mOrgPos.y () + dy);
			}

			constrainedWindows = true;
		    }
		}
	    }
	}
    }
}

/*
 * GroupSelection::tabGroup
 *
 */
void
GroupSelection::tabGroup (CompWindow *main)
{
    GroupTabBarSlot *slot;
    int             width, height;
    int             space, thumbSize;

    GROUP_WINDOW (main);
    GROUP_SCREEN (screen);

    if (mTabBar)
	return;

    if (!screen->XShape ())
    {
	compLogMessage ("group", CompLogLevelError,
			"No X shape extension! Tabbing disabled.");
	return;
    }

    mTabBar = new GroupTabBar (this, main);
    if (!mTabBar)
	return;

    mTabbingState = NoTabbing;
    /* Slot is initialized after GroupTabBar is created */
    gs->changeTab (gw->mSlot, GroupTabBar::RotateUncertain);
    gw->mGroup->mTabBar->recalcTabBarPos (WIN_CENTER_X (main),
			  WIN_X (main), WIN_X (main) + WIN_WIDTH (main));

    width = mTabBar->mRegion.boundingRect ().x2 () -
	    mTabBar->mRegion.boundingRect ().x1 ();
    height = mTabBar->mRegion.boundingRect ().y2 () -
	     mTabBar->mRegion.boundingRect ().y1 ();

    mTabBar->mTextLayer = new TextLayer (CompSize (width, height),
					 this);
    if (mTabBar->mTextLayer)
    {
	TextLayer *layer;

	layer = mTabBar->mTextLayer;
	layer->mState = PaintOff;
	layer->mAnimationTime = 0;
	mTabBar->mTextLayer = TextLayer::rebuild (mTabBar->mTextLayer);
	
	if (mTabBar->mTextLayer)
	    mTabBar->mTextLayer->render ();
    }
    if (mTabBar->mTextLayer)
    {
	TextLayer *layer;

	layer = mTabBar->mTextLayer;
	layer->mAnimationTime = gs->optionGetFadeTextTime () * 1000;
	layer->mState = PaintFadeIn;
    }

    /* we need a buffer for DnD here */
    space = gs->optionGetThumbSpace ();
    thumbSize = gs->optionGetThumbSize ();
    mTabBar->mBgLayer = BackgroundLayer::create (CompSize (width + space + thumbSize,
						      height), this);
    if (mTabBar->mBgLayer)
    {
	mTabBar->mBgLayer->mState = PaintOn;
	mTabBar->mBgLayer->mAnimationTime = 0;
	mTabBar->mBgLayer->render ();
    }

    width = mTabBar->mTopTab->mRegion.boundingRect ().x2 () -
	    mTabBar->mTopTab->mRegion.boundingRect ().x1 ();
    height = mTabBar->mTopTab->mRegion.boundingRect ().y2 () -
	     mTabBar->mTopTab->mRegion.boundingRect ().y1 ();

    mTabBar->mSelectionLayer = SelectionLayer::create (CompSize (width, height), this);
    if (mTabBar->mSelectionLayer)
    {
	CompSize size = 
	    CompSize (mTabBar->mTopTab->mRegion.boundingRect ().width (),
		      mTabBar->mTopTab->mRegion.boundingRect ().height ());
	mTabBar->mSelectionLayer->mState = PaintOn;
	mTabBar->mSelectionLayer->mAnimationTime = 0;
	mTabBar->mSelectionLayer = SelectionLayer::rebuild (mTabBar->mSelectionLayer,
							    size);
	if (mTabBar->mSelectionLayer)
	    mTabBar->mSelectionLayer->render ();
    }

    if (!HAS_TOP_WIN (this))
	return;

    foreach (slot, mTabBar->mSlots)
    {
	CompWindow *cw = slot->mWindow;

	GROUP_WINDOW (cw);

	if (gw->mAnimateState & (IS_ANIMATED | FINISHED_ANIMATION))
	    cw->move (gw->mDestination.x () - WIN_X (cw),
		      gw->mDestination.y () - WIN_Y (cw), true);

	/* center the window to the main window */
	gw->mDestination = CompPoint (WIN_CENTER_X (main) - (WIN_WIDTH (cw) / 2),
				      WIN_CENTER_Y (main) - (WIN_HEIGHT (cw) / 2));

	/* Distance from destination. */
	gw->mMainTabOffset = CompPoint (WIN_X (cw), WIN_Y (cw)) -
					gw->mDestination;

	if (gw->mTx || gw->mTy)
	{
	    gw->mTx -= (WIN_X (cw) - gw->mOrgPos.x ());
	    gw->mTy -= (WIN_Y (cw) - gw->mOrgPos.y ());
	}

	gw->mOrgPos = CompPoint (WIN_X (cw), WIN_Y (cw));

	gw->mAnimateState = IS_ANIMATED;
	gw->mXVelocity = gw->mYVelocity = 0.0f;
    }

    startTabbingAnimation (true);
}

/*
 * GroupSelection::untabGroup
 *
 */
void
GroupSelection::untabGroup ()
{
    int             oldX, oldY;
    CompWindow      *prevTopTab;
    GroupTabBarSlot *slot;
    
    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (this))
	return;

    if (mTabBar->mPrevTopTab)
	prevTopTab = PREV_TOP_TAB (this);
    else
    {
	/* If prevTopTab isn't set, we have no choice but using topTab.
	   It happens when there is still animation, which
	   means the tab wasn't changed anyway. */
	prevTopTab = TOP_TAB (this);
    }

    mTabBar->mLastTopTab = TOP_TAB (this);
    mTabBar->mTopTab = NULL;

    foreach (slot, mTabBar->mSlots)
    {
	CompWindow *cw = slot->mWindow;

	GROUP_WINDOW (cw);

	if (gw->mAnimateState & (IS_ANIMATED | FINISHED_ANIMATION))
	{
	    gs->mQueued = true;
	    cw->move(gw->mDestination.x () - WIN_X (cw),
		     gw->mDestination.y () - WIN_Y (cw), true);
	    gs->mQueued = false;
	}

	gw->setWindowVisibility (true);

	/* save the old original position - we might need it
	   if constraining fails */
	oldX = gw->mOrgPos.x ();
	oldY = gw->mOrgPos.y ();

	gw->mOrgPos = CompPoint (WIN_CENTER_X (prevTopTab) - WIN_WIDTH (cw) / 2,
				 WIN_CENTER_Y (prevTopTab) - WIN_HEIGHT (cw) / 2);

	gw->mDestination = gw->mOrgPos + gw->mMainTabOffset;

	if (gw->mTx || gw->mTy)
	{
	    gw->mTx -= (gw->mOrgPos.x () - oldX);
	    gw->mTy -= (gw->mOrgPos.y () - oldY);
	}

	gw->mMainTabOffset = CompPoint (oldX, oldY);

	gw->mAnimateState = IS_ANIMATED;
	gw->mXVelocity = gw->mYVelocity = 0.0f;
    }

    mTabbingState = NoTabbing;
    startTabbingAnimation (false);

    gs->cScreen->damageScreen ();
}

/*
 * changeTab
 *
 */
bool
GroupScreen::changeTab (GroupTabBarSlot             *topTab,
			     GroupTabBar::ChangeAnimationDirection direction)
{
    CompWindow     *w, *oldTopTab;
    GroupSelection *group;

    if (!topTab)
	return true;

    w = topTab->mWindow;

    GROUP_WINDOW (w);

    group = gw->mGroup;

    if (!group || !group->mTabBar || group->mTabbingState != GroupSelection::NoTabbing)
	return true;

    if (group->mTabBar->mChangeState == GroupTabBar::NoTabChange &&
	group->mTabBar->mTopTab == topTab)
	return true;

    if (group->mTabBar->mChangeState != GroupTabBar::NoTabChange &&
	group->mTabBar->mNextTopTab == topTab)
	return true;

    oldTopTab = group->mTabBar->mTopTab ? group->mTabBar->mTopTab->mWindow : NULL;

    if (group->mTabBar->mChangeState != GroupTabBar::NoTabChange)
	group->mTabBar->mNextDirection = direction;
    else if (direction == GroupTabBar::RotateLeft)
	group->mTabBar->mChangeAnimationDirection = 1;
    else if (direction == GroupTabBar::RotateRight)
	group->mTabBar->mChangeAnimationDirection = -1;
    else
    {
	int             distanceOld = 0, distanceNew = 0;
	GroupTabBarSlot::List::iterator it = group->mTabBar->mSlots.begin ();

	if (group->mTabBar->mTopTab)
	    for (; (*it) && ((*it) != group->mTabBar->mTopTab);
		 it++, distanceOld++);

	for (it = group->mTabBar->mSlots.begin (); (*it) && ((*it) != topTab);
	     it++, distanceNew++);

	if (distanceNew < distanceOld)
	    group->mTabBar->mChangeAnimationDirection = 1;   /*left */
	else
	    group->mTabBar->mChangeAnimationDirection = -1;  /* right */

	/* check if the opposite direction is shorter */
	if (abs (distanceNew - distanceOld) > ((int) group->mTabBar->mSlots.size () / 2))
	    group->mTabBar->mChangeAnimationDirection *= -1;
    }

    if (group->mTabBar->mChangeState != GroupTabBar::NoTabChange)
    {
	if (group->mTabBar->mPrevTopTab == topTab)
	{
	    /* Reverse animation. */
	    GroupTabBarSlot *tmp = group->mTabBar->mTopTab;
	    group->mTabBar->mTopTab = group->mTabBar->mPrevTopTab;
	    group->mTabBar->mPrevTopTab = tmp;

	    group->mTabBar->mChangeAnimationDirection *= -1;
	    group->mTabBar->mChangeAnimationTime =
		optionGetChangeAnimationTime () * 500 -
		group->mTabBar->mChangeAnimationTime;
	    group->mTabBar->mChangeState = (group->mTabBar->mChangeState == GroupTabBar::TabChangeOldOut) ?
		GroupTabBar::TabChangeNewIn : GroupTabBar::TabChangeOldOut;

	    group->mTabBar->mNextTopTab = NULL;
	}
	else
	    group->mTabBar->mNextTopTab = topTab;
    }
    else
    {
	CompSize size (group->mTabBar->mTopTab->mRegion.boundingRect ().width (),
		       group->mTabBar->mTopTab->mRegion.boundingRect ().height ());
	group->mTabBar->mTopTab = topTab;

	group->mTabBar->mTextLayer = TextLayer::rebuild (group->mTabBar->mTextLayer);
	
	if (group->mTabBar->mTextLayer)
	    group->mTabBar->mTextLayer->render ();
	group->mTabBar->mSelectionLayer =
	       SelectionLayer::rebuild (group->mTabBar->mSelectionLayer,
					size);
	if (group->mTabBar->mSelectionLayer)
	    group->mTabBar->mSelectionLayer->render ();
	if (oldTopTab)
	    CompositeWindow::get (oldTopTab)->addDamage ();
	CompositeWindow::get (w)->addDamage ();
    }

    if (topTab != group->mTabBar->mNextTopTab)
    {
	gw->setWindowVisibility (true);
	if (oldTopTab)
	{
	    int dx, dy;

	    dx = WIN_CENTER_X (oldTopTab) - WIN_CENTER_X (w);
	    dy = WIN_CENTER_Y (oldTopTab) - WIN_CENTER_Y (w);

	    mQueued = true;
	    w->move (dx, dy, false);
	    w->syncPosition ();
	    mQueued = false;
	}

	if (HAS_PREV_TOP_WIN (group))
	{
	    /* we use only the half time here -
	       the second half will be PaintFadeOut */
	    group->mTabBar->mChangeAnimationTime =
		optionGetChangeAnimationTime () * 500;
	    tabChangeActivateEvent (true);
	    group->mTabBar->mChangeState = GroupTabBar::TabChangeOldOut;
	}
	else
	{
	    bool activate;

	    /* No window to do animation with. */
	    if (HAS_TOP_WIN (group))
		group->mTabBar->mPrevTopTab = group->mTabBar->mTopTab;
	    else
		group->mTabBar->mPrevTopTab = NULL;

	    activate = !group->mTabBar->mCheckFocusAfterTabChange;
	    if (!activate)
	    {
		/*
		CompFocusResult focus;

		focus    = allowWindowFocus (w, NO_FOCUS_MASK, s->x, s->y, 0);
		activate = focus == CompFocusAllowed;
		*/
	    }

	    if (activate)
		w->activate ();

	    group->mTabBar->mCheckFocusAfterTabChange = false;
	}
    }

    return true;
}

/*
 * recalcSlotPos
 *
 */
void
GroupScreen::recalcSlotPos (GroupTabBarSlot *slot,
				 int		 slotPos)
{
    GroupSelection *group;
    CompRect       box;
    int            space, thumbSize;

    GROUP_WINDOW (slot->mWindow);
    group = gw->mGroup;

    if (!HAS_TOP_WIN (group) || !group->mTabBar)
	return;

    space = optionGetThumbSpace ();
    thumbSize = optionGetThumbSize ();

    slot->mRegion = emptyRegion;

    box.setX (space + ((thumbSize + space) * slotPos));
    box.setY (space);

    box.setWidth (thumbSize);
    box.setHeight (thumbSize);

    slot->mRegion = CompRegion (box);
}

/*
 * GroupSelection::recalcTabBarPos
 *
 */
void
GroupTabBar::recalcTabBarPos (int		middleX,
			      int		minX1,
			      int		maxX2)
{
    GroupTabBarSlot *slot;
    CompWindow      *topTab;
    bool            isDraggedSlotGroup = false;
    int             space, barWidth;
    int             thumbSize;
    int             tabsWidth = 0, tabsHeight = 0;
    int             currentSlot;
    CompRect	    box;

    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (mGroup))
	return;

    topTab = TOP_TAB (mGroup);
    space = gs->optionGetThumbSpace ();

    /* calculate the space which the tabs need */
    foreach (slot, mSlots)
    {
	if (slot == gs->mDraggedSlot && gs->mDragged)
	{
	    isDraggedSlotGroup = true;
	    continue;
	}

	tabsWidth += (slot->mRegion.boundingRect ().x2 () - slot->mRegion.boundingRect ().x1 ());
	if ((slot->mRegion.boundingRect ().y2 () - slot->mRegion.boundingRect ().y1 ()) > tabsHeight)
	    tabsHeight = slot->mRegion.boundingRect ().y2 () - slot->mRegion.boundingRect ().y1 ();
    }

    /* just a little work-a-round for first call
       FIXME: remove this! */
    thumbSize = gs->optionGetThumbSize ();
    if (mSlots.size () && tabsWidth <= 0)
    {
	/* first call */
	tabsWidth = thumbSize * mSlots.size ();

	if (mSlots.size () && tabsHeight < thumbSize)
	{
	    /* we need to do the standard height too */
	    tabsHeight = thumbSize;
	}

	if (isDraggedSlotGroup)
	    tabsWidth -= thumbSize;
    }

    barWidth = space * (mSlots.size () + 1) + tabsWidth;

    if (isDraggedSlotGroup)
    {
	/* 1 tab is missing, so we have 1 less border */
	barWidth -= space;
    }

    if (maxX2 - minX1 < barWidth)
	box.setX ((maxX2 + minX1) / 2 - barWidth / 2);
    else if (middleX - barWidth / 2 < minX1)
	box.setX (minX1);
    else if (middleX + barWidth / 2 > maxX2)
	box.setX (maxX2 - barWidth);
    else
	box.setX (middleX - barWidth / 2);

    box.setY (WIN_Y (topTab));
    box.setWidth (barWidth);
    box.setHeight (space * 2 + tabsHeight);

    resizeTabBarRegion (box, true);

    /* recalc every slot region */
    currentSlot = 0;
    foreach (slot, mSlots)
    {
	if (slot == gs->mDraggedSlot && gs->mDragged)
	    continue;

	gs->recalcSlotPos (slot, currentSlot);
	slot->mRegion.translate (mRegion.boundingRect ().x1 (),
				 mRegion.boundingRect ().y1 ());

	slot->mSpringX = (slot->mRegion.boundingRect ().x1 () +
			 slot->mRegion.boundingRect ().x2 ()) / 2;
	slot->mSpeed = 0;
	slot->mMsSinceLastMove = 0;

	currentSlot++;
    }

    mLeftSpringX = box.x ();
    mRightSpringX = box.x () + box.width ();

    mRightSpeed = 0;
    mLeftSpeed = 0;

    mRightMsSinceLastMove = 0;
    mLeftMsSinceLastMove = 0;
}

void
GroupTabBar::damageRegion ()
{
    CompRegion reg (mRegion);
    int x1 = reg.boundingRect ().x1 ();
    int x2 = reg.boundingRect ().x2 ();
    int y1 = reg.boundingRect ().y1 ();
    int y2 = reg.boundingRect ().y2 ();

    /* we use 15 pixels as damage buffer here, as there is a 10 pixel wide
       border around the selected slot which also needs to be damaged
       properly - however the best way would be if slot->mRegion was
       sized including the border */

#define DAMAGE_BUFFER 20

    if (mSlots.size ())
    {
	const CompRect &bnd = mSlots.front ()->mRegion.boundingRect ();
	x1 = MIN (x1, bnd.x1 ());
	y1 = MIN (y1, bnd.y1 ());
	x2 = MAX (x2, bnd.x2 ());
	y2 = MAX (y2, bnd.y2 ());
    }

    x1 -= DAMAGE_BUFFER;
    y1 -= DAMAGE_BUFFER;
    x2 += DAMAGE_BUFFER;
    y2 += DAMAGE_BUFFER;

    reg = CompRegion (x1, y1,
		      x2 - x1,
		      y2 - y1);

    GroupScreen::get (screen)->cScreen->damageRegion (reg);
}

void
GroupTabBar::moveTabBarRegion (int		   dx,
			       int		   dy,
			       bool	   	   syncIPW)
{
    damageRegion ();

    mRegion.translate (dx, dy);

    if (syncIPW)
	XMoveWindow (screen->dpy (),
		     mInputPrevention,
		     mLeftSpringX,
		     mRegion.boundingRect ().y1 ());

    damageRegion ();
}

void
GroupTabBar::resizeTabBarRegion (CompRect	&box,
				 bool           syncIPW)
{
    int oldWidth;

    GROUP_SCREEN (screen);

    damageRegion ();

    oldWidth = mRegion.boundingRect ().x2 () -
	mRegion.boundingRect ().x1 ();

    if (mBgLayer && oldWidth != box.width () && syncIPW)
    {
	mBgLayer =
	    BackgroundLayer::rebuild (mBgLayer,
				 CompSize (box.width () +
				    gs->optionGetThumbSpace () +
				    gs->optionGetThumbSize (),
				    box.height ()));
	if (mBgLayer)
	    mBgLayer->render ();

	/* invalidate old width */
	mOldWidth = 0;
    }    

    mRegion = CompRegion (box);

    if (syncIPW)
    {
	XWindowChanges xwc;

	xwc.x = box.x ();
	xwc.y = box.y ();
	xwc.width = box.width ();
	xwc.height = box.height ();

	if (!mIpwMapped)
	    XMapWindow (screen->dpy (), mInputPrevention);

	XMoveResizeWindow (screen->dpy (), mInputPrevention, xwc.x, xwc.y, xwc.width, xwc.height);
	
	if (!mIpwMapped)
	    XUnmapWindow (screen->dpy (), mInputPrevention);
    }

    damageRegion ();
}

/*
 * GroupTabBar::insertTabBarSlotBefore
 *
 */
void
GroupTabBar::insertTabBarSlotBefore (GroupTabBarSlot *slot,
				     GroupTabBarSlot *nextSlot)
{
    GroupTabBarSlot *prev = nextSlot->mPrev;
    GroupTabBarSlot::List::iterator pos = std::find (mSlots.begin (),
						     mSlots.end (),
						     nextSlot);
    
    mSlots.insert (pos, slot);
    slot->mTabBar = this;

    if (prev)
    {
	slot->mPrev = prev;
	prev->mNext = slot;
    }
    else
    {
	slot->mPrev = NULL;
    }

    slot->mNext = nextSlot;
    nextSlot->mPrev = slot;

    /* Moving bar->mRegion.boundingRect ().x1 () / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in
       the average between them, which is
       (bar->mRegion.boundingRect ().x1 () + bar->mRegion.boundingRect ().x2 ()) / 2 anyway. */
    recalcTabBarPos ((mRegion.boundingRect ().x1 () +
		      mRegion.boundingRect ().x2 ()) / 2,
		      mRegion.boundingRect ().x1 (), mRegion.boundingRect ().x2 ());
}

/*
 * GroupSelection::insertTabBarSlotAfter
 *
 */
void
GroupTabBar::insertTabBarSlotAfter (GroupTabBarSlot *slot,
				    GroupTabBarSlot *prevSlot)
{
    GroupTabBarSlot *next = prevSlot->mNext;
    GroupTabBarSlot::List::iterator pos = std::find (mSlots.begin (),
						     mSlots.end (),
						     next);

    mSlots.insert (pos, slot);
    slot->mTabBar = this;

    if (next)
    {
	slot->mNext = next;
	next->mPrev = slot;
    }
    else
    {
	slot->mNext = NULL;
    }

    slot->mPrev = prevSlot;
    prevSlot->mNext = slot;

    /* Moving bar->mRegion.boundingRect ().x1 () / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in the
       average between them, which is
       (bar->mRegion.boundingRect ().x1 () + bar->mRegion.boundingRect ().x2 ()) / 2 anyway. */
    recalcTabBarPos ((mRegion.boundingRect ().x1 () +
		      mRegion.boundingRect ().x2 ()) / 2,
		      mRegion.boundingRect ().x1 (), mRegion.boundingRect ().x2 ());
}

/*
 * GroupSelection::insertTabBarSlot
 *
 */
void
GroupTabBar::insertTabBarSlot (GroupTabBarSlot *slot)
{
    if (mSlots.size ())
    {
	mSlots.back ()->mNext = slot;
	slot->mPrev = mSlots.back ();
	slot->mNext = NULL;
    }
    else
    {
	slot->mPrev = NULL;
	slot->mNext = NULL;
    }

    mSlots.push_back (slot);
    slot->mTabBar = this;

    /* Moving bar->mRegion.boundingRect ().x1 () / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in
       the average between them, which is
       (bar->mRegion.boundingRect ().x1 () + bar->mRegion.boundingRect ().x2 ()) / 2 anyway. */
    recalcTabBarPos ((mRegion.boundingRect ().x1 () +
		      mRegion.boundingRect ().x2 ()) / 2,
		      mRegion.boundingRect ().x1 (), mRegion.boundingRect ().x2 ());
}

/*
 * GroupTabBar::unhookTabBarSlot
 *
 */
void
GroupTabBar::unhookTabBarSlot (GroupTabBarSlot *slot,
			       bool            temporary)
{
    GroupTabBarSlot *tempSlot;
    GroupTabBarSlot *prev = slot->mPrev;
    GroupTabBarSlot *next = slot->mNext;
    CompWindow      *w = slot->mWindow;
    GroupSelection  *group = mGroup;
    
    GROUP_SCREEN (screen);

    /* check if slot is not already unhooked */
    foreach (tempSlot, mSlots)
	if (tempSlot == slot)
	    break;

    if (!tempSlot)
	return;

    if (prev)
	prev->mNext = next;

    if (next)
	next->mPrev = prev;

    slot->mPrev = NULL;
    slot->mNext = NULL;
    slot->mTabBar = NULL;
    
    mSlots.remove (slot);

    if (!temporary)
    {
	if (IS_PREV_TOP_TAB (w, group))
	    group->mTabBar->mPrevTopTab = NULL;
	if (IS_TOP_TAB (w, group))
	{
	    group->mTabBar->mTopTab = NULL;

	    if (next)
		gs->changeTab (next, RotateRight);
	    else if (prev)
		gs->changeTab (prev, RotateLeft);

	    if (gs->optionGetUntabOnClose ())
		group->untabGroup ();
	}
    }

    if (slot == mHoveredSlot)
	mHoveredSlot = NULL;

    if (slot == mTextSlot)
    {
	mTextSlot = NULL;

	if (mTextLayer)
	{
	    if (mTextLayer->mState == PaintFadeIn ||
		mTextLayer->mState == PaintOn)
	    {
		mTextLayer->mAnimationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    mTextLayer->mAnimationTime;
		mTextLayer->mState = PaintFadeOut;
	    }
	}
    }

    /* Moving bar->mRegion.boundingRect ().x1 () / x2 as minX1 / maxX2 will work,
       because the tab-bar got thiner now, so
       (bar->mRegion.boundingRect ().x1 () + bar->mRegion.boundingRect ().x2 ()) / 2
       Won't cause the new x1 / x2 to be outside the original region. */
    recalcTabBarPos ((mRegion.boundingRect ().x1 () +
		      mRegion.boundingRect ().x2 ()) / 2,
		      mRegion.boundingRect ().x1 (),
		      mRegion.boundingRect ().x2 ());
}

/*
 * GroupSelection::deleteTabBarSlot
 *
 */
void
GroupTabBar::deleteTabBarSlot (GroupTabBarSlot *slot)
{
    CompWindow *w = slot->mWindow;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    unhookTabBarSlot (slot, false);

    slot->mRegion = CompRegion ();

    if (slot == gs->mDraggedSlot)
    {
	gs->mDraggedSlot = NULL;
	gs->mDragged = false;

	if (gs->mGrabState == GroupScreen::ScreenGrabTabDrag)
	    gs->grabScreen (GroupScreen::ScreenGrabNone);
    }

    gw->mSlot = NULL;
    gw->updateWindowProperty ();
    delete slot;
}

GroupTabBarSlot::GroupTabBarSlot (CompWindow *w, GroupTabBar *bar) :
    GLLayer (CompSize (0,0), bar->mGroup), // FIXME: make this the size?
    mWindow (w),
    mTabBar (bar)
{
}

/*
 * groupCreateSlot
 *
 */
void
GroupTabBar::createSlot (CompWindow      *w)
{
    GroupTabBarSlot *slot;

    GROUP_WINDOW (w);

    slot = new GroupTabBarSlot (w, this);
    if (!slot)
        return;

    insertTabBarSlot (slot);
    gw->mSlot = slot;
    gw->updateWindowProperty ();
}

#define SPRING_K     GroupScreen::get (screen)->optionGetDragSpringK()
#define FRICTION     GroupScreen::get (screen)->optionGetDragFriction()
#define SIZE	     GroupScreen::get (screen)->optionGetThumbSize()
#define BORDER	     GroupScreen::get (screen)->optionGetBorderRadius()
#define Y_START_MOVE GroupScreen::get (screen)->optionGetDragYDistance()
#define SPEED_LIMIT  GroupScreen::get (screen)->optionGetDragSpeedLimit()

/*
 * groupSpringForce
 *
 */
static inline int
groupSpringForce (CompScreen *s,
		  int        centerX,
		  int        springX)
{
    /* Each slot has a spring attached to it, starting at springX,
       and ending at the center of the slot (centerX).
       The spring will cause the slot to move, using the
       well-known physical formula F = k * dl... */
    return -SPRING_K * (centerX - springX);
}

/*
 * groupDraggedSlotForce
 *
 */
static int
groupDraggedSlotForce (CompScreen *s,
		       int        distanceX,
		       int        distanceY)
{
    /* The dragged slot will make the slot move, to get
       DnD animations (slots will make room for the newly inserted slot).
       As the dragged slot is closer to the slot, it will put
       more force on the slot, causing it to make room for the dragged slot...
       But if the dragged slot gets too close to the slot, they are
       going to be reordered soon, so the force will get lower.

       If the dragged slot is in the other side of the slot,
       it will have to make force in the opposite direction.

       So we the needed funtion is an odd function that goes
       up at first, and down after that.
       Sinus is a function like that... :)

       The maximum is got when x = (x1 + x2) / 2,
       in this case: x = SIZE + BORDER.
       Because of that, for x = SIZE + BORDER,
       we get a force of SPRING_K * (SIZE + BORDER) / 2.
       That equals to the force we get from the the spring.
       This way, the slot won't move when its distance from
       the dragged slot is SIZE + BORDER (which is the default
       distance between slots).
       */

    /* The maximum value */
    float a = SPRING_K * (SIZE + BORDER) / 2;
    /* This will make distanceX == 2 * (SIZE + BORDER) to get 0,
       and distanceX == (SIZE + BORDER) to get the maximum. */
    float b = PI /  (2 * SIZE + 2 * BORDER);

    /* If there is some distance between the slots in the y axis,
       the slot should get less force... For this, we change max
       to a lower value, using a simple linear function. */

    if (distanceY < Y_START_MOVE)
	a *= 1.0f - (float)distanceY / Y_START_MOVE;
    else
	a = 0;

    if (abs (distanceX) < 2 * (SIZE + BORDER))
	return a * sin (b * distanceX);
    else
	return 0;
}

/*
 * groupApplyFriction
 *
 */
static inline void
groupApplyFriction (CompScreen *s,
		    int        *speed)
{
    if (abs (*speed) < FRICTION)
	*speed = 0;
    else if (*speed > 0)
	*speed -= FRICTION;
    else if (*speed < 0)
	*speed += FRICTION;
}

/*
 * groupApplySpeedLimit
 *
 */
static inline void
groupApplySpeedLimit (CompScreen *s,
		      int        *speed)
{
    if (*speed > SPEED_LIMIT)
	*speed = SPEED_LIMIT;
    else if (*speed < -SPEED_LIMIT)
	*speed = -SPEED_LIMIT;
}

/*
 * groupApplyForces
 *
 */
void
GroupTabBar::applyForces (GroupTabBarSlot *draggedSlot)
{
    GroupTabBarSlot *slot, *slot2;
    int             centerX, centerY;
    int             draggedCenterX, draggedCenterY;

    if (draggedSlot)
    {
	int vx, vy;

	draggedSlot->getDrawOffset (vx, vy);

	draggedCenterX = ((draggedSlot->mRegion.boundingRect ().x1 () +
			   draggedSlot->mRegion.boundingRect ().x2 ()) / 2) + vx;
	draggedCenterY = ((draggedSlot->mRegion.boundingRect ().y1 () +
			   draggedSlot->mRegion.boundingRect ().y2 ()) / 2) + vy;
    }
    else
    {
	draggedCenterX = 0;
	draggedCenterY = 0;
    }

    mLeftSpeed += groupSpringForce(screen,
				   mRegion.boundingRect ().x1 (),
				   mLeftSpringX);
    mRightSpeed += groupSpringForce(screen,
				    mRegion.boundingRect ().x2 (),
				    mRightSpringX);

    if (draggedSlot)
    {
	int leftForce, rightForce;

	leftForce = groupDraggedSlotForce(screen,
					  mRegion.boundingRect ().x1 () -
					  SIZE / 2 - draggedCenterX,
					  abs ((mRegion.boundingRect ().y1 () +
						mRegion.boundingRect ().y2 ()) / 2 -
					       draggedCenterY));

	rightForce = groupDraggedSlotForce (screen,
					    mRegion.boundingRect ().x2 () +
					    SIZE / 2 - draggedCenterX,
					    abs ((mRegion.boundingRect ().y1 () +
						  mRegion.boundingRect ().y2 ()) / 2 -
						 draggedCenterY));

	if (leftForce < 0)
	    mLeftSpeed += leftForce;
	if (rightForce > 0)
	    mRightSpeed += rightForce;
    }

    foreach (slot, mSlots)
    {
	centerX = (slot->mRegion.boundingRect ().x1 () + slot->mRegion.boundingRect ().x2 ()) / 2;
	centerY = (slot->mRegion.boundingRect ().y1 () + slot->mRegion.boundingRect ().y2 ()) / 2;

	slot->mSpeed += groupSpringForce (screen, centerX, slot->mSpringX);

	if (draggedSlot && draggedSlot != slot)
	{
	    int draggedSlotForce;
	    draggedSlotForce =
		groupDraggedSlotForce(screen, centerX - draggedCenterX,
				      abs (centerY - draggedCenterY));

	    slot->mSpeed += draggedSlotForce;
	    slot2 = NULL;

	    if (draggedSlotForce < 0)
	    {
		slot2 = slot->mPrev;
		mLeftSpeed += draggedSlotForce;
	    }
	    else if (draggedSlotForce > 0)
	    {
		slot2 = slot->mNext;
		mRightSpeed += draggedSlotForce;
	    }

	    while (slot2)
	    {
		if (slot2 != draggedSlot)
		    slot2->mSpeed += draggedSlotForce;

		slot2 = (draggedSlotForce < 0) ? slot2->mPrev : slot2->mNext;
	    }
	}
    }

    foreach (slot, mSlots)
    {
	groupApplyFriction (screen, &slot->mSpeed);
	groupApplySpeedLimit (screen, &slot->mSpeed);
    }

    groupApplyFriction (screen, &mLeftSpeed);
    groupApplySpeedLimit (screen, &mLeftSpeed);

    groupApplyFriction (screen, &mRightSpeed);
    groupApplySpeedLimit (screen, &mRightSpeed);
}

/*
 * groupApplySpeeds
 *
 */
void
GroupTabBar::applySpeeds (int            msSinceLastRepaint)
{
    GroupTabBarSlot *slot;
    int             move;
    CompRect	    box;
    bool            updateTabBar = false;

    box.setX (mRegion.boundingRect ().x1 ());
    box.setY (mRegion.boundingRect ().y1 ());
    box.setWidth (mRegion.boundingRect ().x2 () - mRegion.boundingRect ().x1 ());
    box.setHeight (mRegion.boundingRect ().y2 () - mRegion.boundingRect ().y1 ());

    mLeftMsSinceLastMove += msSinceLastRepaint;
    mRightMsSinceLastMove += msSinceLastRepaint;

    /* Left */
    move = mLeftSpeed * mLeftMsSinceLastMove / 1000;
    if (move)
    {
	box.setX (box.x () + move);
	box.setWidth (box.width () - move);

	mLeftMsSinceLastMove = 0;
	updateTabBar = true;
    }
    else if (mLeftSpeed == 0 &&
	     mRegion.boundingRect ().x1 () != mLeftSpringX &&
	     (SPRING_K * abs (mRegion.boundingRect ().x1 () - mLeftSpringX) <
	      FRICTION))
    {
	/* Friction is preventing from the left border to get
	   to its original position. */
	box.setX (box.x () + mLeftSpringX - mRegion.boundingRect ().x1 ());
	box.setWidth (box.width () - mLeftSpringX - mRegion.boundingRect ().x1 ());

	mLeftMsSinceLastMove = 0;
	updateTabBar = true;
    }
    else if (mLeftSpeed == 0)
	mLeftMsSinceLastMove = 0;

    /* Right */
    move = mRightSpeed * mRightMsSinceLastMove / 1000;
    if (move)
    {
	box.setWidth (box.width () + move);

	mRightMsSinceLastMove = 0;
	updateTabBar = true;
    }
    else if (mRightSpeed == 0 &&
	     mRegion.boundingRect ().x2 () != mRightSpringX &&
	     (SPRING_K * abs (mRegion.boundingRect ().x2 () - mRightSpringX) <
	      FRICTION))
    {
	/* Friction is preventing from the right border to get
	   to its original position. */
	box.setWidth (box.width () + mLeftSpringX - mRegion.boundingRect ().x1 ());

	mLeftMsSinceLastMove = 0;
	updateTabBar = true;
    }
    else if (mRightSpeed == 0)
	mRightMsSinceLastMove = 0;

    if (updateTabBar)
	resizeTabBarRegion (box, false);

    foreach (slot, mSlots)
    {
	int slotCenter;

	slot->mMsSinceLastMove += msSinceLastRepaint;
	move = slot->mSpeed * slot->mMsSinceLastMove / 1000;
	slotCenter = (slot->mRegion.boundingRect ().x1 () +
		      slot->mRegion.boundingRect ().x2 ()) / 2;

	if (move)
	{
	    slot->mRegion.translate (move, 0);
	    slot->mMsSinceLastMove = 0;
	}
	else if (slot->mSpeed == 0 &&
		 slotCenter != slot->mSpringX &&
		 SPRING_K * abs (slotCenter - slot->mSpringX) < FRICTION)
	{
	    /* Friction is preventing from the slot to get
	       to its original position. */

	    slot->mRegion.translate (slot->mSpringX - slotCenter, 0);
	    slot->mMsSinceLastMove = 0;
	}
	else if (slot->mSpeed == 0)
	    slot->mMsSinceLastMove = 0;
    }
}

/*
 * GroupSelection::initTabBar
 *
 */
GroupTabBar::GroupTabBar (GroupSelection *group, 
			  CompWindow     *topTab) :
    mSlots (CompSize (0,0), group),
    mGroup (group),
    mTopTab (NULL),
    mPrevTopTab (NULL),
    mLastTopTab (NULL),
    mNextTopTab (NULL),
    mCheckFocusAfterTabChange (false),
    mChangeAnimationTime (0),
    mChangeAnimationDirection (0),
    mChangeState (NoTabChange),
    mHoveredSlot (NULL),
    mTextSlot (NULL),
    mTextLayer (NULL),
    mBgLayer (NULL),
    mSelectionLayer (NULL),
    mBgAnimationTime (0),
    mBgAnimation (AnimationNone),
    mState (PaintOff),
    mAnimationTime (0),
    mOldWidth (0),
    mLeftSpringX (0),
    mRightSpringX (0),
    mLeftSpeed (0),
    mRightSpeed (0),
    mLeftMsSinceLastMove (0),
    mRightMsSinceLastMove (0),
    mInputPrevention (None),
    mIpwMapped (false)
{
    mGroup->mTabBar = this; /* only need to do this because
			     * GroupTabBar::createSlot checks
			     * for mTabBar
			     */

    mSlots.clear ();
    foreach (CompWindow *cw, mGroup->mWindows)
	createSlot (cw);

    mGroup->mTabBar->createInputPreventionWindow ();
    mGroup->mTabBar->mTopTab = GroupWindow::get (topTab)->mSlot;

    mGroup->mTabBar->recalcTabBarPos (WIN_CENTER_X (topTab),
			  WIN_X (topTab), WIN_X (topTab) + WIN_WIDTH (topTab));
}

/*
 * GroupTabBar::~GroupTabBar
 *
 */
GroupTabBar::~GroupTabBar ()
{
    while (mSlots.size ())
	deleteTabBarSlot (mSlots.front ());

    if (mTextLayer->mPixmap)
	XFreePixmap (screen->dpy (), mTextLayer->mPixmap);
    delete mTextLayer;
    delete mBgLayer;
    delete mSelectionLayer;

    mGroup->mTabBar->destroyInputPreventionWindow ();

    if (mTimeoutHandle.active ())
	mTimeoutHandle.stop ();
}

/*
 * initTab
 *
 */
bool
GroupScreen::initTab (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;
    bool       allowUntab = true;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (!w)
	return true;

    GROUP_WINDOW (w);

    if (gw->mInSelection)
    {
	groupWindows (action, state, options);
	/* If the window was selected, we don't want to
	   untab the group, because the user probably
	   wanted to tab the selected windows. */
	allowUntab = false;
    }

    if (!gw->mGroup)
	return true;

    if (!gw->mGroup->mTabBar)
	gw->mGroup->tabGroup (w);
    else if (allowUntab)
	gw->mGroup->untabGroup ();

    cScreen->damageScreen ();

    return true;
}

/*
 * changeTabLeft
 *
 */
bool
GroupScreen::changeTabLeft (CompAction          *action,
				 CompAction::State   state,
				 CompOption::Vector  options)
{
    Window     xid;
    CompWindow *w, *topTab;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = topTab = screen->findWindow (xid);
    if (!w)
	return true;

    GROUP_WINDOW (w);

    if (!gw->mSlot || !gw->mGroup || !gw->mGroup->mTabBar ||
	!gw->mGroup->mTabBar->mTopTab)
	return true;

    if (gw->mGroup->mTabBar->mNextTopTab)
	topTab = NEXT_TOP_TAB (gw->mGroup);
    else if (gw->mGroup->mTabBar->mTopTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->mGroup);
    }

    gw = GroupWindow::get (topTab);

    if (gw->mSlot->mPrev)
	return changeTab (gw->mSlot->mPrev, GroupTabBar::RotateLeft);
    else
	return changeTab (gw->mGroup->mTabBar->mSlots.back (), GroupTabBar::RotateLeft);
}

/*
 * changeTabRight
 *
 */
bool
GroupScreen::changeTabRight (CompAction         *action,
				  CompAction::State  state,
				  CompOption::Vector options)
{
    Window     xid;
    CompWindow *w, *topTab;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = topTab = screen->findWindow (xid);
    if (!w)
	return true;

    GROUP_WINDOW (w);

    if (!gw->mSlot || !gw->mGroup || !gw->mGroup->mTabBar)
	return true;

    if (gw->mGroup->mTabBar->mNextTopTab)
	topTab = NEXT_TOP_TAB (gw->mGroup);
    else if (gw->mGroup->mTabBar->mTopTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->mGroup);
    }

    gw = GroupWindow::get (topTab);

    if (gw->mSlot->mNext)
	return changeTab (gw->mSlot->mNext, GroupTabBar::RotateRight);
    else
	return changeTab (gw->mGroup->mTabBar->mSlots.front (), GroupTabBar::RotateRight);
}

/*
 * switchTopTabInput
 *
 */
void
GroupScreen::switchTopTabInput (GroupSelection *group,
				     bool	    enable)
{
    if (!group->mTabBar || !HAS_TOP_WIN (group))
	return;

    if (!group->mTabBar->mInputPrevention)
	group->mTabBar->createInputPreventionWindow ();

    if (!enable)
    {
	XMapWindow (screen->dpy (),
		    group->mTabBar->mInputPrevention);
		    
    }
    else
    {
	XUnmapWindow (screen->dpy (),
		      group->mTabBar->mInputPrevention);
    }

    group->mTabBar->mIpwMapped = !enable;
}

/*
 * GroupTabBar::createInputPreventionWindow
 *
 */
void
GroupTabBar::createInputPreventionWindow ()
{
    if (!mInputPrevention)
    {
	XSetWindowAttributes attrib;
	attrib.override_redirect = true;

	mInputPrevention = 
	    XCreateWindow (screen->dpy (),
			   screen->root (), -100, -100, 1, 1, 0,
			   CopyFromParent, InputOnly,
			   CopyFromParent, CWOverrideRedirect, &attrib);
	
	mIpwMapped = false;
    }
}

/*
 * GroupTabBar::destroyInputPreventionWindow
 *
 */
void
GroupTabBar::destroyInputPreventionWindow ()
{
    if (mInputPrevention)
    {
	XDestroyWindow (screen->dpy (),
			mInputPrevention);

	mInputPrevention = None;
	mIpwMapped = true;
    }
}
