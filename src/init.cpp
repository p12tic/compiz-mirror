/**
 *
 * Compiz group plugin
 *
 * init.cpp
 *
 * Copyright : (C) 2006-2010 by Patrick Niklaus, Roi Cohen,
 * 				Danny Baumann, Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 * 	    Sam Spilsbury   <smspillaz@gmail.com>
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


COMPIZ_PLUGIN_20090315 (group, GroupPluginVTable);

/* If this is false, then there is no point in trying to render text
 * since it will fail */
bool gTextAvailable;

/* 
 * GroupScreen::optionChanged
 * 
 * An option was just changed. Since we aren't constantly re-rendering
 * things like glow, the tab bar, the font, etc, we need to re-render
 * applicable things */
void
GroupScreen::optionChanged (CompOption *opt,
			    Options    num)
{
    GroupSelection *group;

    switch (num)
    {
	case GroupOptions::TabBaseColor:
	case GroupOptions::TabHighlightColor:
	case GroupOptions::TabBorderColor:
	case GroupOptions::TabStyle:
	case GroupOptions::BorderRadius:
	case GroupOptions::BorderWidth:
	    foreach (group, mGroups)
		if (group->mTabBar)
		    group->mTabBar->mBgLayer->render ();
	    break;
	case GroupOptions::TabbarFontSize:
	case GroupOptions::TabbarFontColor:
	    foreach (group, mGroups)
		if (group->mTabBar)
		{
		    group->mTabBar->mTextLayer =
		      TextLayer::rebuild (group->mTabBar->mTextLayer);
			
		    if (group->mTabBar->mTextLayer)
			group->mTabBar->mTextLayer->render ();
		}
	    break;
	case GroupOptions::ThumbSize:
	case GroupOptions::ThumbSpace:
	    foreach (group, mGroups)
		if (group->mTabBar)
		{
		    CompRect box = group->mTabBar->mRegion.boundingRect ();
		    group->mTabBar->recalcTabBarPos (
					 (box.x1 () + box.x2 ()) / 2,
					  box.x1 (), box.x2 ());
		}
	    break;
	case GroupOptions::Glow:
	case GroupOptions::GlowSize:
	    {
		/* We have new output extents, so update them
		 * and damage them */
		foreach (CompWindow *w, screen->windows ())
		{
		    GROUP_WINDOW (w);
		    GLTexture::Matrix tMat =
					 mGlowTexture.at (0)->matrix ();

		    gw->computeGlowQuads (&tMat);
		    if (gw->mGlowQuads)
		    {
			gw->cWindow->damageOutputExtents ();
			gw->window->updateWindowOutputExtents ();
			gw->cWindow->damageOutputExtents ();
		    }
		}
		break;
	    }
	case GroupOptions::GlowType:
	    {
		int		      glowType;
		GlowTextureProperties *glowProperty;

		/* Since we have a new glow texture, we have to rebind
		 * it and recalculate it */

		glowType = optionGetGlowType ();
		glowProperty = &mGlowTextureProperties[glowType];

		mGlowTexture = GLTexture::imageDataToTexture (
				    glowProperty->textureData,
				    CompSize (glowProperty->textureSize,
					      glowProperty->textureSize),
				    GL_RGBA, GL_UNSIGNED_BYTE);

		if (optionGetGlow () && mGroups.size ())
		{
		    foreach (CompWindow *w, screen->windows ())
		    {
			GLTexture::Matrix tMat = mGlowTexture.at (0)->matrix ();
			GroupWindow::get (w)->computeGlowQuads (&tMat);
		    }

		    cScreen->damageScreen ();
		}
		break;
	    }

	default:
	    break;
    }
}

/*
 * GroupScreen::applyInitialActions
 *
 * timer callback for stuff that needs to be called after all
 * screens and windows are initialized
 *
 */
