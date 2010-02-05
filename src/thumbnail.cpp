/* copyright */

#include "thumbnail.h"

COMPIZ_PLUGIN_20090315 (thumbnail, ThumbPluginVTable);

void
ThumbScreen::freeThumbText (Thumbnail  *t)
{
    if (!t->text)
	return;

    delete t->text;
    t->text = NULL;
}

void
ThumbScreen::renderThumbText (Thumbnail  *t,
		 	      bool       freeThumb)
{
    CompText::Attrib tA;

    if (freeThumb || !t->text)
    {
	freeThumbText (t);
	t->text = new CompText ();
    }

    if (!textPluginLoaded)
	return;

    tA.maxWidth   = t->width;
    tA.maxHeight  = 100;

    tA.size       = optionGetFontSize ();
    tA.color[0]   = optionGetFontColorRed ();
    tA.color[1]   = optionGetFontColorGreen ();
    tA.color[2]   = optionGetFontColorBlue ();
    tA.color[3]   = optionGetFontColorAlpha ();
    tA.flags      = CompText::Ellipsized;
    if (optionGetFontBold ())
	tA.flags |= CompText::StyleBold;
    tA.family     = "Sans";

    t->textValid = t->text->renderWindowTitle (t->win->id (), false, tA);
}

void
ThumbScreen::damageThumbRegion (Thumbnail  *t)
{
    int x = t->x - t->offset;
    int y = t->y - t->offset;
    int width = t->width + (t->offset * 2);
    int height = t->height + (t->offset * 2);
    CompRect   rect (x, y, width, height);

    if (t->text)
	rect.setHeight (t->text->getHeight () + TEXT_DISTANCE);

    CompRegion region (rect);

    cScreen->damageRegion (region);
}

#define GET_DISTANCE(a,b) \
    (sqrt((((a)[0] - (b)[0]) * ((a)[0] - (b)[0])) + \
	  (((a)[1] - (b)[1]) * ((a)[1] - (b)[1]))))

