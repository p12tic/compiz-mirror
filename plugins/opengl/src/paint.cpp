/*
 * Copyright © 2005 Novell, Inc.
 * Copyright © 2011 Linaro, Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: David Reveman <davidr@novell.com>
 *          Travis Watkins <travis.watkins@linaro.org>
 */

#include "privates.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <boost/foreach.hpp>
#include <boost/scoped_array.hpp>
#define foreach BOOST_FOREACH

#include <opengl/opengl.h>

#include "privates.h"

#define DEG2RAD (M_PI / 180.0f)

GLScreenPaintAttrib defaultScreenPaintAttrib = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -DEFAULT_Z_CAMERA
};

GLWindowPaintAttrib GLWindow::defaultPaintAttrib = {
    OPAQUE, BRIGHT, COLOR, 1.0f, 1.0f, 0.0f, 0.0f
};

void
GLScreen::glApplyTransform (const GLScreenPaintAttrib &sAttrib,
			    CompOutput                *output,
			    GLMatrix                  *transform)
{
    WRAPABLE_HND_FUNCTN (glApplyTransform, sAttrib, output, transform)

    transform->translate (sAttrib.xTranslate,
			  sAttrib.yTranslate,
			  sAttrib.zTranslate + sAttrib.zCamera);
    transform->rotate (sAttrib.xRotate, 0.0f, 1.0f, 0.0f);
    transform->rotate (sAttrib.vRotate,
		       cosf (sAttrib.xRotate * DEG2RAD),
		       0.0f,
		       sinf (sAttrib.xRotate * DEG2RAD));
    transform->rotate (sAttrib.yRotate, 0.0f, 1.0f, 0.0f);
}

void
PrivateGLScreen::paintBackground (const GLMatrix   &transform,
                                  const CompRegion &region,
				  bool             transformed)
{
    GLVertexBuffer *streamingBuffer = GLVertexBuffer::streamingBuffer ();
    GLfloat         vertexData[18];
    GLushort        colorData[4];

    BoxPtr    pBox = const_cast <Region> (region.handle ())->rects;
    int	      n, nBox = const_cast <Region> (region.handle ())->numRects;

    if (!nBox)
	return;

    if (screen->desktopWindowCount ())
    {
	if (!backgroundTextures.empty ())
	{
	    backgroundTextures.clear ();
	}

	backgroundLoaded = false;

	return;
    }
    else
    {
	if (!backgroundLoaded)
	    updateScreenBackground ();

	backgroundLoaded = true;
    }

    if (backgroundTextures.empty ())
    {
	streamingBuffer->begin (GL_TRIANGLES);
	n = nBox;

	while (n--)
	{
	    vertexData[0]  = pBox->x1;
	    vertexData[1]  = pBox->y1;
	    vertexData[2]  = 0.0f;
	    vertexData[3]  = pBox->x1;
	    vertexData[4]  = pBox->y2;
	    vertexData[5]  = 0.0f;
	    vertexData[6]  = pBox->x2;
	    vertexData[7]  = pBox->y1;
	    vertexData[8]  = 0.0f;
	    vertexData[9]  = pBox->x1;
	    vertexData[10] = pBox->y2;
	    vertexData[11] = 0.0f;
	    vertexData[12] = pBox->x2;
	    vertexData[13] = pBox->y2;
	    vertexData[14] = 0.0f;

	    vertexData[15] = pBox->x2;
	    vertexData[16] = pBox->y1;
	    vertexData[17] = 0.0f;

	    streamingBuffer->addVertices (6, vertexData);

	    pBox++;
	}

	colorData[0] = colorData[1] = colorData[2] = 0;
	colorData[3] = std::numeric_limits <unsigned short>::max ();
	streamingBuffer->addColors (1, colorData);

	streamingBuffer->end ();
	streamingBuffer->render (transform);
    }
    else
    {
	n = nBox;

	for (unsigned int i = 0; i < backgroundTextures.size (); i++)
	{
	    GLfloat textureData[12];
	    GLTexture *bg = backgroundTextures[i];
	    CompRegion r = region & *bg;

	    pBox = const_cast <Region> (r.handle ())->rects;
	    nBox = const_cast <Region> (r.handle ())->numRects;
	    n = nBox;

	    streamingBuffer->begin (GL_TRIANGLES);

	    while (n--)
	    {
		GLfloat tx1 = COMP_TEX_COORD_X (bg->matrix (), pBox->x1);
		GLfloat tx2 = COMP_TEX_COORD_X (bg->matrix (), pBox->x2);
		GLfloat ty1 = COMP_TEX_COORD_Y (bg->matrix (), pBox->y1);
		GLfloat ty2 = COMP_TEX_COORD_Y (bg->matrix (), pBox->y2);

		vertexData[0]  = pBox->x1;
		vertexData[1]  = pBox->y1;
		vertexData[2]  = 0.0f;
		vertexData[3]  = pBox->x1;
		vertexData[4]  = pBox->y2;
		vertexData[5]  = 0.0f;
		vertexData[6]  = pBox->x2;
		vertexData[7]  = pBox->y1;
		vertexData[8]  = 0.0f;
		vertexData[9]  = pBox->x1;
		vertexData[10] = pBox->y2;
		vertexData[11] = 0.0f;
		vertexData[12] = pBox->x2;
		vertexData[13] = pBox->y2;
		vertexData[14] = 0.0f;

		vertexData[15] = pBox->x2;
		vertexData[16] = pBox->y1;
		vertexData[17] = 0.0f;

		textureData[0]  = tx1;
		textureData[1]  = ty1;

		textureData[2]  = tx1;
		textureData[3]  = ty2;

		textureData[4]  = tx2;
		textureData[5]  = ty1;

		textureData[6]  = tx1;
		textureData[7]  = ty2;

		textureData[8]  = tx2;
		textureData[9]  = ty2;

		textureData[10] = tx2;
		textureData[11] = ty1;

		streamingBuffer->addVertices (6, vertexData);
		streamingBuffer->addTexCoords (0, 6, textureData);

		pBox++;
	    }

	    streamingBuffer->end ();

	    if (bg->name ())
	    {
		if (transformed)
		    bg->enable (GLTexture::Good);
		else
		    bg->enable (GLTexture::Fast);

		streamingBuffer->render (transform);

		bg->disable ();
	    }
	}
    }
}


