/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
 *
 * Ported to GLES by : Travis Watkins
 *                     (C) 2011 Linaro Limited
 * E-mail            : travis.watkins@linaro.org
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "private.h"

// use the old structure then copy the data we want out of it
class GridGeometry
{
    public:

	GridGeometry () :
	    vertices (NULL),
	    vertexSize (0),
	    vertexStride (0),
	    vCount (0),
	    texUnits (0),
	    texCoordSize (0)
	{
	}

    public:

	GLfloat  *vertices;
	int      vertexSize;
	int      vertexStride;
	int      vCount;
	int      texUnits;
	int      texCoordSize;
};


// =====================  Effect: Dodge  =========================

GridAnim::GridModel::GridObject::GridObject () :
    mPosition (0, 0, 0),
    mGridPosition (0, 0),
    mOffsetTexCoordForQuadBefore (0, 0),
    mOffsetTexCoordForQuadAfter (0, 0)
{
}

void
GridAnim::GridModel::GridObject::setGridPosition (Point &gridPosition)
{
    mGridPosition = gridPosition;
}

GridAnim::GridModel::GridModel (CompWindow *w,
				WindowEvent curWindowEvent,
				int height,
				int gridWidth,
				int gridHeight,
				int decorTopHeight,
				int decorBottomHeight) :
    mScale (1.0f, 1.0f),
    mScaleOrigin (0, 0)
{
    mNumObjects = (unsigned)(gridWidth * gridHeight);
    mObjects = new GridObject[mNumObjects];

    initObjects (curWindowEvent,
		 height,
		 gridWidth, gridHeight,
		 decorTopHeight, decorBottomHeight);
}

GridAnim::GridModel::~GridModel ()
{
    delete[] mObjects;
}

void
GridAnim::GridModel::initObjects (WindowEvent curWindowEvent,
				  int height,
				  int gridWidth, int gridHeight,
				  int decorTopHeight, int decorBottomHeight)
{
    int gridX, gridY;
    int nGridCellsX, nGridCellsY;

    // number of grid cells in x direction
    nGridCellsX = gridWidth - 1;

    if (curWindowEvent == WindowEventShade ||
	curWindowEvent == WindowEventUnshade)
    {
	// Number of grid cells in y direction.
	// One allocated for top, one for bottom.
	nGridCellsY = gridHeight - 3;

	float winContentsHeight =
	    height - decorTopHeight - decorBottomHeight;

	//Top
	for (gridX = 0; gridX < gridWidth; gridX++)
	{
	    Point gridPos ((float)gridX / nGridCellsX, 0);

	    mObjects[gridX].setGridPosition (gridPos);
	}

	// Window contents
	for (gridY = 1; gridY < gridHeight - 1; gridY++)
	{
	    float inWinY =
		(gridY - 1) * winContentsHeight / nGridCellsY +
		decorTopHeight;
	    float gridPosY = inWinY / height;

	    for (gridX = 0; gridX < gridWidth; gridX++)
	    {
		Point gridPos ((float)gridX / nGridCellsX, gridPosY);
		mObjects[gridY * gridWidth + gridX].setGridPosition (gridPos);
	    }
	}

	// Bottom (gridY is gridHeight-1 now)
	for (gridX = 0; gridX < gridWidth; gridX++)
	{
	    Point gridPos ((float)gridX / nGridCellsX, 1);
	    mObjects[gridY * gridWidth + gridX].setGridPosition (gridPos);
	}
    }
    else
    {
	int objIndex = 0;

	// number of grid cells in y direction
	nGridCellsY = gridHeight - 1;

	for (gridY = 0; gridY < gridHeight; gridY++)
	{
	    for (gridX = 0; gridX < gridWidth; gridX++)
	    {
		// TODO Optimize
		Point gridPos ((float)gridX / nGridCellsX,
			       (float)gridY / nGridCellsY);
		mObjects[objIndex].setGridPosition (gridPos);
		objIndex++;
	    }
	}
    }
}

void
GridAnim::GridModel::move (float tx,
			   float ty)
{
    GridObject *object = mObjects;
    for (unsigned int i = 0; i < mNumObjects; i++, object++)
    {
	object->mPosition.add (Point3d (tx, ty, 0));
    }
}

void
GridAnim::updateBB (CompOutput &output)
{
    GridModel::GridObject *object = mModel->mObjects;
    for (unsigned int i = 0; i < mModel->mNumObjects; i++, object++)
    {
	mAWindow->expandBBWithPoint (object->position ().x () + 0.5,
				     object->position ().y () + 0.5);
    }
}

void
GridAnim::initGrid ()
{
    mGridWidth = 2;
    mGridHeight = 2;
}

GridAnim::GridAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float duration,
		    const AnimEffect info,
		    const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    mModel (NULL),
    mUseQTexCoord (false)
{
}