bool
GroupScreen::applyInitialActions ()
{
    CompWindowList::reverse_iterator rit = screen->windows ().rbegin ();
    /* we need to do it from top to buttom of the stack to avoid problems
       with a reload of Compiz and tabbed static groups. (topTab will always
       be above the other windows in the group) */
    while (rit != screen->windows ().rend ())
    {
	bool     tabbed;
	long int id;
	GLushort color[3];
	CompWindow *w = *rit;

	GROUP_WINDOW (w);

	/* read window property to see if window was grouped
	   before - if it was, regroup */
	if (gw->checkWindowProperty (w, &id, &tabbed, color))
	{
	    GroupSelection *group = NULL;
	    bool	   found = false;

	    foreach (group, mGroups)
	    {
		if (group->mIdentifier == id)
		{
		    found = true;
		    break;
		}
	    }
	    
	    if (!found)
		group = NULL;

	    /* Add this window to a group (with that id) */
	    gw->addWindowToGroup (group, id);
	    if (tabbed)
		gw->mGroup->tabGroup (w);


	    /* Restore color */
	    gw->mGroup->mColor[0] = color[0];
	    gw->mGroup->mColor[1] = color[1];
	    gw->mGroup->mColor[2] = color[2];

	    /* if there was a tab bar, re-render it */
	    if (gw->mGroup->mTabBar)
	    {
		CompRegion     &r = gw->mGroup->mTabBar->mRegion;
		SelectionLayer *l = gw->mGroup->mTabBar->mSelectionLayer;

		CompSize size (r.boundingRect ().width (),
			       r.boundingRect ().height ());
		gw->mGroup->mTabBar->mSelectionLayer =
		       SelectionLayer::rebuild (l, size);
		if (gw->mGroup->mTabBar->mSelectionLayer)
		    gw->mGroup->mTabBar->mSelectionLayer->render ();
	    }
	    cScreen->damageScreen ();
	}

	/* Otherwise, add this window to a group on it's own if we are
	 * auto-tabbing */
	if (optionGetAutotabCreate () && gw->isGroupWindow ())
	{
	    if (!gw->mGroup && (gw->mWindowState ==
				GroupWindow::WindowNormal))
	    {
		gw->addWindowToGroup (NULL, 0);
		if (gw->mGroup)
		    gw->mGroup->tabGroup (w);
	    }
	}
	
	rit++;
    }

    return false;
}

/*
 * GroupScreen::GroupScreen
 * 
 * Constructor for GroupScreen. Set up atoms, glow texture, queues, etc
 *
 */
