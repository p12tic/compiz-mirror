/**
 *
 * Compiz group plugin
 *
 * init.cpp
 *
 * Copyright : (C) 2006-2009 by Patrick Niklaus, Roi Cohen, Danny Baumann,
 *				Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *	    Sam Spilsbury   <smspillaz@gmail.com>
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

void
GroupScreen::optionChanged (CompOption            *opt,
			    GroupOptions::Options num)
{

    switch (num)
    {
	case GroupOptions::TabBaseColor:
	case GroupOptions::TabHighlightColor:
	case GroupOptions::TabBorderColor:
	case GroupOptions::TabStyle:
	case GroupOptions::BorderRadius:
	case GroupOptions::BorderWidth:
	    /*foreach (Group *group, groups)
		if (group->tabBar && group->tabBar->bgLayer)
		    group->tabBar->bgLayer-renderTabBarBackground (group);*/
	    break;
	case GroupOptions::TabbarFontSize:
	case GroupOptions::TabbarFontColor:
	    /*foreach (Group *group, groups)
	    {
		if (group->tabBar && group->tabBar->textLayer)
		    delete group->tabBar->textLayer;
		group->tabBar->textLayer = TextLayer::renderWindowTitle (group);
	    }*/
	    break;
	case GroupOptions::ThumbSize:
	case GroupOptions::ThumbSpace:
	    /*foreach (Group *group, groups)
	    {
		if (group->tabBar)
		{
		    CompRect box = group->tabBar->region->boundingRect ();
		    group->tabBar->recalcTabBarPos (box->x1 () + box->x2 () ) / 2,
					  	    box->x1 (), box->x2 ());
		}
	    }*/
	    break;
	case GroupOptions::Glow:
	case GroupOptions::GlowSize:
	    {
		CompWindow *w;

		foreach (CompWindow *w, screen->windows ())
		{
		    GROUP_WINDOW (w);

		    //gw->glowQuads.computeGlowQuads (w, glowQuads.matrix);
		    if (!gw->glowQuads)
		    {
			gw->cWindow->damageOutputExtents ();
			//w->updateOutputExtents ();
			gw->cWindow->damageOutputExtents ();
		    }
		}
		break;
	    }
	case GroupOptions::GlowType:
	    {
		GlowTexture::Properties *glowProperty;

		glowProperty = &glowTexture.properties[optionGetGlowType ()];

		glowTexture.texture = 
		GLTexture::imageDataToTexture
		 (glowProperty->textureData,
		  CompSize (glowProperty->textureSize,
			    glowProperty->textureSize),
		  GL_RGBA, GL_UNSIGNED_BYTE);

		if (optionGetGlow () && !groups.empty ())
		{
		    foreach (CompWindow *w, screen->windows ())
		     {
			GROUP_WINDOW (w);
			//gw->glowQuads.computeGlowQuads (w, gw->glowQuads.matrix);
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
GroupScreen::applyInitialActions ()
{
    CompWindowList::iterator it = screen->windows ().end ();

    initialActionsTimeoutHandle.stop ();

    /* we need to do it from top to buttom of the stack to avoid problems
       with a reload of Compiz and tabbed static groups. (topTab will always
       be above the other windows in the group) */
    while (it != screen->windows ().begin ())
    {
	CompWindow *w = *(it);

	if (!w)
        {
	    it--;
	    continue; // ???
	}

	bool     tabbed;
	long int id;
	GLushort color[3];

	GROUP_WINDOW (w);

	/* read window property to see if window was grouped
	   before - if it was, regroup */
	/*if (gw->checkWindowProperty (id, tabbed, color))
	{
	    foreach (Group *group, groups)
		if (group->identifier == id)
		    break;

	    gw->addToGroup (group, id);
	    if (tabbed)
		group->tab (w);

	    gw->group->color[0] = color[0];
	    gw->group->color[1] = color[1];
	    gw->group->color[2] = color[2];

	    if (gw->group->tabBar && gw->group->tabBar->selectionLayer)
		gw->group->tabBar->selectionLayer->renderTopTabHighlight (gw->group);

	    cScren->damageScreen ();
	}*/

	/*if (optionGetAutotabCreate (s) && gw->isGroupable ()))
	{
	    if (!gw->group && (gw->windowState == GroupWindow::WindowNormal))
	    {
		gw->addToGroup (NULL, 0);
		if (gw->group)
		    gw->group->tab ();
	    }
	}*/

	it--;
    }

    return false;
}

/*
 * groupInitDisplay
 *
 */

#if 0
TabBar::~TabBar
{
		GroupTabBarSlot *slot, *nextSlot;

		for (slot = group->tabBar->slots; slot;)
		{
		    if (slot->region)
			XDestroyRegion (slot->region);

		    nextSlot = slot->next;
		    free (slot);
		    slot = nextSlot;
		}

		groupDestroyCairoLayer (s, group->tabBar->textLayer);
		groupDestroyCairoLayer (s, group->tabBar->bgLayer);
		groupDestroyCairoLayer (s, group->tabBar->selectionLayer);

		if (group->inputPrevention)
		    XDestroyWindow (s->display->display,
				    group->inputPrevention);

		if (group->tabBar->region)
		    XDestroyRegion (group->tabBar->region);

		if (group->tabBar->timeoutHandle)
		    compRemoveTimeout (group->tabBar->timeoutHandle);

		free (group->tabBar);
}
#endif

GroupScreen::GroupScreen (CompScreen *screen) :
    PluginClassHandler <GroupScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    ignoreMode (false),
    resizeInfo (NULL),
    lastRestackedGroup (NULL),
    groupWinPropertyAtom (XInternAtom (screen->dpy (), "_COMPIZ_GROUP", 0)),
    resizeNotifyAtom (XInternAtom (screen->dpy (), "_COMPIZ_RESIZE_NOTIFY", 0)),
    grabState (ScreenGrabNone),
    grabIndex (0),
    lastHoveredGroup (NULL),
    draggedSlot (NULL),
    dragged (false),
    prevX (0),
    prevY (0)   
{

    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    glowTexture.properties[0].textureData = glowTexRect;
    glowTexture.properties[0].textureSize = 32;
    glowTexture.properties[0].glowOffset = 21;
    glowTexture.properties[1].textureData = glowTexRing;
    glowTexture.properties[1].textureSize = 32;
    glowTexture.properties[1].glowOffset = 16;

    optionSetSelectButtonInitiate (boost::bind (&GroupScreen::select, this,
						_1, _2, _3));
    optionSetSelectButtonTerminate (boost::bind (&GroupScreen::selectTerminate,
						 this, _1, _2, _3));
    optionSetSelectSingleKeyInitiate (boost::bind (&GroupScreen::selectSingle,
						   this, _1, _2, _3));
/*
    optionSetGroupKeyInitiate (boost::bind (&GroupScreen::groupWindows, this
					     _1, _2, _3));
    optionSetUngroupKeyInitiate (boost::bind (&GroupScreen::unGroupWindows,
					      this, _1, _2, _3));
    optionSetTabmodeKeyInitiate (boost::bind (&GroupScreen::initTab, this, _1,
					      _2, _3));
    optionSetChangeTabLeftKeyInitiate (boost::bind (&GroupScreen::changeTabLeft,
						     _1, _2, _3));
    optionSetChangeTabRightKeyInitiate (boost::bind (&GroupScreen::changeTabRight,
						     this, _1, _2, _3));
    optionSetRemoveKeyInitiate (boost::bind (&GroupScreen::removeWindow, this,
					     _1, _2, _3));
    optionSetCloseKeyInitiate (boost::bind (&GroupScreen::closeWindows, this,
					    _1, _2, _3));
    optionSetIgnoreKeyInitiate (boost::bind (&GroupScreen::setIgnore, this, _1
					     _2, _3));
    optionSetIgnoreKeyTerminate (boost::bind (&GroupScreen::unsetIgnore, this,
					      _1, _2, _3));
    optionSetChangeColorKeyInitiate (boost::bind (&GroupScreen::changeColor,
						  this, _1, _2, _3));
*/
    srand (time (NULL));

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



    /* one-shot timeout for stuff that needs to be initialized after
       all screens and windows are initialized */
    initialActionsTimeoutHandle.start (boost::bind (
				&GroupScreen::applyInitialActions, this), 0, 0);

    GlowTexture::Properties *glowProperty;

    glowProperty = &glowTexture.properties[optionGetGlowType ()];

    glowTexture.texture = 
    GLTexture::imageDataToTexture (glowProperty->textureData,
				   CompSize (glowProperty->textureSize,
				             glowProperty->textureSize),
			     	   GL_RGBA, GL_UNSIGNED_BYTE);
}

GroupScreen::~GroupScreen ()
{
    while (!groups.empty ())
    {
	Group *group = groups.back ();

	delete group;
    }

    /*if (grabIndex)
	grabScreen (ScreenGrabNone);*/

    if (dragHoverTimeoutHandle.active ())
	dragHoverTimeoutHandle.stop ();

    if (showDelayTimeoutHandle.active ())
	showDelayTimeoutHandle.stop ();

    if (dequeueTimeoutHandle.active ())
	dequeueTimeoutHandle.stop ();

    if (initialActionsTimeoutHandle.active ())
	initialActionsTimeoutHandle.stop ();
}


GroupWindow::GroupWindow (CompWindow *window) :
    PluginClassHandler <GroupWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    selection (NULL),
    group (NULL),
    inSelection (false),
    readOnlyProperty (false),
    tab (NULL),
    needsPosSync (false),
    windowHideInfo (NULL),
    resizeGeometry (NULL),
    animateState (0),
    tx (0),
    ty (0),
    xVelocity (0.0),
    yVelocity (0.0)
{
    
    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);

    if (window->minimized ())
	windowState = WindowMinimized;
    else if (window->shaded ())
	windowState = WindowShaded;
    else
	windowState = WindowNormal;

    ///glowQuads.computeGlowQuads (window, glowQuads.matrix);
}

GroupWindow::~GroupWindow ()
{
    /*if (windowHideInfo)
	setVisibility (true);*/

    readOnlyProperty = TRUE;

    /* FIXME: this implicitly calls the wrapped function activateWindow
       (via groupDeleteTabBarSlot -> groupUnhookTabBarSlot -> groupChangeTab)
       --> better wrap into removeObject and call it for removeWindow
       */

    /*if (group)
	deleteGroupWindow ();*/
}

bool
GroupPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;

    return true;
}
