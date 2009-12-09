#include "group.h"

/*
 * GroupWindow::glPaint ()
 *
 */

/*
 * groupPaintWindow
 *
 */
bool
GroupWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      const CompRegion		&region,
		      unsigned int		mask)
{
    Bool       status;
    Bool       doRotate, doTabbing, showTabbar;

    GROUP_SCREEN (screen);

    /*

    if (gw->group)
    {
	GroupSelection *group = gw->group;

	doRotate = (group->changeState != NoTabChange) &&
	           HAS_TOP_WIN (group) && HAS_PREV_TOP_WIN (group) &&
	           (IS_TOP_TAB (w, group) || IS_PREV_TOP_TAB (w, group));

	doTabbing = (gw->animateState & (IS_ANIMATED | FINISHED_ANIMATION)) &&
	            !(IS_TOP_TAB (w, group) &&
		      (group->tabbingState == Tabbing));

	showTabbar = group->tabBar && (group->tabBar->state != PaintOff) &&
	             (((IS_TOP_TAB (w, group)) &&
		       ((group->changeState == NoTabChange) ||
			(group->changeState == TabChangeNewIn))) ||
		      (IS_PREV_TOP_TAB (w, group) &&
		       (group->changeState == TabChangeOldOut)));
    }
    else
    {
*/
	doRotate   = FALSE;
	doTabbing  = FALSE;
	showTabbar = FALSE;
    //}
/*
    if (gw->windowHideInfo)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
*/
    if (inSelection || resizeGeometry || doRotate ||
	doTabbing || showTabbar)
    {
	GLWindowPaintAttrib wAttrib = attrib;
	GLMatrix            wTransform = transform;
	float               animProgress = 0.0f;
	int                 drawnPosX = 0, drawnPosY = 0;

	if (inSelection)
	{
	    wAttrib.opacity    = OPAQUE * gs->optionGetSelectOpacity () / 100;
	    wAttrib.saturation = COLOR * gs->optionGetSelectSaturation () / 100;
	    wAttrib.brightness = BRIGHT * gs->optionGetSelectBrightness () / 100;
	}
#if 0
	if (doTabbing)
	{
	    /* fade the window out */
	    float progress;
	    int   distanceX, distanceY;
	    float origDistance, distance;

	    if (gw->animateState & FINISHED_ANIMATION)
	    {
		drawnPosX = gw->destination.x;
		drawnPosY = gw->destination.y;
	    }
	    else
	    {
		drawnPosX = gw->orgPos.x + gw->tx;
		drawnPosY = gw->orgPos.y + gw->ty;
	    }

	    distanceX = drawnPosX - gw->destination.x;
	    distanceY = drawnPosY - gw->destination.y;
	    distance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    distanceX = (gw->orgPos.x - gw->destination.x);
	    distanceY = (gw->orgPos.y - gw->destination.y);
	    origDistance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    if (!distanceX && !distanceY)
		progress = 1.0f;
	    else
		progress = 1.0f - (distance / origDistance);

	    animProgress = progress;

	    progress = MAX (progress, 0.0f);
	    if (gw->group->tabbingState == Tabbing)
		progress = 1.0f - progress;

	    wAttrib.opacity = (float)wAttrib.opacity * progress;
	}

	if (doRotate)
	{
	    float timeLeft = gw->group->changeAnimationTime;
	    int   animTime = groupGetChangeAnimationTime (s) * 500;

	    if (gw->group->changeState == TabChangeOldOut)
		timeLeft += animTime;

	    /* 0 at the beginning, 1 at the end */
	    animProgress = 1 - (timeLeft / (2 * animTime));
	}

	if (gw->resizeGeometry)
	{
	    int    xOrigin, yOrigin;
	    float  xScale, yScale;
	    BoxRec box;

	    groupGetStretchRectangle (w, &box, &xScale, &yScale);

	    xOrigin = w->attrib.x - w->input.left;
	    yOrigin = w->attrib.y - w->input.top;

	    matrixTranslate (&wTransform, xOrigin, yOrigin, 0.0f);
	    matrixScale (&wTransform, xScale, yScale, 1.0f);
	    matrixTranslate (&wTransform,
			     (gw->resizeGeometry->x - w->attrib.x) /
			     xScale - xOrigin,
			     (gw->resizeGeometry->y - w->attrib.y) /
			     yScale - yOrigin,
			     0.0f);

	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	}
	else if (doRotate || doTabbing)
	{
	    float      animWidth, animHeight;
	    float      animScaleX, animScaleY;
	    CompWindow *morphBase, *morphTarget;

	    if (doTabbing)
	    {
		if (gw->group->tabbingState == Tabbing)
		{
		    morphBase   = w;
		    morphTarget = TOP_TAB (gw->group);
		}
		else
		{
		    morphTarget = w;
		    if (HAS_TOP_WIN (gw->group))
			morphBase = TOP_TAB (gw->group);
		    else
			morphBase = gw->group->lastTopTab;
		}
	    }
	    else
	    {
		morphBase   = PREV_TOP_TAB (gw->group);
		morphTarget = TOP_TAB (gw->group);
	    }

	    animWidth = (1 - animProgress) * WIN_REAL_WIDTH (morphBase) +
		        animProgress * WIN_REAL_WIDTH (morphTarget);
	    animHeight = (1 - animProgress) * WIN_REAL_HEIGHT (morphBase) +
		         animProgress * WIN_REAL_HEIGHT (morphTarget);

	    animWidth = MAX (1.0f, animWidth);
	    animHeight = MAX (1.0f, animHeight);
	    animScaleX = animWidth / WIN_REAL_WIDTH (w);
	    animScaleY = animHeight / WIN_REAL_HEIGHT (w);

	    if (doRotate)
		matrixScale (&wTransform, 1.0f, 1.0f, 1.0f / s->width);

	    matrixTranslate (&wTransform,
			     WIN_REAL_X (w) + WIN_REAL_WIDTH (w) / 2.0f,
			     WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) / 2.0f,
			     0.0f);

	    if (doRotate)
	    {
		float rotateAngle = animProgress * 180.0f;
		if (IS_TOP_TAB (w, gw->group))
		    rotateAngle += 180.0f;

		if (gw->group->changeAnimationDirection < 0)
		    rotateAngle *= -1.0f;

		matrixRotate (&wTransform, rotateAngle, 0.0f, 1.0f, 0.0f);
	    }

	    if (doTabbing)
		matrixTranslate (&wTransform,
				 drawnPosX - WIN_X (w),
				 drawnPosY - WIN_Y (w), 0.0f);

	    matrixScale (&wTransform, animScaleX, animScaleY, 1.0f);

	    matrixTranslate (&wTransform,
			     -(WIN_REAL_X (w) + WIN_REAL_WIDTH (w) / 2.0f),
			     -(WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) / 2.0f),
			     0.0f);

	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	}
#endif

	status = gWindow->glPaint (wAttrib, wTransform, region, mask);

#if 0
	if (showTabbar)
	    groupPaintTabBar (gw->group, &wAttrib, &wTransform, mask, region);
#endif
    }
    else
	status = gWindow->glPaint (attrib, transform, region, mask);

    return status;
}


/*
 * GroupWindow::is ()
 *
 */

bool
GroupWindow::is ()
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
 * GroupWindow::inRegion ()
 *
 */

bool
GroupWindow::inRegion (CompRegion reg,
		       float      precision)
{
    CompRegion buf;
    int    area = 0;

    buf = reg.intersected (window->region ());

    /* buf area */
    area = buf.boundingRect ().width () * buf.boundingRect ().height ();

    if (area >= WIN_WIDTH (window) * WIN_HEIGHT (window) * precision)
    {
	return true;
    }

    return false;
}

void
GroupWindow::select ()
{
    GROUP_SCREEN (screen);

    if (!inSelection)
    {
	gs->masterSelection.push_back (window);
	selection = &gs->masterSelection;
    }
    else
    {
	selection = NULL;
	gs->masterSelection.remove (window);
    }
    inSelection = !inSelection;
}