GroupScreen::GroupScreen (CompScreen *s) :
    PluginClassHandler <GroupScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mIgnoreMode (false),
    mGlowTextureProperties ((GlowTextureProperties *) glowTextureProperties),
    mLastRestackedGroup (NULL),
    mGroupWinPropertyAtom (XInternAtom (screen->dpy (),
					"_COMPIZ_GROUP", 0)),
    mResizeNotifyAtom (XInternAtom (screen->dpy (),
				    "_COMPIZ_RESIZE_NOTIFY", 0)),
    mPendingMoves (NULL),
    mPendingGrabs (NULL),
    mPendingUngrabs (NULL),
    mGroups (NULL),
    mQueued (false),
    mGrabState (ScreenGrabNone),
    mGrabIndex (0),
    mLastHoveredGroup (NULL),
    mDraggedSlot (NULL),
    mDragged (false),
    mPrevX (0),
    mPrevY (0) ,
    mLastGrabbedWindow (None)  
{
    ScreenInterface::setHandler (screen);
    GLScreenInterface::setHandler (gScreen);
    CompositeScreenInterface::setHandler (cScreen);

    int glowType = optionGetGlowType ();
    boost::function <void (CompOption *, GroupOptions::Options)> oSetCb
	    = boost::bind (&GroupScreen::optionChanged,
					  this, _1, _2);
    /* one-shot timeout for stuff that needs to be initialized after
       all screens and windows are initialized */
    mInitialActionsTimeoutHandle.start (boost::bind (
					&GroupScreen::applyInitialActions,
					this), 0, 0);

    mDequeueTimeoutHandle.setCallback (boost::bind (
				       &GroupScreen::dequeueTimer, this));
    mDequeueTimeoutHandle.setTimes (0, 0);

    /* Bind the glow texture now */
    mGlowTexture =
    GLTexture::imageDataToTexture (mGlowTextureProperties[glowType].textureData,
				   CompSize (mGlowTextureProperties[glowType].textureSize,
					     mGlowTextureProperties[glowType].textureSize),
				   GL_RGBA, GL_UNSIGNED_BYTE);

    /* Set option callback code */
    optionSetTabHighlightColorNotify (oSetCb);
    optionSetTabBaseColorNotify (oSetCb);
    optionSetTabBorderColorNotify (oSetCb);
    optionSetTabbarFontSizeNotify (oSetCb);
    optionSetTabbarFontColorNotify (oSetCb);
    optionSetGlowNotify (oSetCb);
    optionSetGlowTypeNotify (oSetCb);
    optionSetGlowSizeNotify (oSetCb);
    optionSetTabStyleNotify (oSetCb);
    optionSetThumbSizeNotify (oSetCb);
    optionSetThumbSpaceNotify (oSetCb);
    optionSetBorderWidthNotify (oSetCb);
    optionSetBorderRadiusNotify (oSetCb);

    optionSetSelectButtonInitiate (boost::bind (&GroupScreen::select,
						     this, _1, _2, _3));
    optionSetSelectButtonTerminate (boost::bind 
					(&GroupScreen::selectTerminate,
						     this, _1, _2, _3));
    optionSetSelectSingleKeyInitiate (boost::bind 
					(&GroupScreen::selectSingle,
						     this, _1, _2, _3));
    optionSetGroupKeyInitiate (boost::bind 
					(&GroupScreen::groupWindows,
						     this, _1, _2, _3));
    optionSetUngroupKeyInitiate (boost::bind 
					(&GroupScreen::ungroupWindows,
						     this, _1, _2, _3));
    optionSetTabmodeKeyInitiate (boost::bind (&GroupScreen::initTab,
						     this, _1, _2, _3));
    optionSetChangeTabLeftKeyInitiate (boost::bind 
					   (&GroupScreen::changeTabLeft,
						     this, _1, _2, _3));
    optionSetChangeTabRightKeyInitiate (boost::bind 
					  (&GroupScreen::changeTabRight,
						     this, _1, _2, _3));
    optionSetRemoveKeyInitiate (boost::bind (&GroupScreen::removeWindow,
						     this, _1, _2, _3));
    optionSetCloseKeyInitiate (boost::bind (&GroupScreen::closeWindows,
						     this, _1, _2, _3));
    optionSetIgnoreKeyInitiate (boost::bind (&GroupScreen::setIgnore,
						     this, _1, _2, _3));
    optionSetIgnoreKeyTerminate (boost::bind (&GroupScreen::unsetIgnore,
						     this, _1, _2, _3));
    optionSetChangeColorKeyInitiate (boost::bind
					     (&GroupScreen::changeColor,
						     this, _1, _2, _3));

}

/*
 * GroupScreen::~GroupScreen
 * 
 * Screen properties tear-down, delete all the groups, destroy IPWs
 * etc
 *
 */
GroupScreen::~GroupScreen ()
{
    if (mGroups.size ())
    {
	GroupSelection *group;
	GroupSelection::List::iterator it = mGroups.end ();

	while (it != mGroups.begin ())
	{
	    group = *it;
	    
	    if (group->mTabBar)
	    {
		GroupTabBarSlot *slot, *nextSlot;

		while (slot)
		{
		    nextSlot = slot->mNext;
		    delete slot;
		    slot = nextSlot;
		}

		if (group->mTabBar->mTextLayer->mPixmap)
		    XFreePixmap (screen->dpy (),
				 group->mTabBar->mTextLayer->mPixmap);
		delete group->mTabBar->mBgLayer;
		delete group->mTabBar->mSelectionLayer;

		if (group->mTabBar->mInputPrevention)
		    XDestroyWindow (screen->dpy (),
				    group->mTabBar->mInputPrevention);

		if (group->mTabBar->mTimeoutHandle.active ())
		    group->mTabBar->mTimeoutHandle.stop ();

		delete group->mTabBar;
	    }

	    delete group;
	    it--;
	}
    }

    mTmpSel.clear ();

    if (mGrabIndex)
	grabScreen (ScreenGrabNone);

    if (mDragHoverTimeoutHandle.active ())
	mDragHoverTimeoutHandle.stop ();

    if (mShowDelayTimeoutHandle.active ())
	mShowDelayTimeoutHandle.stop ();

    if (mDequeueTimeoutHandle.active ())
	mDequeueTimeoutHandle.stop ();

    if (mInitialActionsTimeoutHandle.active ())
	mInitialActionsTimeoutHandle.stop ();
}