void
GridAnim::init ()
{
    initGrid ();

    CompRect outRect (mAWindow->savedRectsValid () ?
		      mAWindow->savedOutRect () :
		      mWindow->outputRect ());

    mModel = new GridModel (mWindow, mCurWindowEvent,
			    outRect.height (),
			    mGridWidth, mGridHeight,
			    mDecorTopHeight, mDecorBottomHeight);
}

GridAnim::~GridAnim ()
{
    if (mModel)
	delete mModel;
}

void
GridAnim::addGeometry (const GLTexture::MatrixList &matrix,
		       const CompRegion            &region,
		       const CompRegion            &clip,
		       unsigned int                maxGridWidth,
		       unsigned int                maxGridHeight)
{
    GridGeometry geometry;
    unsigned int nMatrix = matrix.size ();
    int nVertices;
    GLfloat *v;
    int x1, y1, x2, y2;
    float winContentsY, winContentsHeight;
    float deformedX, deformedY;
    float deformedZ = 0;
    int nVertX, nVertY;
    int vSize;
    float gridW, gridH;
    bool rect = true;
    bool notUsing3dCoords = !using3D ();

    if (region.isEmpty ()) // nothing to do
	return;

    for (unsigned int it = 0; it < nMatrix; it++)
    {
	if (matrix[it].xy != 0.0f || matrix[it].yx != 0.0f)
	{
	    rect = false;
	    break;
	}
    }

    CompRect outRect (mAWindow->savedRectsValid () ?
		      mAWindow->savedOutRect () :
		      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
				  mAWindow->savedOutExtents () :
				  mWindow->output ());

    // window output (contents + decorations + shadows) coordinates and size
    int ox = outRect.x ();
    int oy = outRect.y ();
    int owidth = outRect.width ();
    int oheight = outRect.height ();

    // to be used if event is shade/unshade
    winContentsY = oy + outExtents.top;
    winContentsHeight = oheight - outExtents.top - outExtents.bottom;

    geometry.texUnits = (int)nMatrix;
    geometry.vertices = NULL;
    geometry.vCount = 0;
	geometry.texCoordSize = 4;
    geometry.vertexStride = 3 + geometry.texUnits * geometry.texCoordSize;

    vSize = geometry.vertexStride;
    nVertices = geometry.vCount;
    v = geometry.vertices + (nVertices * vSize);

    // For each clip passed to this function
    foreach (const CompRect &pClip, region.rects ())
    {
	x1 = pClip.x1 ();
	y1 = pClip.y1 ();
	x2 = pClip.x2 ();
	y2 = pClip.y2 ();

	gridW = (float)owidth / (mGridWidth - 1);

	if (mCurWindowEvent == WindowEventShade ||
	    mCurWindowEvent == WindowEventUnshade)
	{
	    if (y1 < winContentsY)	// if at top part
	    {
		gridH = mDecorTopHeight;
	    }
	    else if (y2 > winContentsY + winContentsHeight)  // if at bottom
	    {
		gridH = mDecorBottomHeight;
	    }
	    else			// in window contents (only in Y coords)
	    {
		float winContentsHeight =
		    oheight - (mDecorTopHeight + mDecorBottomHeight);
		gridH = winContentsHeight / (mGridHeight - 3);
	    }
	}
	else
	    gridH = (float)oheight / (mGridHeight - 1);

	// nVertX, nVertY: number of vertices for this clip in x and y dimensions
	// + 2 to avoid running short of vertices in some cases
	nVertX = ceil ((x2 - x1) / gridW) + 2;
	nVertY = (gridH ? ceil ((y2 - y1) / gridH) : 0) + 2;

	// Allocate vertices
	int newVertexSize =
	    (nVertices + nVertX * (2 * nVertY - 2)) * vSize;
	if (newVertexSize > geometry.vertexSize || geometry.vertices == NULL)
	{
	    if (geometry.vertices == NULL)
		geometry.vertices = (GLfloat *)
		                      malloc (sizeof (GLfloat) * newVertexSize);
	    else
		geometry.vertices = (GLfloat *)
		  realloc (geometry.vertices, sizeof (GLfloat) * newVertexSize);

	    if (!geometry.vertices)
		return;

	    geometry.vertexSize = newVertexSize;
	    v = geometry.vertices + (nVertices * vSize);
	}

	float rowTexCoordQ = 1;
	float prevRowCellWidth = 0;	// this initial value won't be used
	float rowCellWidth = 0;
	int clipRowSize = nVertX * vSize;

	// For each vertex
	float y = y1;
	for (int jy = 0; jy < nVertY; jy++)
	{
	    float topiyFloat;
	    bool applyOffsets = true;

	    if (y > y2)
		y = y2;

	    // Do calculations for y here to avoid repeating
	    // them unnecessarily in the x loop

	    if (mCurWindowEvent == WindowEventShade ||
		mCurWindowEvent == WindowEventUnshade)
	    {
		if (y1 < winContentsY)	// if at top part
		{
		    topiyFloat = (y - oy) / mDecorTopHeight;
		    topiyFloat = MIN (topiyFloat, 0.999);	// avoid 1.0
		    applyOffsets = false;
		}
		else if (y2 > winContentsY + winContentsHeight)	// if at bottom
		{
		    topiyFloat = (mGridHeight - 2) +
			(mDecorBottomHeight ? (y - winContentsY -
					       winContentsHeight) /
			 mDecorBottomHeight : 0);
		    applyOffsets = false;
		}
		else		// in window contents (only in Y coords)
		{
		    topiyFloat = (mGridHeight - 3) *
			(y - winContentsY) / winContentsHeight + 1;
		}
	    }
	    else
	    {
		topiyFloat = (mGridHeight - 1) * (y - oy) / oheight;
	    }
	    // topiy should be at most (mGridHeight - 2)
	    int topiy = (int)(topiyFloat + 1e-4);

	    if (topiy == mGridHeight - 1)
		topiy--;
	    int bottomiy = topiy + 1;
	    float iny = topiyFloat - topiy;
	    float inyRest = 1 - iny;

	    // End of calculations for y

	    float x = x1;
	    for (int jx = 0; jx < nVertX; jx++)
	    {
		if (x > x2)
		    x = x2;

		// find containing grid cell (leftix rightix) x (topiy bottomiy)
		float leftixFloat =
		    (mGridWidth - 1) * (x - ox) / owidth;
		int leftix = (int)(leftixFloat + 1e-4);

		if (leftix == mGridWidth - 1)
		    leftix--;
		int rightix = leftix + 1;

		// GridModel::GridObjects that are at top, bottom, left, right corners of quad
		GridModel::GridObject *objToTopLeft =
		    &(mModel->mObjects[topiy * mGridWidth + leftix]);
		GridModel::GridObject *objToTopRight =
		    &(mModel->mObjects[topiy * mGridWidth + rightix]);
		GridModel::GridObject *objToBottomLeft =
		    &(mModel->mObjects[bottomiy * mGridWidth + leftix]);
		GridModel::GridObject *objToBottomRight =
		    &(mModel->mObjects[bottomiy * mGridWidth + rightix]);

		Point3d &objToTopLeftPos = objToTopLeft->mPosition;
		Point3d &objToTopRightPos = objToTopRight->mPosition;
		Point3d &objToBottomLeftPos = objToBottomLeft->mPosition;
		Point3d &objToBottomRightPos = objToBottomRight->mPosition;

		// find position in cell by taking remainder of flooring
		float inx = leftixFloat - leftix;
		float inxRest = 1 - inx;

		// Interpolate to find deformed coordinates

		float hor1x = (inxRest * objToTopLeftPos.x () +
			       inx * objToTopRightPos.x ());
		float hor1y = (inxRest * objToTopLeftPos.y () +
			       inx * objToTopRightPos.y ());
		float hor1z = (notUsing3dCoords ? 0 :
			       inxRest * objToTopLeftPos.z () +
			       inx * objToTopRightPos.z ());
		float hor2x = (inxRest * objToBottomLeftPos.x () +
			       inx * objToBottomRightPos.x ());
		float hor2y = (inxRest * objToBottomLeftPos.y () +
			       inx * objToBottomRightPos.y ());
		float hor2z = (notUsing3dCoords ? 0 :
			       inxRest * objToBottomLeftPos.z () +
			       inx * objToBottomRightPos.z ());

		deformedX = inyRest * hor1x + iny * hor2x;
		deformedY = inyRest * hor1y + iny * hor2y;
		deformedZ = inyRest * hor1z + iny * hor2z;

		// Texture coordinates (s, t, r, q)

		if (mUseQTexCoord)
		{
		    if (jx == 1)
			rowCellWidth = deformedX - v[-3];

		    // do only once per row for all rows except row 0
		    if (jy > 0 && jx == 1)
		    {
			rowTexCoordQ = (rowCellWidth / prevRowCellWidth);

			for (unsigned int it = 0; it < nMatrix; it++, v += 4)
			{
			    // update first column
			    // (since we didn't know rowTexCoordQ before)
			    v[-vSize]     *= rowTexCoordQ; // multiply s & t by q
			    v[-vSize + 1] *= rowTexCoordQ;
			    v[-vSize + 3] = rowTexCoordQ;  // copy q
			}
			v -= nMatrix * 4;
		    }
		}

		// Loop for each texture element
		// (4 texture coordinates for each one)
		for (unsigned int it = 0; it < nMatrix; it++, v += 4)
		{
		    float offsetY = 0;

		    if (rect)
		    {
			if (applyOffsets && y < y2)
			    offsetY = objToTopLeft->mOffsetTexCoordForQuadAfter.y ();
			v[0] = COMP_TEX_COORD_X (matrix[it], x); // s
			v[1] = COMP_TEX_COORD_Y (matrix[it], y + offsetY); // t
		    }
		    else
		    {
			if (applyOffsets && y < y2)
			    // FIXME:
			    // The correct y offset below produces wrong
			    // texture coordinates for some reason.
			    offsetY = 0;
			    // offsetY = objToTopLeft->offsetTexCoordForQuadAfter.y;
			v[0] = COMP_TEX_COORD_XY (matrix[it], x, y + offsetY); // s
			v[1] = COMP_TEX_COORD_YX (matrix[it], x, y + offsetY); // t
		    }
		    v[2] = 0; // r

		    if (0 < jy && jy < nVertY - 1)
		    {
			// copy s, t, r to duplicate row
			memcpy (v + clipRowSize, v, 3 * sizeof (GLfloat));
			v[3 + clipRowSize] = 1; // q
		    }

		    if (applyOffsets &&
			objToTopLeft->mOffsetTexCoordForQuadBefore.y () != 0)
		    {
			// After copying to next row, update texture y coord
			// by following object's offset
			offsetY = objToTopLeft->mOffsetTexCoordForQuadBefore.y ();
			if (rect)
			{
			    v[1] = COMP_TEX_COORD_Y (matrix[it], y + offsetY);
			}
			else
			{
			    v[0] = COMP_TEX_COORD_XY (matrix[it],
						      x, y + offsetY);
			    v[1] = COMP_TEX_COORD_YX (matrix[it],
						      x, y + offsetY);
			}
		    }
		    if (mUseQTexCoord)
		    {
			v[3] = rowTexCoordQ; // q

			if (jx > 0)	// since column 0 is updated when jx == 1
			{
			    // multiply s & t by q
			    v[0] *= rowTexCoordQ;
			    v[1] *= rowTexCoordQ;
			}
		    }
		    else
		    {
			v[3] = 1; // q
		    }
		}

		v[0] = deformedX;
		v[1] = deformedY;
		v[2] = deformedZ;

		// Copy vertex coordinates to duplicate row
		if (0 < jy && jy < nVertY - 1)
		    memcpy (v + clipRowSize, v, 3 * sizeof (GLfloat));

		nVertices++;

		// increment x properly (so that coordinates fall on grid intersections)
		x = rightix * gridW + ox;

		v += 3; // move on to next vertex
	    }
	    if (mUseQTexCoord)
		prevRowCellWidth = rowCellWidth;

	    if (0 < jy && jy < nVertY - 1)
	    {
		v += clipRowSize;	// skip the duplicate row
		nVertices += nVertX;
	    }
	    // increment y properly (so that coordinates fall on grid intersections)
	    if (mCurWindowEvent == WindowEventShade ||
		mCurWindowEvent == WindowEventUnshade)
	    {
		y += gridH;
	    }
	    else
	    {
		y = bottomiy * gridH + oy;
	    }
	}
    }

    geometry.vCount = nVertices;
}