/* This function currently always performs occlusion detection to
   minimize paint regions. OpenGL precision requirements are no good
   enough to guarantee that the results from using occlusion detection
   is the same as without. It's likely not possible to see any
   difference with most hardware but occlusion detection in the
   transformed screen case should be made optional for those who do
   see a difference. */
void
PrivateGLScreen::paintOutputRegion (const GLMatrix   &transform,
				    const CompRegion &region,
				    CompOutput       *output,
				    unsigned int     mask)
{
    CompRegion    tmpRegion (region);
    CompWindow    *w;
    GLWindow      *gw;
    int		  count, windowMask, odMask;
    CompWindow	  *fullscreenWindow = NULL;
    bool          status, unredirectFS;
    bool          withOffset = false;
    GLMatrix      vTransform;
    CompPoint     offXY;

    CompWindowList                   pl;
    CompWindowList::reverse_iterator rit;

    unredirectFS = CompositeScreen::get (screen)->
	getOption ("unredirect_fullscreen_windows")->value ().b ();

    if (mask & PAINT_SCREEN_TRANSFORMED_MASK)
    {
	windowMask     = PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK;
	count	       = 1;
    }
    else
    {
	windowMask     = 0;
	count	       = 0;
    }

    /*
     * We need to COPY the PaintList for now because there seem to be some
     * odd cases where the master list might change during the below loops.
     * (LP: #958540)
     */
    pl = cScreen->getWindowPaintList ();

    if (!(mask & PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK))
    {
	/* detect occlusions */
	for (rit = pl.rbegin (); rit != pl.rend (); rit++)
	{
	    w = (*rit);
	    gw = GLWindow::get (w);

	    if (w->destroyed ())
		continue;

	    if (!w->shaded ())
	    {
		/* Non-damaged windows don't have valid pixmap
		 * contents and we aren't displaying them yet
		 * so don't factor them into occlusion detection */
		if (!gw->priv->cWindow->damaged ())
		{
		    gw->priv->clip = region;
		    continue;
		}
		if (!w->isViewable ())
		    continue;
	    }

	    /* copy region */
	    gw->priv->clip = tmpRegion;

	    odMask = PAINT_WINDOW_OCCLUSION_DETECTION_MASK;

	    if ((cScreen->windowPaintOffset ().x () != 0 ||
		 cScreen->windowPaintOffset ().y () != 0) &&
		!w->onAllViewports ())
	    {
		withOffset = true;

		offXY = w->getMovementForOffset (cScreen->windowPaintOffset ());

		vTransform = transform;
		vTransform.translate (offXY.x (), offXY.y (), 0);

		gw->priv->clip.translate (-offXY.x (), -offXY. y ());

		odMask |= PAINT_WINDOW_WITH_OFFSET_MASK;
		status = gw->glPaint (gw->paintAttrib (), vTransform,
				      tmpRegion, odMask);
	    }
	    else
	    {
		withOffset = false;
		status = gw->glPaint (gw->paintAttrib (), transform, tmpRegion,
				      odMask);
	    }

	    if (status)
	    {
		if (withOffset)
		{
		    tmpRegion -= w->region ().translated (offXY);
		}
		else
		    tmpRegion -= w->region ();

		/* unredirect top most fullscreen windows. */
		if (count == 0 && unredirectFS)
		{
		    if (w->region () == screen->region () &&
			tmpRegion.isEmpty ())
		    {
			fullscreenWindow = w;
		    }
		    else
		    {
			foreach (CompOutput &o, screen->outputDevs ())
			    if (w->region () == CompRegion (o))
				fullscreenWindow = w;
		    }
		}
	    }

	    count++;
	}
    }

    if (fullscreenWindow)
	CompositeWindow::get (fullscreenWindow)->unredirect ();

    if (!(mask & PAINT_SCREEN_NO_BACKGROUND_MASK))
	paintBackground (transform,
	                 tmpRegion,
	                 (mask & PAINT_SCREEN_TRANSFORMED_MASK));

    /* paint all windows from bottom to top */
    foreach (w, pl)
    {
	if (w->destroyed ())
	    continue;

	if (w == fullscreenWindow)
	    continue;

	if (!w->shaded ())
	{
	    if (!w->isViewable ())
		continue;
	}

	gw = GLWindow::get (w);

	const CompRegion &clip =
	    (!(mask & PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK)) ?
	    gw->clip () : region;

	if ((cScreen->windowPaintOffset ().x () != 0 ||
	     cScreen->windowPaintOffset ().y () != 0) &&
	    !w->onAllViewports ())
	{
	    offXY = w->getMovementForOffset (cScreen->windowPaintOffset ());

	    vTransform = transform;
	    vTransform.translate (offXY.x (), offXY.y (), 0);
	    gw->glPaint (gw->paintAttrib (), vTransform, clip,
		         windowMask | PAINT_WINDOW_WITH_OFFSET_MASK);
	}
	else
	{
	    gw->glPaint (gw->paintAttrib (), transform, clip, windowMask);
	}
    }
}