/*
 * GroupWindow::checkFunctions
 * 
 * Function to check if we need to enable any of our wrapped
 * functions.
 * 
 */

#define GL_PAINT (1 << 0)
#define GL_DRAW (1 << 1)
#define DAMAGE_RECT (1 << 2)
#define GET_OUTPUT_EXTENTS (1 << 3)
#define MOVE_NOTIFY (1 << 4)
#define RESIZE_NOTIFY (1 << 5)
#define GRAB_NOTIFY (1 << 6)
#define WINDOW_NOTIFY (1 << 7)
#define STATECHANGE_NOTIFY (1 << 8)
#define ACTIVATE_NOTIFY (1 << 9)

void
GroupWindow::checkFunctions ()
{
    unsigned long functionsMask = 0;
    
    /* For glPaint, the window must either be:
     * -> In an animation (eg rotating, tabbing, etc)
     * -> Having its tab bar shown
     * -> "Selected" (but not yet grouped)
     * -> Being Stretched
     * -> Have a hide info struct (since we need to hide the window)
     */

    if (checkRotating () || checkTabbing () || checkShowTabBar ()
	|| !mResizeGeometry.isEmpty () || mWindowHideInfo
	|| mInSelection)
	functionsMask |= GL_PAINT;
	
 
    /* For glDraw, the window must be:
     * -> Window must be in a group
     * -> Window must have glow quads
     */
     
    if (mGroup && (mGroup->mWindows.size () > 1) && mGlowQuads);
	functionsMask |= GL_DRAW;

    gWindow->glPaintSetEnabled (this, functionsMask & GL_PAINT);
    gWindow->glDrawSetEnabled (this, functionsMask & GL_DRAW);
}

/*
 * GroupWindow::GroupWindow
 * 
 * Constructor for GroupWindow, set up the hide info, animation state
 * resize geometry, glow quads etc
 *
 */
GroupWindow::GroupWindow (CompWindow *w) :
    PluginClassHandler <GroupWindow, CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    mGroup (NULL),
    mInSelection (false),
    mReadOnlyProperty (false),
    mSlot (NULL),
    mNeedsPosSync (false),
    mGlowQuads (NULL),
    mWindowHideInfo (NULL),
    mResizeGeometry (CompRect (0, 0, 0, 0)),
    mAnimateState (0),
    mTx (0.0f),
    mTy (0.0f),
    mXVelocity (0.0f),
    mYVelocity (0.0f)
{
    GLTexture::Matrix mat;

    GROUP_SCREEN (screen);

    mat = gs->mGlowTexture.front ()->matrix ();

    WindowInterface::setHandler (window, true);
    CompositeWindowInterface::setHandler (cWindow, true);
    GLWindowInterface::setHandler (gWindow, true);
    
    gWindow->glDrawSetEnabled (this, false);
    gWindow->glPaintSetEnabled (this, false);

    mOrgPos = CompPoint (0, 0);
    mMainTabOffset = CompPoint (0, 0);
    mDestination = CompPoint (0, 0);

    if (w->minimized ())
	mWindowState = WindowMinimized;
    else if (w->shaded ())
	mWindowState = WindowShaded;
    else
	mWindowState = WindowNormal;

    computeGlowQuads (&mat);
}

/*
 * GroupWindow::~GroupWindow
 * 
 * Tear down for when we don't need group data on a window anymore
 *
 */
GroupWindow::~GroupWindow ()
{
    if (mWindowHideInfo)
	setWindowVisibility (true);

    mReadOnlyProperty = true;

    /* FIXME: this implicitly calls the wrapped function activateWindow
       * (via GroupSelection::deleteTabBarSlot -> 
       * GrouppSelection::unhookTabBarSlot ->
       * GroupSelection::changeTab)
       --> better wrap into removeObject and call it for removeWindow
       */
    if (mGroup)
	deleteGroupWindow ();

    if (mGlowQuads)
	delete[] mGlowQuads;
}

bool
GroupPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;
	
    if (!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
	gTextAvailable = false;
    else
	gTextAvailable = true;

    return true;
}
