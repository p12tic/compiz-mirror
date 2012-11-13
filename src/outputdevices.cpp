/*
 * Compiz Core: OutputDevices class
 *
 * Copyright (c) 2012 Canonical Ltd.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <core/option.h>
#include "outputdevices.h"
#include "core_options.h"

namespace compiz { namespace private_screen {

OutputDevices::OutputDevices() :
    outputDevs (),
    overlappingOutputs (false),
    currentOutputDev (0)
{
}

void
OutputDevices::setGeometryOnDevice(unsigned int const nOutput,
    int x, int y,
    const int width, const int height)
{
    if (outputDevs.size() < nOutput + 1)
	outputDevs.resize(nOutput + 1);

    outputDevs[nOutput].setGeometry(x, y, width, height);
}

void
OutputDevices::adoptDevices(unsigned int nOutput)
{
    /* make sure we have at least one output */
    if (!nOutput)
    {
	setGeometryOnDevice(nOutput, 0, 0, screen->width(), screen->height());
	nOutput++;
    }
    if (outputDevs.size() > nOutput)
	outputDevs.resize(nOutput);

    char str[10];
    /* set name, width, height and update rect pointers in all regions */
    for (unsigned int i = 0; i < nOutput; i++)
    {
	snprintf(str, 10, "Output %d", i);
	outputDevs[i].setId(str, i);
    }
    overlappingOutputs = false;
    setCurrentOutput (currentOutputDev);
    for (unsigned int i = 0; i < nOutput - 1; i++)
	for (unsigned int j = i + 1; j < nOutput; j++)
	    if (outputDevs[i].intersects(outputDevs[j]))
		overlappingOutputs = true;
}

void
OutputDevices::updateOutputDevices(CoreOptions& coreOptions, CompScreen* screen)
{
    CompOption::Value::Vector& list = coreOptions.optionGetOutputs();
    unsigned int nOutput = 0;
    int x, y, bits;
    unsigned int uWidth, uHeight;
    int width, height;
    int x1, y1, x2, y2;
    foreach(CompOption::Value & value, list)
    {
	x = 0;
	y = 0;
	uWidth = (unsigned) screen->width();
	uHeight = (unsigned) screen->height();

	bits = XParseGeometry(value.s().c_str(), &x, &y, &uWidth, &uHeight);
	width = (int) uWidth;
	height = (int) uHeight;

	if (bits & XNegative)
	    x = screen->width() + x - width;

	if (bits & YNegative)
	    y = screen->height() + y - height;

	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;

	if (x1 < 0)
	    x1 = 0;
	if (y1 < 0)
	    y1 = 0;
	if (x2 > screen->width())
	    x2 = screen->width();
	if (y2 > screen->height())
	    y2 = screen->height();

	if (x1 < x2 && y1 < y2)
	{
	    setGeometryOnDevice(nOutput, x1, y1, x2 - x1, y2 - y1);
	    nOutput++;
	}
    }
    adoptDevices(nOutput);
}

void
OutputDevices::setCurrentOutput (unsigned int outputNum)
{
    if (outputNum >= outputDevs.size ())
	outputNum = 0;

    currentOutputDev = outputNum;
}

CompRect
OutputDevices::computeWorkareaForBox (const CompRect& box, CompWindowList const& windows)
{
    CompRegion region;
    int        x1, y1, x2, y2;

    region += box;

    foreach (CompWindow *w, windows)
    {
	if (!w->isMapped ())
	    continue;

	if (w->struts ())
	{
	    x1 = w->struts ()->left.x;
	    y1 = w->struts ()->left.y;
	    x2 = x1 + w->struts ()->left.width;
	    y2 = y1 + w->struts ()->left.height;

	    if (y1 < box.y2 () && y2 > box.y1 ())
		region -= CompRect (x1, box.y1 (), x2 - x1, box.height ());

	    x1 = w->struts ()->right.x;
	    y1 = w->struts ()->right.y;
	    x2 = x1 + w->struts ()->right.width;
	    y2 = y1 + w->struts ()->right.height;

	    if (y1 < box.y2 () && y2 > box.y1 ())
		region -= CompRect (x1, box.y1 (), x2 - x1, box.height ());

	    x1 = w->struts ()->top.x;
	    y1 = w->struts ()->top.y;
	    x2 = x1 + w->struts ()->top.width;
	    y2 = y1 + w->struts ()->top.height;

	    if (x1 < box.x2 () && x2 > box.x1 ())
		region -= CompRect (box.x1 (), y1, box.width (), y2 - y1);

	    x1 = w->struts ()->bottom.x;
	    y1 = w->struts ()->bottom.y;
	    x2 = x1 + w->struts ()->bottom.width;
	    y2 = y1 + w->struts ()->bottom.height;

	    if (x1 < box.x2 () && x2 > box.x1 ())
		region -= CompRect (box.x1 (), y1, box.width (), y2 - y1);
	}
    }

    if (region.isEmpty ())
    {
	compLogMessage ("core", CompLogLevelWarn,
			"Empty box after applying struts, ignoring struts");
	return box;
    }

    return region.boundingRect ();
}