void
GLScreen::glEnableOutputClipping (const GLMatrix   &transform,
				  const CompRegion &region,
				  CompOutput       *output)
{
    WRAPABLE_HND_FUNCTN (glEnableOutputClipping, transform, region, output)

    #ifndef USE_GLES
    GLdouble h = screen->height ();

    GLdouble p1[2] = { static_cast<GLdouble> (region.handle ()->extents.x1),
                       static_cast<GLdouble> (h - region.handle ()->extents.y2) };
    GLdouble p2[2] = { static_cast<GLdouble> (region.handle ()->extents.x2),
                       static_cast<GLdouble> (h - region.handle ()->extents.y1) };

    GLdouble halfW = output->width () / 2.0;
    GLdouble halfH = output->height () / 2.0;

    GLdouble cx = output->x1 () + halfW;
    GLdouble cy = (h - output->y2 ()) + halfH;

    GLdouble top[4]    = { 0.0, halfH / (cy - p1[1]), 0.0, 1.0 };
    GLdouble bottom[4] = { 0.0, halfH / (cy - p2[1]), 0.0, 1.0 };
    GLdouble left[4]   = { halfW / (cx - p1[0]), 0.0, 0.0, 1.0 };
    GLdouble right[4]  = { halfW / (cx - p2[0]), 0.0, 0.0, 1.0 };

    glPushMatrix ();
    glLoadMatrixf (transform.getMatrix ());

    glClipPlane (GL_CLIP_PLANE0, top);
    glClipPlane (GL_CLIP_PLANE1, bottom);
    glClipPlane (GL_CLIP_PLANE2, left);
    glClipPlane (GL_CLIP_PLANE3, right);

    glEnable (GL_CLIP_PLANE0);
    glEnable (GL_CLIP_PLANE1);
    glEnable (GL_CLIP_PLANE2);
    glEnable (GL_CLIP_PLANE3);

    glPopMatrix ();
    #endif
}

void
GLScreen::glDisableOutputClipping ()
{
    WRAPABLE_HND_FUNCTN (glDisableOutputClipping)

    #ifndef USE_GLES
    glDisable (GL_CLIP_PLANE0);
    glDisable (GL_CLIP_PLANE1);
    glDisable (GL_CLIP_PLANE2);
    glDisable (GL_CLIP_PLANE3);
    #endif
}