void
ThumbScreen::thumbUpdateThumbnail ()
{
    int        igMidPoint[2], tMidPoint[2];
    int        tPos[2], tmpPos[2];
    float      distance = 1000000;
    int        off, oDev, tHeight;
    int        ox1, oy1, ox2, oy2, ow, oh;
    float      maxSize = optionGetThumbSize ();
    double     scale  = 1.0;
    CompWindow *w;

    if (thumb.win == pointedWin)
	return;

    if (thumb.opacity > 0.0 && oldThumb.opacity > 0.0)
	return;

    if (thumb.win)
	damageThumbRegion (&thumb);

    freeThumbText (&oldThumb);

    oldThumb       = thumb;
    thumb.text     = NULL;
    thumb.win      = pointedWin;
    thumb.dock     = dock;

    if (!thumb.win || !dock)
    {
	thumb.win  = NULL;
        thumb.dock = NULL;
	return;
    }

    w = thumb.win;

    /* do we nee to scale the window down? */
    if (WIN_W (w) > maxSize || WIN_H (w) > maxSize)
    {
	if (WIN_W (w) >= WIN_H (w))
	    scale = maxSize / WIN_W (w);
	else
	    scale = maxSize / WIN_H (w);
    }

    thumb.width  = WIN_W (w)* scale;
    thumb.height = WIN_H (w) * scale;
    thumb.scale  = scale;

    if (optionGetTitleEnabled ())
	renderThumbText (&thumb, false);
    else
	freeThumbText (&thumb);

    igMidPoint[0] = w->iconGeometry ().x () + (w->iconGeometry ().width () / 2);
    igMidPoint[1] = w->iconGeometry ().y () + (w->iconGeometry ().height () / 2);

    off = optionGetBorder ();
    oDev = screen->outputDeviceForPoint (w->iconGeometry ().x () +
				 (w->iconGeometry ().width () / 2),
				 w->iconGeometry ().y () +
				 (w->iconGeometry ().height () / 2));

    if (screen->outputDevs ().size () == 1 ||
        (unsigned int) oDev > screen->outputDevs ().size ())
    {
	ox1 = 0;
	oy1 = 0;
	ox2 = screen->width ();
	oy2 = screen->height ();
	ow  = screen->width ();
	oh  = screen->height ();
    }
    else
    {
	ox1 = screen->outputDevs ()[oDev].x1 ();
	ox2 = screen->outputDevs ()[oDev].x2 ();
	oy1 = screen->outputDevs ()[oDev].y1 ();
	oy2 = screen->outputDevs ()[oDev].y2 ();
	ow  = ox2 - ox1;
	oh  = oy2 - oy1;
    }

    tHeight = thumb.height;
    if (thumb.text)
	tHeight += thumb.text->getHeight () + TEXT_DISTANCE;

    /* Could someone please explain how this works */

    // failsave position
    tPos[0] = igMidPoint[0] - (thumb.width / 2.0);

    if (w->iconGeometry ().y () - tHeight >= 0)
	tPos[1] = w->iconGeometry ().y () - tHeight;
    else
	tPos[1] = w->iconGeometry ().y () + w->iconGeometry ().height ();

    // above
    tmpPos[0] = igMidPoint[0] - (thumb.width / 2.0);

    if (tmpPos[0] - off < ox1)
	tmpPos[0] = ox1 + off;

    if (tmpPos[0] + off + thumb.width > ox2)
    {
	if (thumb.width + (2 * off) <= ow)
	    tmpPos[0] = ox2 - thumb.width - off;
	else
	    tmpPos[0] = ox1 + off;
    }

    tMidPoint[0] = tmpPos[0] + (thumb.width / 2.0);

    tmpPos[1] = WIN_Y (dock) - tHeight - off;
    tMidPoint[1] = tmpPos[1] + (tHeight / 2.0);

    if (tmpPos[1] > oy1)
    {
	tPos[0]  = tmpPos[0];
	tPos[1]  = tmpPos[1];
	distance = GET_DISTANCE (igMidPoint, tMidPoint);
    }

    // below
    tmpPos[1] = WIN_Y (dock) + WIN_H (dock) + off;

    tMidPoint[1] = tmpPos[1] + (tHeight / 2.0);

    if (tmpPos[1] + tHeight + off < oy2 &&
	GET_DISTANCE (igMidPoint, tMidPoint) < distance)
    {
	tPos[0]  = tmpPos[0];
	tPos[1]  = tmpPos[1];
	distance = GET_DISTANCE (igMidPoint, tMidPoint);
    }

    // left
    tmpPos[1] = igMidPoint[1] - (tHeight / 2.0);

    if (tmpPos[1] - off < oy1)
	tmpPos[1] = oy1 + off;

    if (tmpPos[1] + off + tHeight > oy2)
    {
	if (tHeight + (2 * off) <= oh)
	    tmpPos[1] = oy2 - thumb.height - off;
	else
	    tmpPos[1] = oy1 + off;
    }

    tMidPoint[1] = tmpPos[1] + (tHeight / 2.0);

    tmpPos[0] = WIN_X (dock) - thumb.width - off;
    tMidPoint[0] = tmpPos[0] + (thumb.width / 2.0);

    if (tmpPos[0] > ox1 && GET_DISTANCE (igMidPoint, tMidPoint) < distance)
    {
	tPos[0]  = tmpPos[0];
	tPos[1]  = tmpPos[1];
	distance = GET_DISTANCE (igMidPoint, tMidPoint);
    }

    // right
    tmpPos[0] = WIN_X (dock) + WIN_W (dock) + off;

    tMidPoint[0] = tmpPos[0] + (thumb.width / 2.0);

    if (tmpPos[0] + thumb.width + off < ox2 &&
	GET_DISTANCE (igMidPoint, tMidPoint) < distance)
    {
	tPos[0]  = tmpPos[0];
	tPos[1]  = tmpPos[1];
	distance = GET_DISTANCE (igMidPoint, tMidPoint);
    }

    thumb.x       = tPos[0];
    thumb.y       = tPos[1];
    thumb.offset  = off;
    thumb.opacity = 0.0;

    damageThumbRegion (&thumb);
}