GridTransformAnim::GridTransformAnim (CompWindow *w,
				      WindowEvent curWindowEvent,
				      float duration,
				      const AnimEffect info,
				      const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridAnim::GridAnim (w, curWindowEvent, duration, info, icon),
    mUsingTransform (true)
{
}

void
GridTransformAnim::init ()
{
    GridAnim::init ();
    TransformAnim::init ();
}

void
GridTransformAnim::updateBB (CompOutput &output)
{
    if (using3D ())
    {
	GLMatrix wTransform;

	// center for perspective correction
	Point center = getCenter ();

	GLMatrix fullTransform (mTransform.getMatrix ());
	applyPerspectiveSkew (output, fullTransform, center);

	prepareTransform (output, wTransform, fullTransform);

	mAWindow->expandBBWithPoints3DTransform (output,
						 wTransform,
						 0,
						 mModel->objects (),
						 mModel->numObjects ());
    }
    else
    {
	GridModel::GridObject *object = mModel->objects ();
	unsigned int n = mModel->numObjects ();
	for (unsigned int i = 0; i < n; i++, object++)
	{
	    GLVector coords (object->mPosition.x (),
			     object->mPosition.y (), 0, 1);
	    mAWindow->expandBBWithPoint2DTransform (coords, mTransform);
	}
    }
}

void
GridTransformAnim::updateTransform (GLMatrix &wTransform)
{
    if (!mUsingTransform)
	return;

    TransformAnim::updateTransform (wTransform);

    if (using3D ())
    {
	// center for perspective correction
	Point center = getCenter ();

	GLMatrix skewTransform;
	applyPerspectiveSkew (AnimScreen::get (::screen)->output (),
			      skewTransform, center);
	wTransform *= skewTransform;
    }
}