void
OutputDevices::computeWorkAreas(CompRect& workArea, bool& workAreaChanged,
	CompRegion& allWorkArea, CompWindowList const& windows)
{
    for (unsigned int i = 0; i < outputDevs.size(); i++)
    {
	CompRect oldWorkArea = outputDevs[i].workArea();
	workArea = computeWorkareaForBox(outputDevs[i], windows);
	if (workArea != oldWorkArea)
	{
	    workAreaChanged = true;
	    outputDevs[i].setWorkArea(workArea);
	}
	allWorkArea += workArea;
    }
}

int
OutputDevices::outputDeviceForGeometry (
	const CompWindow::Geometry& gm,
	int strategy,
	CompScreen* screen) const
{
    int          overlapAreas[outputDevs.size ()];
    int          highest, seen, highestScore;
    int          x, y;
    unsigned int i;
    CompRect     geomRect;

    if (outputDevs.size () == 1)
	return 0;


    if (strategy == CoreOptions::OverlappingOutputsSmartMode)
    {
	int centerX, centerY;

	/* for smart mode, calculate the overlap of the whole rectangle
	   with the output device rectangle */
	geomRect.setWidth (gm.width () + 2 * gm.border ());
	geomRect.setHeight (gm.height () + 2 * gm.border ());

	x = gm.x () % screen->width ();
	centerX = (x + (geomRect.width () / 2));
	if (centerX < 0)
	    x += screen->width ();
	else if (centerX > screen->width ())
	    x -= screen->width ();
	geomRect.setX (x);

	y = gm.y () % screen->height ();
	centerY = (y + (geomRect.height () / 2));
	if (centerY < 0)
	    y += screen->height ();
	else if (centerY > screen->height ())
	    y -= screen->height ();
	geomRect.setY (y);
    }
    else
    {
	/* for biggest/smallest modes, only use the window center to determine
	   the correct output device */
	x = (gm.x () + (gm.width () / 2) + gm.border ()) % screen->width ();
	if (x < 0)
	    x += screen->width ();
	y = (gm.y () + (gm.height () / 2) + gm.border ()) % screen->height ();
	if (y < 0)
	    y += screen->height ();

	geomRect.setGeometry (x, y, 1, 1);
    }

    /* get amount of overlap on all output devices */
    for (i = 0; i < outputDevs.size (); i++)
    {
	CompRect overlap = outputDevs[i] & geomRect;
	overlapAreas[i] = overlap.area ();
    }

    /* find output with largest overlap */
    for (i = 0, highest = 0, highestScore = 0;
	 i < outputDevs.size (); i++)
    {
	if (overlapAreas[i] > highestScore)
	{
	    highest = i;
	    highestScore = overlapAreas[i];
	}
    }

    /* look if the highest score is unique */
    for (i = 0, seen = 0; i < outputDevs.size (); i++)
	if (overlapAreas[i] == highestScore)
	    seen++;

    if (seen > 1)
    {
	/* it's not unique, select one output of the matching ones and use the
	   user preferred strategy for that */
	unsigned int currentSize, bestOutputSize;
	bool         searchLargest;

	searchLargest =
	    (strategy != CoreOptions::OverlappingOutputsPreferSmallerOutput);

	if (searchLargest)
	    bestOutputSize = 0;
	else
	    bestOutputSize = UINT_MAX;

	for (i = 0, highest = 0; i < outputDevs.size (); i++)
	    if (overlapAreas[i] == highestScore)
	    {
		bool bestFit;

		currentSize = outputDevs[i].area ();

		if (searchLargest)
		    bestFit = (currentSize > bestOutputSize);
		else
		    bestFit = (currentSize < bestOutputSize);

		if (bestFit)
		{
		    highest = i;
		    bestOutputSize = currentSize;
		}
	    }
    }

    return highest;
}

}} // namespace compiz { namespace private_screen {