bool
ThumbScreen::thumbShowThumbnail ()
{
    showingThumb   = true;

    thumbUpdateThumbnail ();
    damageThumbRegion (&thumb);

    return false;
}

bool
ThumbScreen::checkPosition (CompWindow *w)
{
    if (optionGetCurrentViewport ())
    {
	if (w->serverX () >= screen->width ()    ||
	    w->serverX () + w->serverWidth () <= 0  ||
	    w->serverY () >= screen->height ()   ||
	    w->serverY () + w->serverHeight () <= 0)
	{
	    return false;
	}
    }

    return true;
}

void
ThumbScreen::positionUpdate (const CompPoint &p)
{
    CompWindow *found = NULL;

    foreach (CompWindow *cw, screen->windows ())
    {
	THUMB_WINDOW (cw);

	if (cw->iconGeometry ().isEmpty ())
	    continue;

	if (!cw->isMapped ())
	    continue;
	
	if (cw->state () & CompWindowStateSkipTaskbarMask)
	    continue;

	if (cw->state () & CompWindowStateSkipPagerMask)
	    continue;

	if (!cw->managed ())
	    continue;

	if (!tw->cWindow->pixmap ())
	    continue;

	if (cw->iconGeometry ().contains (p) &&
	    checkPosition (cw))
	{
	    found = cw;
	    break;
	}
    }

    if (found)
    {
	if (!showingThumb &&
	    !(thumb.opacity != 0.0 && thumb.win == found))
	{
	    if (displayTimeout.active ())

	    {
		if (pointedWin != found)
		{
		    displayTimeout.stop ();
		    displayTimeout.start (boost::bind
					   (&ThumbScreen::thumbShowThumbnail,
					    this),
					  optionGetShowDelay (),
					  optionGetShowDelay () + 500);
		}
	    }
	    else
	    {
	    displayTimeout.stop ();
	    displayTimeout.start (boost::bind (&ThumbScreen::thumbShowThumbnail,
						this),
				  optionGetShowDelay (),
				  optionGetShowDelay () + 500);
	    }
        }

        pointedWin = found;
        thumbUpdateThumbnail ();
    }
    else
    {
	if (displayTimeout.active ())
	{
	    displayTimeout.stop ();
	}

	pointedWin   = NULL;
	showingThumb = false;
    }
}


void
ThumbWindow::resize (int        dx,
		     int        dy,
		     int        dwidth,
		     int        dheight)
{
    THUMB_SCREEN (screen);

    ts->thumbUpdateThumbnail ();

    window->resize (dx, dy, dwidth, dheight);
}

void
ThumbScreen::handleEvent (XEvent * event)
{

    screen->handleEvent (event);

    CompWindow *w;

    switch (event->type)
    {
    case PropertyNotify:
	if (event->xproperty.atom == Atoms::wmName)
	{
	    w = screen->findWindow (event->xproperty.window);

	    if (w)
	    {
		if (thumb.win == w && optionGetTitleEnabled ())
		    renderThumbText (&thumb, true);
	    }
	}
	break;

    case ButtonPress:
	{

	    if (displayTimeout.active ())
	    {
		displayTimeout.stop ();
	    }

	    pointedWin   = 0;
	    showingThumb = false;
	}
	break;

    case EnterNotify:
	w = screen->findWindow (event->xcrossing.window);
	if (w)
	{
	    if (w->wmType () & CompWindowTypeDockMask)
	    {
		if (dock != w)
		{
		    dock = w;

		    if (displayTimeout.active ())
		    {
			displayTimeout.stop ();
		    }

		    pointedWin   = NULL;
		    showingThumb = false;
		}

		if (!poller.active ())
		{
		    poller.start ();
		}
	    }
	    else
	    {
		dock = NULL;

		if (displayTimeout.active ())
		{
		    displayTimeout.stop ();
		}

		pointedWin   = NULL;
		showingThumb = false;

		if (poller.active ())
		{
		    poller.stop ();
		}
	    }
	}
	break;
    case LeaveNotify:
	w = screen->findWindow (event->xcrossing.window);
	if (w)
	{

	    if (w->wmType () & CompWindowTypeDockMask)
	    {
		dock = NULL;

		if (displayTimeout.active ())
		{
		    displayTimeout.stop ();
		}

		pointedWin   = NULL;
		showingThumb = false;

		if (poller.active ())
		{
		    poller.stop ();
		}
	    }
	}
	break;

    default:
	break;
    }
}


