/**
 *
 * Compiz group plugin
 *
 * glow.cpp
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

GlowTexture::Properties::Properties () :
    textureData (NULL),
    textureSize (0),
    glowOffset (0)
{
}

GlowTexture::Properties::~Properties ()
{
}

GlowTexture::Quads::Quads ()
{
}

GlowTexture::Quads::~Quads ()
{
}

void
GroupWindow::computeGlowQuads (GLTexture::Matrix *matrix)
{
    CompRect	      *box;
    GLTexture::Matrix *quadMatrix;
    int               glowSize, glowOffset;

    GROUP_SCREEN (screen);

    if (gs->optionGetGlow () && matrix)
    {
	if (!glowQuads)
	    glowQuads = new GlowTexture::Quads[NUM_GLOWQUADS];
	if (!glowQuads)
	    return;
    }
    else
    {
	if (glowQuads)
	{
	    delete[] glowQuads;
	    glowQuads = NULL;
	}
	return;
    }

    glowSize = gs->optionGetGlowSize ();
    glowOffset = (glowSize * gs->glowTexture.properties[gs->optionGetGlowType ()].glowOffset /
		  gs->glowTexture.properties[gs->optionGetGlowType ()].textureSize) + 1;

    /* Top left corner */
    box = &glowQuads[GLOWQUAD_TOPLEFT].box;
    glowQuads[GLOWQUAD_TOPLEFT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_TOPLEFT].matrix;

    box->setX (WIN_REAL_X (window) - glowSize + glowOffset);
    box->setY (WIN_REAL_Y (window) - glowSize + glowOffset);
    box->setWidth (WIN_REAL_X (window) + glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + glowOffset - box->y ());

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 1.0f / (glowSize);
    quadMatrix->x0 = -(box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = -(box->y1 () * quadMatrix->yy);

    box->setWidth (MIN (WIN_REAL_X (window) + glowOffset - box->x (),
		   WIN_REAL_X (window) + (WIN_REAL_WIDTH (window) / 2) - box->x ()));
    box->setHeight (MIN (WIN_REAL_Y (window) + glowOffset - box->y (),
		   WIN_REAL_Y (window) + (WIN_REAL_HEIGHT (window) / 2) - box->y ()));

    /* Top right corner */
    box = &glowQuads[GLOWQUAD_TOPRIGHT].box;
    glowQuads[GLOWQUAD_TOPRIGHT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_TOPRIGHT].matrix;

    box->setX (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset);
    box->setY (WIN_REAL_Y (window) - glowSize + glowOffset);
    box->setWidth (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) + glowSize - glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + glowOffset - box->y ());

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = -(box->y1 () * quadMatrix->yy);

    box->setX (MAX (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset,
		   WIN_REAL_X (window) + (WIN_REAL_WIDTH (window) / 2)));
    box->setWidth (MIN (WIN_REAL_Y (window) + glowOffset - box->y (),
		        WIN_REAL_Y (window) + (WIN_REAL_HEIGHT (window) / 2) - box->y ()));

    /* Bottom left corner */
    box = &glowQuads[GLOWQUAD_BOTTOMLEFT].box;
    glowQuads[GLOWQUAD_BOTTOMLEFT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_BOTTOMLEFT].matrix;

    box->setX (WIN_REAL_X (window) - glowSize + glowOffset);
    box->setY (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset);
    box->setWidth (WIN_REAL_X (window) + glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) + glowSize - glowOffset -box->y ());

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = -(box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = 1.0f - (box->y1 () * quadMatrix->yy);

    box->setY (MAX (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset,
		   WIN_REAL_Y (window) + (WIN_REAL_HEIGHT (window) / 2)));
    box->setWidth (MIN (WIN_REAL_X (window) + glowOffset,
		   WIN_REAL_X (window) + (WIN_REAL_WIDTH (window) / 2)) -box->x ());

    /* Bottom right corner */
    box = &glowQuads[GLOWQUAD_BOTTOMRIGHT].box;
    glowQuads[GLOWQUAD_BOTTOMRIGHT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_BOTTOMRIGHT].matrix;

    box->setX (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset);
    box->setY (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset);
    box->setWidth (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) + glowSize - glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) + glowSize - glowOffset - box->y ());

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = 1.0 - (box->y1 () * quadMatrix->yy);

    box->setX (MAX (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset,
		   WIN_REAL_X (window) + (WIN_REAL_WIDTH (window) / 2)));
    box->setY (MAX (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset,
		   WIN_REAL_Y (window) + (WIN_REAL_HEIGHT (window) / 2)));

    /* Top edge */
    box = &glowQuads[GLOWQUAD_TOP].box;
    glowQuads[GLOWQUAD_TOP].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_TOP].matrix;

    box->setX (WIN_REAL_X (window) + glowOffset);
    box->setY (WIN_REAL_Y (window) - glowSize + glowOffset);
    box->setWidth (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + glowOffset - box->y ());

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = -(box->y1 () * quadMatrix->yy);

    /* Bottom edge */
    box = &glowQuads[GLOWQUAD_BOTTOM].box;
    glowQuads[GLOWQUAD_BOTTOM].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_BOTTOM].matrix;

    box->setX (WIN_REAL_X (window) + glowOffset);
    box->setY (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset);
    box->setWidth (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) + glowSize - glowOffset - box->y ());

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = 1.0 - (box->y1 () * quadMatrix->yy);

    /* Left edge */
    box = &glowQuads[GLOWQUAD_LEFT].box;
    glowQuads[GLOWQUAD_LEFT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_LEFT].matrix;

    box->setX (WIN_REAL_X (window) - glowSize + glowOffset);
    box->setY (WIN_REAL_Y (window) + glowOffset);
    box->setWidth (WIN_REAL_X (window) + glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset - box->y ());

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = -(box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = 1.0;

    /* Right edge */
    box = &glowQuads[GLOWQUAD_RIGHT].box;
    glowQuads[GLOWQUAD_RIGHT].matrix = *matrix;
    quadMatrix = &glowQuads[GLOWQUAD_RIGHT].matrix;

    box->setX (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) - glowOffset);
    box->setY (WIN_REAL_Y (window) + glowOffset);
    box->setWidth (WIN_REAL_X (window) + WIN_REAL_WIDTH (window) + glowSize - glowOffset - box->x ());
    box->setHeight (WIN_REAL_Y (window) + WIN_REAL_HEIGHT (window) - glowOffset - box->y ());

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = 1.0 - (box->x1 () * quadMatrix->xx);
    quadMatrix->y0 = 1.0;
}
