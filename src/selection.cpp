/**
 *
 * Compiz group plugin
 *
 * selection.cpp
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

/**
 * Selection::toGroup ()
 *
 */

Group *
Selection::toGroup ()
{
    int            i;
    Group          *retGroup = NULL;
    Bool           tabbed = FALSE;

    /* check if there is an existing group or if the group is tabbed */

    foreach (CompWindow *w, *this)
    {
        GROUP_WINDOW (w);

        if (gw->group)
        {
	    if (!tabbed || gw->group->tabBar)
	        retGroup = gw->group;

	    if (retGroup->tabBar)
	        tabbed = TRUE;
        }
    }

    if (!retGroup)
	retGroup = Group::create (0);

    /* we need to do one first to get the pointer of a new group */
/*    cw = this->front ();
    GROUP_WINDOW (cw);

    if (gw->group && (group != gw->group))
        gw->removeFromGroup ();
    group->addWindow (cw, 0);
    gw->cWindow->addDamage ();

    gw->inSelection = false;
    group = gw->group;*/

    foreach (CompWindow *w, *this)
    {
        GROUP_WINDOW (w);

        if (gw->group && (retGroup != gw->group))
            gw->removeFromGroup ();
        retGroup->addWindow (w);
        gw->cWindow->addDamage ();

        gw->inSelection = FALSE;
	
	gw->updateProperty ();

    }

    /* exit selection */
    clear ();

    return retGroup;
}

/**
 * Selection::push_back (CompWindow *)
 *
 */

void
Selection::push_back (CompWindow *w)
{
    CompWindowList::push_back (w);
}

/**
 * Selection::push_back (Selection)
 * This function doesn't really add a whole selection as a list item
 * but adds the windows within it that are not already selected
 */

void
Selection::push_back (Selection &sel)
{
    /* First remove windows that are already in the list
     */

    Selection::iterator it1 = this->end ();
    bool ok;

    while (it1 != this->begin ())
    {
	ok = true;
	CompWindow *list = *it1;

	Selection::iterator it2 = sel.end ();

	while (it2 != sel.end ())
	{
	    CompWindow *selection = *it2;

	    if (list == selection)
	    {
		this->erase (it1);
		sel.erase (it2);

		ok = false;
		break;
	    }
	}

	if (!ok)
	    it1 = this->end ();
	else
	    it1--;
    }

    foreach (CompWindow *w, sel)
	GroupWindow::get (w)->select ();

}


/**
 * SelectionRect::damage
 *
 */

void
SelectionRect::damage (int xRoot, int yRoot)
{
    GROUP_SCREEN (screen);

    CompRect damageRect;

    damageRect.setX (MIN (x1 (), x2 ()) - 5);
    damageRect.setY (MIN (y1 (), y2 ()) - 5);
    damageRect.setWidth ((MAX (x1 (), x2 ()) + 5) - (MIN (x1 (), x2 ()) - 5));
    damageRect.setHeight ((MAX (y1 (), y2 ()) + 5) - (MIN (y1 (), y2 ()) - 5));

    CompRegion oldDamageRegion (damageRect);

    gs->cScreen->damageRegion (oldDamageRegion);

    setWidth (xRoot - x1 ());
    setHeight (yRoot - y1 ());

    damageRect.setX (MIN (x1 (), x2 ()) - 5);
    damageRect.setY (MIN (y1 (), y2 ()) - 5);
    damageRect.setWidth ((MAX (x1 (), x2 ()) + 5) - (MIN (x1 (), x2 ()) - 5));
    damageRect.setHeight ((MAX (y1 (), y2 ()) + 5) - (MIN (y1 (), y2 ()) - 5));

    CompRegion newDamageRegion (damageRect);

    gs->cScreen->damageRegion (newDamageRegion);
}

/**
 * SelectionRect::paint
 *
 */
void
SelectionRect::paint (const GLScreenPaintAttrib &sa,
		      const GLMatrix		&transform,
		      CompOutput		*output,
		      bool			transformed)
{

    int fx1, fx2, fy1, fy2;

    GROUP_SCREEN (screen);

    fx1 = MIN (x1 (), x2 ());
    fy1 = MIN (y1 (), y2 ());
    fx2 = MAX (x1 (), x2 ());
    fy2 = MAX (y1 (), y2 ());

    if (gs->grabState == GroupScreen::ScreenGrabSelect)
    {
	GLMatrix sTransform (transform);

	if (transformed)
	{
	    gs->gScreen->glApplyTransform (sa, output, &sTransform);
	    sTransform.toScreenSpace (output, -sa.zTranslate);
	} else
	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable (GL_BLEND);

	glColor4usv (gs->optionGetFillColor ());
	glRecti (fx1, fy2, fx2, fy1);

	glColor4usv (gs->optionGetLineColor ());
	glBegin (GL_LINE_LOOP);
	glVertex2i (fx1, fy1);
	glVertex2i (fx2, fy1);
	glVertex2i (fx2, fy2);
	glVertex2i (fx1, fy2);
	glEnd ();

	glColor4usv (defaultColor);
	glDisable (GL_BLEND);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glPopMatrix ();
    }
}

/*
 * SelectionRect::toSelection
 *
 */
Selection
SelectionRect::toSelection ()
{
    GROUP_SCREEN (screen);

    float      precision = gs->optionGetSelectPrecision () / 100.0f;
    Selection	sel;
    SelectionRect  &msr = gs->masterSelectionRect;
    /* XXX: Why are we using masterSelectionRect here ... */
    CompRegion reg (MIN (msr.x1 (), msr.x2 ()) - 2,
		    MIN (msr.y1 (), msr.y2 ()) - 2,
		    (MAX (msr.x1 (), msr.x2 ()) -
                     MIN (msr.x1 (), msr.x2 ()) + 4),
		    (MAX (msr.y1 (), msr.y2 ()) -
                     MIN (msr.y1 (), msr.y2 ()) + 4));
    CompWindowList::iterator it = screen->windows ().end ();


    while (it != screen->windows ().begin ())
    {
	it--;
	CompWindow *w = *it;

	GROUP_WINDOW (w);

	if (gw->is () &&
	    gw->inRegion (reg, precision))
	{
	    GROUP_WINDOW (w);

	    /*if (gw->group && groupFindGroupInWindows (gw->group, ret, count))
		continue;*/

	    sel.push_back (w);
	}
    }

    return sel;
}