void
ThumbScreen::paintTexture (int wx,
			  int wy,
			  int width,
			  int height,
			  int off)
{
    glBegin (GL_QUADS);

    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glVertex2f (wx, wy + height);
    glVertex2f (wx + width, wy + height);
    glVertex2f (wx + width, wy);

    glTexCoord2f (0, 1);
    glVertex2f (wx - off, wy - off);
    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy - off);

    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy - off);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy);
    glTexCoord2f (0, 1);
    glVertex2f (wx + width + off, wy - off);

    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy + height);
    glTexCoord2f (0, 1);
    glVertex2f (wx - off, wy + height + off);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy + height + off);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);

    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy + height + off);
    glTexCoord2f (0, 1);
    glVertex2f (wx + width + off, wy + height + off);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy + height);

    glTexCoord2f (1, 1);
    glVertex2f (wx, wy - off);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy - off);

    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy + height + off);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy + height + off);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);

    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy);
    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy + height);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);

    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy + height);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy);

    glEnd ();
}

void
ThumbScreen::thumbPaintThumb (Thumbnail           *t,
		 	      const GLMatrix *transform)
{
    int			  addWindowGeometryIndex;
    CompWindow            *w = t->win;
    int                   wx = t->x;
    int                   wy = t->y;
    float                 width  = t->width;
    float                 height = t->height;
    GLWindowPaintAttrib     sAttrib;
    unsigned int          mask = PAINT_WINDOW_TRANSFORMED_MASK |
	                         PAINT_WINDOW_TRANSLUCENT_MASK;
    GLWindow		  *gWindow = GLWindow::get (w);

    if (!w)
	return;

    sAttrib = gWindow->paintAttrib ();

    if (t->text)
	height += t->text->getHeight () + TEXT_DISTANCE;

    /* Wrap drawWindowGeometry to make sure the general
       drawWindowGeometry function is used */
    addWindowGeometryIndex =
	    gWindow->glAddGeometryGetCurrentIndex ();

    if (!gWindow->textures ().empty ())
    {
	int            off = t->offset;
	GLenum         filter = gScreen->textureFilter ();
	GLFragment::Attrib fragment (sAttrib);
	GLMatrix       wTransform = *transform;

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	
	if (optionGetWindowLike ())
	{
	    glColor4f (1.0, 1.0, 1.0, t->opacity);
	    foreach (GLTexture *tex, windowTexture)
	    {
		tex->enable (GLTexture::Good);
		paintTexture (wx, wy, width, height, off);
		tex->disable ();
	    }
	}
	else
	{
	    glColor4us (optionGetThumbColorRed (),
			optionGetThumbColorGreen (),
			optionGetThumbColorBlue (),
			optionGetThumbColorAlpha () * t->opacity);

	    foreach (GLTexture *tex, glowTexture)
	    {
		tex->enable (GLTexture::Good);
		paintTexture (wx, wy, width, height, off);
		tex->disable ();
	    }
	}

	glColor4usv (defaultColor);

	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	if (t->text)
	{
	    float ox = 0.0;

	    if (t->text->getWidth () < width)
		ox = (width - t->text->getWidth ()) / 2.0;

	    t->text->draw (wx + ox, wy + height, t->opacity);
	}

	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glDisable (GL_BLEND);

	gScreen->setTexEnvMode (GL_REPLACE);

	glColor4usv (defaultColor);

	sAttrib.opacity *= t->opacity;
	sAttrib.yScale = t->scale;
	sAttrib.xScale = t->scale;

	sAttrib.xTranslate = wx - w->x () + w->input ().left * sAttrib.xScale;
	sAttrib.yTranslate = wy - w->y () + w->input ().top * sAttrib.yScale;

	if (optionGetMipmap ())
	    gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

	wTransform.translate (w->x (), w->y (), 0.0f);
	wTransform.scale (sAttrib.xScale, sAttrib.yScale, 1.0f);
	wTransform.translate (sAttrib.xTranslate / sAttrib.xScale - w->x (),
			      sAttrib.yTranslate / sAttrib.yScale - w->y (),
			      0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());
	/* XXX: replacing the addWindowGeometry function like this is
	   very ugly but necessary until the vertex stage has been made
	   fully pluggable. */
	gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);
	gWindow->glDraw (wTransform, fragment, infiniteRegion, mask);
	glPopMatrix ();

	gScreen->setTextureFilter (filter);
    }

    gWindow->glAddGeometrySetCurrentIndex (addWindowGeometryIndex);
}

