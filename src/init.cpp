/**
 *
 * Compiz group plugin
 *
 * init.c
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
#include "group_glow.h"

COMPIZ_PLUGIN_20090315 (group, GroupPluginVTable);

bool textAvailable;

static const GlowTextureProperties glowTextureProperties[2] = {
    /* GlowTextureRectangular */
    {glowTexRect, 32, 21},
    /* GlowTextureRing */
    {glowTexRing, 32, 16}
};

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
		    group->mTabBar->renderTabBarBackground ();
	    break;
	case GroupOptions::TabbarFontSize:
	case GroupOptions::TabbarFontColor:
	    foreach (group, mGroups)
		if (group->mTabBar)
		    group->mTabBar->renderWindowTitle ();
	    break;
	case GroupOptions::ThumbSize:
	case GroupOptions::ThumbSpace:
	    foreach (group, mGroups)
		if (group->mTabBar)
		{
		    CompRect box = group->mTabBar->mRegion.boundingRect ();
		    group->mTabBar->recalcTabBarPos ((box.x1 () + box.x2 ()) / 2,
					  box.x1 (), box.x2 ());
		}
	    break;
	case GroupOptions::Glow:
	case GroupOptions::GlowSize:
	    {
		foreach (CompWindow *w, screen->windows ())
		{
		    GROUP_WINDOW (w);
		    GLTexture::Matrix tMat = mGlowTexture.at (0)->matrix ();

		    gw->groupComputeGlowQuads (&tMat);
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
			GroupWindow::get (w)->groupComputeGlowQuads (&tMat);
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
 * groupApplyInitialActions
 *
 * timer callback for stuff that needs to be called after all
 * screens and windows are initialized
 *
 */
bool
GroupScreen::groupApplyInitialActions ()
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
	if (gw->groupCheckWindowProperty (w, &id, &tabbed, color))
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

	    gw->groupAddWindowToGroup (group, id);
	    if (tabbed)
		gw->mGroup->tabGroup (w);

	    gw->mGroup->mColor[0] = color[0];
	    gw->mGroup->mColor[1] = color[1];
	    gw->mGroup->mColor[2] = color[2];

	    if (gw->mGroup->mTabBar)
		gw->mGroup->mTabBar->renderTopTabHighlight ();
	    cScreen->damageScreen ();
	}

	if (optionGetAutotabCreate () && gw->groupIsGroupWindow ())
	{
	    if (!gw->mGroup && (gw->mWindowState == WindowNormal))
	    {
		gw->groupAddWindowToGroup (NULL, 0);
		if (gw->mGroup)
		    gw->mGroup->tabGroup (w);
	    }
	}
	
	rit++;
    }

    return false;
}

/*
 * groupInitDisplay
 *
 */
GroupScreen::GroupScreen (CompScreen *s) :
    PluginClassHandler <GroupScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mIgnoreMode (false),
    mGlowTextureProperties ((GlowTextureProperties *) glowTextureProperties),
    mLastRestackedGroup (NULL),
    mGroupWinPropertyAtom (XInternAtom (screen->dpy (), "_COMPIZ_GROUP", 0)),
    mResizeNotifyAtom (XInternAtom (screen->dpy (), "_COMPIZ_RESIZE_NOTIFY", 0)),
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
    /* one-shot timeout for stuff that needs to be initialized after
       all screens and windows are initialized */
    mInitialActionsTimeoutHandle.start (boost::bind (
					&GroupScreen::groupApplyInitialActions,
					this), 0, 0);

    mDequeueTimeoutHandle.setCallback (boost::bind (
				       &GroupScreen::groupDequeueTimer, this));
    mDequeueTimeoutHandle.setTimes (0, 0);

    mGlowTexture =
    GLTexture::imageDataToTexture (mGlowTextureProperties[glowType].textureData,
				   CompSize (mGlowTextureProperties[glowType].textureSize,
					     mGlowTextureProperties[glowType].textureSize),
				   GL_RGBA, GL_UNSIGNED_BYTE);

    optionSetTabHighlightColorNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetTabBaseColorNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetTabBorderColorNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetTabbarFontSizeNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetTabbarFontColorNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetGlowNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetGlowTypeNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetGlowSizeNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetTabStyleNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetThumbSizeNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetThumbSpaceNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetBorderWidthNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));
    optionSetBorderRadiusNotify (boost::bind (&GroupScreen::optionChanged, this, _1, _2));

    optionSetSelectButtonInitiate (boost::bind (&GroupScreen::groupSelect, this, _1, _2, _3));
    optionSetSelectButtonTerminate (boost::bind (&GroupScreen::groupSelectTerminate, this, _1, _2, _3));
    optionSetSelectSingleKeyInitiate (boost::bind (&GroupScreen::groupSelectSingle, this, _1, _2, _3));
    optionSetGroupKeyInitiate (boost::bind (&GroupScreen::groupGroupWindows, this, _1, _2, _3));
    optionSetUngroupKeyInitiate (boost::bind (&GroupScreen::groupUnGroupWindows, this, _1, _2, _3));
    optionSetTabmodeKeyInitiate (boost::bind (&GroupScreen::groupInitTab, this, _1, _2, _3));
    optionSetChangeTabLeftKeyInitiate (boost::bind (&GroupScreen::groupChangeTabLeft, this, _1, _2, _3));
    optionSetChangeTabRightKeyInitiate (boost::bind (&GroupScreen::groupChangeTabRight, this, _1, _2, _3));
    optionSetRemoveKeyInitiate (boost::bind (&GroupScreen::groupRemoveWindow, this, _1, _2, _3));
    optionSetCloseKeyInitiate (boost::bind (&GroupScreen::groupCloseWindows, this, _1, _2, _3));
    optionSetIgnoreKeyInitiate (boost::bind (&GroupScreen::groupSetIgnore, this, _1, _2, _3));
    optionSetIgnoreKeyTerminate (boost::bind (&GroupScreen::groupUnsetIgnore, this, _1, _2, _3));
    optionSetChangeColorKeyInitiate (boost::bind (&GroupScreen::groupChangeColor, this, _1, _2, _3));

}

/*
 * groupFiniDisplay
 *
 */
/*
 * groupInitScreen
 *
 */

/*
 * groupFiniScreen
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
		groupDestroyCairoLayer (group->mTabBar->mBgLayer);
		groupDestroyCairoLayer (group->mTabBar->mSelectionLayer);

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
	groupGrabScreen (ScreenGrabNone);

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
 * groupInitWindow
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

    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);

    mOrgPos.x = 0;
    mOrgPos.y = 0;
    mMainTabOffset.x = 0;
    mMainTabOffset.y = 0;
    mDestination.x = 0;
    mDestination.y = 0;

    if (w->minimized ())
	mWindowState = WindowMinimized;
    else if (w->shaded ())
	mWindowState = WindowShaded;
    else
	mWindowState = WindowNormal;

    groupComputeGlowQuads (&mat);
}

/*
 * groupFiniWindow
 *
 */
GroupWindow::~GroupWindow ()
{
    if (mWindowHideInfo)
	groupSetWindowVisibility (true);

    mReadOnlyProperty = true;

    /* FIXME: this implicitly calls the wrapped function activateWindow
       * (via GroupSelection::deleteTabBarSlot -> 
       * GrouppSelection::unhookTabBarSlot ->
       * GroupSelection::changeTab)
       --> better wrap into removeObject and call it for removeWindow
       */
    if (mGroup)
	groupDeleteGroupWindow ();

    if (mGlowQuads)
	free (mGlowQuads);
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
	textAvailable = false;
    else
	textAvailable = true;

    return true;
}