void
GLScreen::glBufferStencil (const GLMatrix       &matrix,
			   GLVertexBuffer       &vertexBuffer,
			   CompOutput           *output)
{
    WRAPABLE_HND_FUNCTN (glBufferStencil, matrix, vertexBuffer, output);

    GLfloat x = output->x ();
    GLfloat y = screen->height () - output->y2 ();
    GLfloat x2 = output->x () + output->width ();
    GLfloat y2 = screen->height () - output->y2 () + output->height ();

    GLfloat vertices[] =
    {
	x, y, 0,
	x, y2, 0,
	x2, y, 0,
	x2, y2, 0
    };

    GLushort colorData[] = { 0xffff, 0xffff, 0xffff, 0xffff };

    vertexBuffer.begin (GL_TRIANGLE_STRIP);

    vertexBuffer.addVertices (4, vertices);
    vertexBuffer.addColors (1, colorData);

    vertexBuffer.end ();
}

#define CLIP_PLANE_MASK (PAINT_SCREEN_TRANSFORMED_MASK | \
			 PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK)

void
GLScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &sAttrib,
				    const GLMatrix            &transform,
				    const CompRegion          &region,
				    CompOutput                *output,
				    unsigned int              mask)
{
    WRAPABLE_HND_FUNCTN (glPaintTransformedOutput, sAttrib, transform,
		       region, output, mask)

    GLMatrix sTransform = transform;

    if (mask & PAINT_SCREEN_CLEAR_MASK)
	clearTargetOutput (GL_COLOR_BUFFER_BIT);

    setLighting (true);

    glApplyTransform (sAttrib, output, &sTransform);

    if ((mask & CLIP_PLANE_MASK) == CLIP_PLANE_MASK)
    {
	sTransform.toScreenSpace (output, -sAttrib.zTranslate);

	if (GL::stencilBuffer)
	{
	    GLboolean depthTestEnabled = glIsEnabled (GL_DEPTH_TEST);
#ifdef USE_GLES
	    GLboolean saveColorMask[4];
	    GLboolean saveDepthMask;
	    // WARNING: glGetBooleanv involves a roundtrip and is extremely
	    //          slow. Use only on ES where we have no glPushAttrib.
	    glGetBooleanv (GL_COLOR_WRITEMASK, saveColorMask);
	    glGetBooleanv (GL_DEPTH_WRITEMASK, &saveDepthMask);
#else
	    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

	    glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	    glDepthMask (GL_FALSE);

	    glClearStencil (0);
	    glClear (GL_STENCIL_BUFFER_BIT);
	    glEnable (GL_STENCIL_TEST);
	    glStencilFunc (GL_ALWAYS, 1, 1);
	    glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);

	    glDisable (GL_DEPTH_TEST);

	    GLVertexBuffer vb;

	    vb.setAutoProgram (priv->autoProgram);

	    glBufferStencil (sTransform, vb, output);

	    vb.render (sTransform);

#ifdef USE_GLES
	    glColorMask (saveColorMask[0], saveColorMask[1], saveColorMask[2], saveColorMask[3]);
	    glDepthMask (saveDepthMask);
#else
	    glPopAttrib ();
#endif

	    if (depthTestEnabled)
		glEnable (GL_DEPTH_TEST);

	    glStencilFunc (GL_EQUAL, 1, 1);
	    glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else
	    glEnableOutputClipping (sTransform, region, output);

	priv->paintOutputRegion (sTransform, region, output, mask);

	if (GL::stencilBuffer)
	    glDisable (GL_STENCIL_TEST);
	else
	glDisableOutputClipping ();
    }
    else
    {
	sTransform.toScreenSpace (output, -sAttrib.zTranslate);
	priv->paintOutputRegion (sTransform, region, output, mask);
    }
}

bool
GLScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			 const GLMatrix            &transform,
			 const CompRegion          &region,
			 CompOutput                *output,
			 unsigned int              mask)
{
    WRAPABLE_HND_FUNCTN_RETURN (bool, glPaintOutput, sAttrib, transform,
			      region, output, mask)

    GLMatrix sTransform = transform;

    if (mask & PAINT_SCREEN_REGION_MASK)
    {
	if (mask & PAINT_SCREEN_TRANSFORMED_MASK)
	{
	    if (mask & PAINT_SCREEN_FULL_MASK)
	    {
		glPaintTransformedOutput (sAttrib, sTransform,
					  CompRegion (*output), output, mask);

		return true;
	    }

	    return false;
	}

	setLighting (false);

	sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	priv->paintOutputRegion (sTransform, region, output, mask);

	return true;
    }
    else if (mask & PAINT_SCREEN_FULL_MASK)
    {
	glPaintTransformedOutput (sAttrib, sTransform, CompRegion (*output),
				  output, mask);

	return true;
    }
    else
    {
	return false;
    }
}