/* From here onwards */

void
ThumbScreen::preparePaint (int ms)
{
    float val = ms;

    val /= 1000;
    val /= optionGetFadeSpeed ();

    /*if (screen->otherGrabExist ("")) // shouldn't there be a s->grabs.empty () or something?
    {
	dock = NULL;

	if (displayTimeout.active ())
	{
	    displayTimeout.stop ();
	}

	pointedWin   = 0;
	showingThumb = false;
    }*/

    if (showingThumb && thumb.win == pointedWin)
    {
	thumb.opacity = MIN (1.0, thumb.opacity + val);
    }

    if (!showingThumb || thumb.win != pointedWin)
    {
	thumb.opacity = MAX (0.0, thumb.opacity - val);
	if (thumb.opacity == 0.0)
	    thumb.win = NULL;
    }

    if (oldThumb.opacity > 0.0f)
    {
	oldThumb.opacity = MAX (0.0, oldThumb.opacity - val);
	if (oldThumb.opacity == 0.0)
	{
	    damageThumbRegion (&oldThumb);
	    freeThumbText (&oldThumb);
	    oldThumb.win = NULL;
	}
    }

    cScreen->preparePaint (ms);
}

void
ThumbScreen::donePaint ()
{
    if (thumb.opacity > 0.0 && thumb.opacity < 1.0)
	damageThumbRegion (&thumb);

    if (oldThumb.opacity > 0.0 && oldThumb.opacity < 1.0)
	damageThumbRegion (&oldThumb);

    cScreen->donePaint ();
}

bool
ThumbScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
		       	    const GLMatrix &transform,
		       	    const CompRegion &region,
		       	    CompOutput *output,
		       	    unsigned int mask)
{
    bool         status;
    unsigned int newMask = mask;

    painted = false;

    x = screen->vp ().x ();
    y = screen->vp ().y ();

    if ((oldThumb.opacity > 0.0 && oldThumb.win) ||
       	(thumb.opacity > 0.0 && thumb.win))
    {
	newMask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (optionGetAlwaysOnTop () && !painted)
    {
	if (oldThumb.opacity > 0.0 && oldThumb.win)
	{
	    GLMatrix sTransform = transform;

	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
	    glPushMatrix ();
	    glLoadMatrixf (sTransform.getMatrix ());
	    thumbPaintThumb (&oldThumb, &sTransform);
	    glPopMatrix ();
	}

	if (thumb.opacity > 0.0 && thumb.win)
	{
	    GLMatrix sTransform = transform;

	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
	    glPushMatrix ();
	    glLoadMatrixf (sTransform.getMatrix ());
	    thumbPaintThumb (&thumb, &sTransform);
	    glPopMatrix ();
	}
    }

    return status;
}

void
ThumbScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
			               const GLMatrix &transform,
			               const CompRegion &region,
			               CompOutput *output,
				       unsigned int mask)
{

    gScreen->glPaintTransformedOutput (attrib, transform, region, output, mask);

    if (optionGetAlwaysOnTop () && x == screen->vp ().x () &&
				   y == screen->vp ().y ())
    {
	painted = true;

	if (oldThumb.opacity > 0.0 && oldThumb.win)
	{
	    GLMatrix sTransform = transform;

	    gScreen->glApplyTransform (attrib, output, &sTransform);
	    sTransform.toScreenSpace(output, -attrib.zTranslate);
	    glPushMatrix ();
	    glLoadMatrixf (sTransform.getMatrix ());
	    thumbPaintThumb (&oldThumb, &sTransform);
	    glPopMatrix ();
	}

	if (thumb.opacity > 0.0 && thumb.win)
	{
	    GLMatrix sTransform = transform;

	    gScreen->glApplyTransform (attrib, output, &sTransform);
	    sTransform.toScreenSpace(output, -attrib.zTranslate);
	    glPushMatrix ();
	    glLoadMatrixf (sTransform.getMatrix ());
	    thumbPaintThumb (&thumb, &sTransform);
	    glPopMatrix ();
	}
    }
}

