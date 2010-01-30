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

bool textAvailable;

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
	    foreach (Group *group, groups)
		if (group->tabBar && group->tabBar->bgLayer)
		    group->tabBar->renderTabBarBackground ();
	    break;
	case GroupOptions::TabbarFontSize:
	case GroupOptions::TabbarFontColor:
	    foreach (Group *group, groups)
	    {
		if (group->tabBar && group->tabBar->textLayer)
		    delete group->tabBar->textLayer;
		group->tabBar->renderWindowTitle ();
	    }
	    break;
	case GroupOptions::ThumbSize:
	case GroupOptions::ThumbSpace:
	    foreach (Group *group, groups)
	    {
		if (group->tabBar)
		{
		    CompRect box = group->tabBar->region.boundingRect ();
		    group->tabBar->recalcPos (box.x1 () + box.x2 ()  / 2,
					      box.x1 (), box.x2 ());
		}
	    }
	    break;
	case GroupOptions::Glow:
	case GroupOptions::GlowSize:
	    {
		CompWindow *w;

		foreach (CompWindow *w, screen->windows ())
		{
		    GROUP_WINDOW (w);

		    GLTexture::Matrix mat =
		    		        glowTexture.texture.front ()->matrix ();

		    gw->computeGlowQuads (&mat);
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
			GLTexture::Matrix mat =
				        glowTexture.texture.front ()->matrix ();

			gw->computeGlowQuads (&mat);
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
    CompWindowList::reverse_iterator it = screen->windows ().rbegin ();

    initialActionsTimeoutHandle.stop ();

    /* we need to do it from top to buttom of the stack to avoid problems
       with a reload of Compiz and tabbed static groups. (topTab will always
       be above the other windows in the group) */
    while (it != screen->windows ().rend ())
    {
	CompWindow *w = *it;
	it++;

	bool     tabbed;
	long int id;
	GLushort color[3];

	GROUP_WINDOW (w);

	/* read window property to see if window was grouped
	   before - if it was, regroup */
	if (gw->checkProperty (id, tabbed, color))
	{
	    Selection sel;
	    Group *foundGroup = NULL;
	    
	    foreach (Group *group, groups)
		if (group->identifier == id)
		{
		    foundGroup = group;
		    break;
		}

	    if (foundGroup)
	        foundGroup->addWindow (w);
	    else
	    {
		sel.push_back (w);
		foundGroup = sel.toGroup ();
	    }
	    if (tabbed)
		foundGroup->tab (w);

	    foundGroup->color[0] = color[0];
	    foundGroup->color[1] = color[1];
	    foundGroup->color[2] = color[2];

	    if (foundGroup->tabBar && foundGroup->tabBar->selectionLayer)
		foundGroup->tabBar->renderTopTabHighlight ();

	    cScreen->damageScreen ();
	}

	if (optionGetAutotabCreate () && gw->is ())
	{
	    if (!gw->group && (gw->windowState == GroupWindow::WindowNormal))
	    {
		Selection sel;
		sel.push_back (w);
		
		sel.toGroup ();
		if (gw->group)
		    gw->group->tab (w);
	    }
	}
    }

    return false;
}

/*
 * groupInitDisplay
 *
 */



GroupScreen::GroupScreen (CompScreen *screen) :
    PluginClassHandler <GroupScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    ignoreMode (false),
    resizeInfo (NULL),
    lastRestackedGroup (NULL),
    groupWinPropertyAtom (XInternAtom (screen->dpy (), "_COMPIZ_GROUP", 0)),
    resizeNotifyAtom (XInternAtom (screen->dpy (), "_COMPIZ_RESIZE_NOTIFY", 0)),
    queued (false),
    grabState (ScreenGrabNone),
    grabIndex (0),
    lastHoveredGroup (NULL),
    draggedSlot (NULL),
    dragged (false),
    prevX (0),
    prevY (0)   
{

    dequeueTimeoutHandle.setTimes (0, 0);
    dequeueTimeoutHandle.setCallback (boost::bind (&GroupScreen::dequeue,
									 this));
    dequeueTimeoutHandle.stop ();

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

    optionSetGroupKeyInitiate (boost::bind (&GroupScreen::groupWindows, this,
					     _1, _2, _3));
    optionSetUngroupKeyInitiate (boost::bind (&GroupScreen::ungroupWindows,
					      this, _1, _2, _3));

    optionSetTabmodeKeyInitiate (boost::bind (&GroupScreen::initTab, this, _1,
					      _2, _3));
    optionSetChangeTabLeftKeyInitiate (boost::bind (&GroupScreen::changeTabLeft,
    						     this, _1, _2, _3));
    optionSetChangeTabRightKeyInitiate (boost::bind (&GroupScreen::changeTabRight,
						     this, _1, _2, _3));

    optionSetRemoveKeyInitiate (boost::bind (&GroupScreen::removeWindow, this,
					     _1, _2, _3));



    optionSetCloseKeyInitiate (boost::bind (&GroupScreen::closeWindows, this,
					    _1, _2, _3));
    optionSetIgnoreKeyInitiate (boost::bind (&GroupScreen::setIgnore, this, _1,
					     _2, _3));
    optionSetIgnoreKeyTerminate (boost::bind (&GroupScreen::unsetIgnore, this,
					      _1, _2, _3));
/*
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

	group->destroy (true);
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
    glowQuads (NULL),
    selection (NULL),
    group (NULL),
    inSelection (false),
    readOnlyProperty (false),
    tab (NULL),
    needsPosSync (false),
    windowHideInfo (NULL),
    resizeGeometry (0, 0, 0, 0),
    animateState (0),
    tx (0),
    ty (0),
    xVelocity (0.0),
    yVelocity (0.0)
{
    GROUP_SCREEN (screen);
    GLTexture::Matrix mat = gs->glowTexture.texture.front ()->matrix ();

    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);

    if (window->minimized ())
	windowState = WindowMinimized;
    else if (window->shaded ())
	windowState = WindowShaded;
    else
	windowState = WindowNormal;

    computeGlowQuads (&mat);
}

GroupWindow::~GroupWindow ()
{
    /*if (windowHideInfo)
	setVisibility (true);*/

    readOnlyProperty = TRUE;
    
    if (glowQuads)
	delete[] glowQuads;

    /* FIXME: this implicitly calls the wrapped function activateWindow
       (via groupDeleteTabBarSlot -> groupUnhookTabBarSlot -> groupChangeTab)
       --> better wrap into removeObject and call it for removeWindow
       */

    if (group)
	deleteGroupWindow ();
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