void
GLScreen::glPaintCompositedOutput (const CompRegion    &region,
				   GLFramebufferObject *fbo,
				   unsigned int         mask)
{
    WRAPABLE_HND_FUNCTN (glPaintCompositedOutput, region, fbo, mask)

    GLMatrix sTransform;
    std::vector<GLfloat> vertexData;
    std::vector<GLfloat> textureData;
    const GLTexture::Matrix & texmatrix = fbo->tex ()->matrix ();
    GLVertexBuffer *streamingBuffer = GLVertexBuffer::streamingBuffer ();

    streamingBuffer->begin (GL_TRIANGLES);

    if (mask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
    {
	GLfloat tx1 = COMP_TEX_COORD_X (texmatrix, 0.0f);
	GLfloat tx2 = COMP_TEX_COORD_X (texmatrix, screen->width ());
	GLfloat ty1 = 1.0 - COMP_TEX_COORD_Y (texmatrix, 0.0f);
	GLfloat ty2 = 1.0 - COMP_TEX_COORD_Y (texmatrix, screen->height ());

	vertexData = {
	    0.0f,                    0.0f,                     0.0f,
	    0.0f,                    (float)screen->height (), 0.0f,
	    (float)screen->width (), 0.0f,                     0.0f,

	    0.0f,                    (float)screen->height (), 0.0f,
	    (float)screen->width (), (float)screen->height (), 0.0f,
	    (float)screen->width (), 0.0f,                     0.0f,
	};

	textureData = {
	    tx1, ty1,
	    tx1, ty2,
	    tx2, ty1,
	    tx1, ty2,
	    tx2, ty2,
	    tx2, ty1,
	};

	streamingBuffer->addVertices (6, &vertexData[0]);
	streamingBuffer->addTexCoords (0, 6, &textureData[0]);
    }
    else
    {
	BoxPtr pBox = const_cast <Region> (region.handle ())->rects;
	int nBox = const_cast <Region> (region.handle ())->numRects;

	while (nBox--)
	{
	    GLfloat tx1 = COMP_TEX_COORD_X (texmatrix, pBox->x1);
	    GLfloat tx2 = COMP_TEX_COORD_X (texmatrix, pBox->x2);
	    GLfloat ty1 = 1.0 - COMP_TEX_COORD_Y (texmatrix, pBox->y1);
	    GLfloat ty2 = 1.0 - COMP_TEX_COORD_Y (texmatrix, pBox->y2);

	    vertexData = {
		(float)pBox->x1, (float)pBox->y1, 0.0f,
		(float)pBox->x1, (float)pBox->y2, 0.0f,
		(float)pBox->x2, (float)pBox->y1, 0.0f,

		(float)pBox->x1, (float)pBox->y2, 0.0f,
		(float)pBox->x2, (float)pBox->y2, 0.0f,
		(float)pBox->x2, (float)pBox->y1, 0.0f,
	    };

	    textureData = {
		tx1, ty1,
		tx1, ty2,
		tx2, ty1,
		tx1, ty2,
		tx2, ty2,
		tx2, ty1,
	    };

	    streamingBuffer->addVertices (6, &vertexData[0]);
	    streamingBuffer->addTexCoords (0, 6, &textureData[0]);
	    pBox++;
	}
    }

    streamingBuffer->end ();
    fbo->tex ()->enable (GLTexture::Fast);
    sTransform.toScreenSpace (&screen->fullscreenOutput (), -DEFAULT_Z_CAMERA);
    streamingBuffer->render (sTransform);
    fbo->tex ()->disable ();
}

static void
addSingleQuad (GLVertexBuffer *vertexBuffer,
	       const        GLTexture::MatrixList &matrix,
	       unsigned int nMatrix,
	       int          x1,
	       int          y1,
	       int          x2,
	       int          y2,
	       bool         rect)
{
    GLfloat vertexData[18] = {
	(float)x1, (float)y1, 0.0,
	(float)x1, (float)y2, 0.0,
	(float)x2, (float)y1, 0.0,
	(float)x2, (float)y1, 0.0,
	(float)x1, (float)y2, 0.0,
	(float)x2, (float)y2, 0.0
    };
    vertexBuffer->addVertices (6, vertexData);

    if (rect)
    {
	unsigned int it;
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x1);
	    data[1] = COMP_TEX_COORD_Y (mat, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x1);
	    data[1] = COMP_TEX_COORD_Y (mat, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x2);
	    data[1] = COMP_TEX_COORD_Y (mat, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x2);
	    data[1] = COMP_TEX_COORD_Y (mat, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x1);
	    data[1] = COMP_TEX_COORD_Y (mat, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_X (mat, x2);
	    data[1] = COMP_TEX_COORD_Y (mat, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
    }
    else
    {
	unsigned int it;
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x1, y1);
	    data[1] = COMP_TEX_COORD_YX (mat, x1, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x1, y2);
	    data[1] = COMP_TEX_COORD_YX (mat, x1, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x2, y1);
	    data[1] = COMP_TEX_COORD_YX (mat, x2, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x2, y1);
	    data[1] = COMP_TEX_COORD_YX (mat, x2, y1);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x1, y2);
	    data[1] = COMP_TEX_COORD_YX (mat, x1, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
	for (it = 0; it < nMatrix; it++)
	{
	    GLfloat data[2];
	    const GLTexture::Matrix &mat = matrix[it];
	    data[0] = COMP_TEX_COORD_XY (mat, x2, y2);
	    data[1] = COMP_TEX_COORD_YX (mat, x2, y2);
	    vertexBuffer->addTexCoords (it, 1, data);
	}
    }
}

static void
addQuads (GLVertexBuffer *vertexBuffer,
	  const        GLTexture::MatrixList &matrix,
	  unsigned int nMatrix,
	  int          x1,
	  int          y1,
	  int          x2,
	  int          y2,
	  bool         rect,
	  unsigned int maxGridWidth,
	  unsigned int maxGridHeight)
{
    int nQuadsX = (maxGridWidth == MAXSHORT) ? 1 :
	1 + (x2 - x1 - 1) / (int) maxGridWidth;  // ceil. division
    int nQuadsY = (maxGridHeight == MAXSHORT) ? 1 :
	1 + (y2 - y1 - 1) / (int) maxGridHeight;

    if (nQuadsX == 1 && nQuadsY == 1)
    {
	addSingleQuad (vertexBuffer, matrix, nMatrix, x1, y1, x2, y2, rect);
    }
    else
    {
	int quadWidth  = 1 + (x2 - x1 - 1) / nQuadsX;  // ceil. division
	int quadHeight = 1 + (y2 - y1 - 1) / nQuadsY;
	int nx1, ny1, nx2, ny2;

	for (ny1 = y1; ny1 < y2; ny1 = ny2)
	{
	    ny2 = MIN (ny1 + (int) quadHeight, y2);

	    for (nx1 = x1; nx1 < x2; nx1 = nx2)
	    {
		nx2 = MIN (nx1 + (int) quadWidth, x2);

		addSingleQuad (vertexBuffer, matrix, nMatrix,
		               nx1, ny1, nx2, ny2, rect);
	    }
	}
    }
}

void
GLWindow::glAddGeometry (const GLTexture::MatrixList &matrix,
			 const CompRegion            &region,
			 const CompRegion            &clip,
			 unsigned int                maxGridWidth,
			 unsigned int                maxGridHeight)
{
    WRAPABLE_HND_FUNCTN (glAddGeometry, matrix, region, clip)

    BoxRec full;
    int    nMatrix = matrix.size ();

    full = clip.handle ()->extents;
    if (region.handle ()->extents.x1 > full.x1)
	full.x1 = region.handle ()->extents.x1;
    if (region.handle ()->extents.y1 > full.y1)
	full.y1 = region.handle ()->extents.y1;
    if (region.handle ()->extents.x2 < full.x2)
	full.x2 = region.handle ()->extents.x2;
    if (region.handle ()->extents.y2 < full.y2)
	full.y2 = region.handle ()->extents.y2;

    if (full.x1 < full.x2 && full.y1 < full.y2)
    {
	BoxPtr  pBox;
	int     nBox;
	BoxPtr  pClip;
	int     nClip;
	BoxRec  cbox;
	int     it, x1, y1, x2, y2;
	bool    rect = true;

	for (it = 0; it < nMatrix; it++)
	{
	    if (matrix[it].xy != 0.0f || matrix[it].yx != 0.0f)
	    {
		rect = false;
		break;
	    }
	}

	pBox = const_cast <Region> (region.handle ())->rects;
	nBox = const_cast <Region> (region.handle ())->numRects;

	while (nBox--)
	{
	    x1 = pBox->x1;
	    y1 = pBox->y1;
	    x2 = pBox->x2;
	    y2 = pBox->y2;

	    pBox++;

	    if (x1 < full.x1)
		x1 = full.x1;
	    if (y1 < full.y1)
		y1 = full.y1;
	    if (x2 > full.x2)
		x2 = full.x2;
	    if (y2 > full.y2)
		y2 = full.y2;

	    if (x1 < x2 && y1 < y2)
	    {
		nClip = const_cast <Region> (clip.handle ())->numRects;

		if (nClip == 1)
		{
		    addQuads (priv->vertexBuffer, matrix, nMatrix,
			      x1, y1, x2, y2,
			      rect,
			      maxGridWidth, maxGridHeight);
		}
		else
		{
		    pClip = const_cast <Region> (clip.handle ())->rects;

		    while (nClip--)
		    {
			cbox = *pClip;

			pClip++;

			if (cbox.x1 < x1)
			    cbox.x1 = x1;
			if (cbox.y1 < y1)
			    cbox.y1 = y1;
			if (cbox.x2 > x2)
			    cbox.x2 = x2;
			if (cbox.y2 > y2)
			    cbox.y2 = y2;

			if (cbox.x1 < cbox.x2 && cbox.y1 < cbox.y2)
			{
			    addQuads (priv->vertexBuffer, matrix, nMatrix,
				      cbox.x1, cbox.y1, cbox.x2, cbox.y2,
				      rect,
				      maxGridWidth, maxGridHeight);
			}
		    }
		}
	    }
	}
    }
}

#ifndef USE_GLES
static void
enableLegacyOBSAndRender (GLScreen                  *gs,
					 GLWindow	    *w,
					 GLTexture	    *texture,
                          const GLMatrix            &transform,
                          const GLWindowPaintAttrib &attrib,
					 GLTexture::Filter  filter,
					 unsigned int	    mask)
{
    // XXX: This codepath only works with !GL::vbo so that's the only case
    //      where you'll find it's called. At least for now.

    if (GL::canDoSaturated && attrib.saturation != COLOR)
    {
	GLfloat constant[4];

	texture->enable (filter);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	glColor4f (1.0f, 1.0f, 1.0f, 0.5f);

	GL::activeTexture (GL_TEXTURE1_ARB);

	texture->enable (filter);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	if (GL::canDoSlightlySaturated && attrib.saturation > 0)
	{
	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	    constant[0] = 0.5f + 0.5f * RED_SATURATION_WEIGHT;
	    constant[1] = 0.5f + 0.5f * GREEN_SATURATION_WEIGHT;
	    constant[2] = 0.5f + 0.5f * BLUE_SATURATION_WEIGHT;
	    constant[3] = 1.0;

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    GL::activeTexture (GL_TEXTURE2_ARB);

	    texture->enable (filter);

	    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	    constant[3] = attrib.saturation / 65535.0f;

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    if (attrib.opacity < OPAQUE ||
		attrib.brightness != BRIGHT)
	    {
		GL::activeTexture (GL_TEXTURE3_ARB);

		texture->enable (filter);

		constant[3] = attrib.opacity / 65535.0f;
		constant[0] = constant[1] = constant[2] = constant[3] *
		    attrib.brightness / 65535.0f;

		glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

		w->vertexBuffer ()->render (transform, attrib);

		texture->disable ();

		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		GL::activeTexture (GL_TEXTURE2_ARB);
	    }
	    else
	    {
		w->vertexBuffer ()->render (transform, attrib);
	    }

	    texture->disable ();

	    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	    GL::activeTexture (GL_TEXTURE1_ARB);
	}
	else
	{
	    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

	    constant[3] = attrib.opacity / 65535.0f;
	    constant[0] = constant[1] = constant[2] = constant[3] *
			  attrib.brightness / 65535.0f;

	    constant[0] = 0.5f + 0.5f * RED_SATURATION_WEIGHT   * constant[0];
	    constant[1] = 0.5f + 0.5f * GREEN_SATURATION_WEIGHT * constant[1];
	    constant[2] = 0.5f + 0.5f * BLUE_SATURATION_WEIGHT  * constant[2];

	    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

	    w->vertexBuffer ()->render (transform, attrib);
	}

	texture->disable ();

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	GL::activeTexture (GL_TEXTURE0_ARB);

	texture->disable ();

	glColor4usv (defaultColor);
	gs->setTexEnvMode (GL_REPLACE);
    }
    else
    {
	texture->enable (filter);

	if (mask & PAINT_WINDOW_BLEND_MASK)
	{
	    if (attrib.opacity != OPAQUE ||
		attrib.brightness != BRIGHT)
	    {
		GLushort color;

		color = (attrib.opacity * attrib.brightness) >> 16;

		gs->setTexEnvMode (GL_MODULATE);
		glColor4us (color, color, color, attrib.opacity);

		w->vertexBuffer ()->render (transform, attrib);

		glColor4usv (defaultColor);
		gs->setTexEnvMode (GL_REPLACE);
	    }
	    else
	    {
		w->vertexBuffer ()->render (transform, attrib);
	    }
	}
	else if (attrib.brightness != BRIGHT)
	{
	    gs->setTexEnvMode (GL_MODULATE);
	    glColor4us (attrib.brightness, attrib.brightness,
			attrib.brightness, BRIGHT);

	    w->vertexBuffer ()->render (transform, attrib);

	    glColor4usv (defaultColor);
	    gs->setTexEnvMode (GL_REPLACE);
	}
	else
	{
	    w->vertexBuffer ()->render (transform, attrib);
	}

	texture->disable ();
    }
}
#endif

void
GLWindow::glDrawTexture (GLTexture          *texture,
                         const GLMatrix            &transform,
			 const GLWindowPaintAttrib &attrib,
			 unsigned int       mask)
{
    WRAPABLE_HND_FUNCTN (glDrawTexture, texture, transform, attrib, mask)

    GLTexture::Filter filter;

    if (mask & PAINT_WINDOW_BLEND_MASK)
	glEnable (GL_BLEND);

    if (mask & (PAINT_WINDOW_TRANSFORMED_MASK |
		PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK))
	filter = priv->gScreen->filter (SCREEN_TRANS_FILTER);
    else
	filter = priv->gScreen->filter (NOTHING_TRANS_FILTER);

    glActiveTexture(GL_TEXTURE0);
    texture->enable (filter);

    #ifdef USE_GLES
    priv->vertexBuffer->render (transform, attrib);
    #else

    if (!GLVertexBuffer::enabled ())
	enableLegacyOBSAndRender (priv->gScreen, this, texture, transform,
						 attrib, filter, mask);
    else
	priv->vertexBuffer->render (transform, attrib);
    #endif

    priv->shaders.clear ();
    texture->disable ();

    if (mask & PAINT_WINDOW_BLEND_MASK)
	glDisable (GL_BLEND);
}

bool
GLWindow::glDraw (const GLMatrix     &transform,
		  const GLWindowPaintAttrib &attrib,
		  const CompRegion   &region,
		  unsigned int       mask)
{
    WRAPABLE_HND_FUNCTN_RETURN (bool, glDraw, transform,
			      attrib, region, mask)

    const CompRegion &reg = (mask & PAINT_WINDOW_TRANSFORMED_MASK) ?
                            infiniteRegion : region;

    if (reg.isEmpty ())
	return true;

    if (!priv->window->isViewable () ||
	!priv->cWindow->damaged ())
	return true;

    if (textures ().empty () && !bind ())
	return false;

    if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	mask |= PAINT_WINDOW_BLEND_MASK;

    GLTexture::MatrixList ml (1);

    //
    // Don't assume all plugins leave TexEnvMode in a clean state (GL_REPLACE).
    // Sometimes plugins forget to clean up correctly, so make sure we're
    // in the correct mode or else windows could be rendered incorrectly
    // like in LP: #877920.
    //
    priv->gScreen->setTexEnvMode (GL_REPLACE);

    if (priv->updateState & PrivateGLWindow::UpdateMatrix)
	priv->setWindowMatrix ();

    if (priv->updateState & PrivateGLWindow::UpdateRegion)
	priv->updateWindowRegions ();

    for (unsigned int i = 0; i < priv->textures.size (); i++)
    {
	ml[0] = priv->matrices[i];
	priv->vertexBuffer->begin ();
	glAddGeometry (ml, priv->regions[i], reg);
	priv->vertexBuffer->end ();

	if (priv->vertexBuffer->countVertices())
	    glDrawTexture (priv->textures[i], transform, attrib, mask);
    }

    return true;
}

bool
GLWindow::glPaint (const GLWindowPaintAttrib &attrib,
		   const GLMatrix            &transform,
		   const CompRegion          &region,
		   unsigned int              mask)
{
    WRAPABLE_HND_FUNCTN_RETURN (bool, glPaint, attrib, transform, region, mask)

    bool               status;

    priv->lastPaint = attrib;

    if (priv->window->alpha () || attrib.opacity != OPAQUE)
	mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

    priv->lastMask = mask;

    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
    {
	if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	    return false;

	if (mask & PAINT_WINDOW_NO_CORE_INSTANCE_MASK)
	    return false;

	if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	    return false;

	if (priv->window->shaded ())
	    return false;

	return true;
    }

    if (mask & PAINT_WINDOW_NO_CORE_INSTANCE_MASK)
	return true;

    status = glDraw (transform, attrib, region, mask);

    return status;
}