bool
ThumbWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      CompRegion 		&region,
		      unsigned int		mask)
{
    bool status;

    THUMB_SCREEN (screen);

    status = gWindow->glPaint (attrib, transform, region, mask);

    if (!ts->optionGetAlwaysOnTop () && ts->x == screen->vp ().x () &&
				    ts->y == screen->vp ().y ())
    {
	GLMatrix sTransform = transform;
	if (ts->oldThumb.opacity > 0.0 && ts->oldThumb.win &&
	    ts->oldThumb.dock == window)
	{
	    ts->thumbPaintThumb (&ts->oldThumb, &sTransform);
	}

	if (ts->thumb.opacity > 0.0 && ts->thumb.win && ts->thumb.dock == window)
	{
	    ts->thumbPaintThumb (&ts->thumb, &sTransform);
	}
    }

    return status;
}

bool
ThumbWindow::damageRect (bool initial,
		         const CompRect &rect)
{
    THUMB_SCREEN (screen);

    if (ts->thumb.win == window && ts->thumb.opacity > 0.0)
	ts->damageThumbRegion (&ts->thumb);

    if (ts->oldThumb.win == window && ts->oldThumb.opacity > 0.0)
	ts->damageThumbRegion (&ts->oldThumb);

    return cWindow->damageRect (initial, rect);
}


ThumbScreen::ThumbScreen (CompScreen *screen) :
    PluginClassHandler <ThumbScreen, CompScreen> (screen),
    gScreen (GLScreen::get (screen)),
    cScreen (CompositeScreen::get (screen)),
    dock (NULL),
    pointedWin (NULL),
    showingThumb (false),
    painted (false),
    glowTexture (GLTexture::imageDataToTexture
		 (glowTex, CompSize (32, 32), GL_RGBA, GL_UNSIGNED_BYTE)),
    windowTexture (GLTexture::imageDataToTexture
		   (windowTex, CompSize (32, 32), GL_RGBA, GL_UNSIGNED_BYTE)),
    x (0),
    y (0)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    thumb.win = NULL;
    oldThumb.win = NULL;

    thumb.text = NULL;
    oldThumb.text = NULL;

    poller.setCallback (boost::bind (&ThumbScreen::positionUpdate, this, _1));
}

ThumbScreen::~ThumbScreen ()
{
    poller.stop ();
    displayTimeout.stop ();

    freeThumbText (&thumb);
    freeThumbText (&oldThumb);
}

ThumbWindow::ThumbWindow (CompWindow *window) :
    PluginClassHandler <ThumbWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window))
{
}
    

ThumbWindow::~ThumbWindow ()
{
    THUMB_SCREEN (screen);

    if (ts->thumb.win == window)
    {
	ts->damageThumbRegion (&ts->thumb);
	ts->thumb.win = NULL;
	ts->thumb.opacity = 0;
    }

    if (ts->oldThumb.win == window)
    {
	ts->damageThumbRegion (&ts->oldThumb);
	ts->oldThumb.win = NULL;
	ts->oldThumb.opacity = 0;
    }
}


bool
ThumbPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;
    if (!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;
    if (CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
	textPluginLoaded = true;
    else
	textPluginLoaded = false;

    return true;
}
