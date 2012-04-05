/*
 * Copyright © 2008 Dennis Kasprzyk
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 */

#include "privatewindow.h"
#include "core/window.h"


CompWindow::Geometry &
CompWindow::serverGeometry () const
{
    return priv->serverGeometry;
}

CompWindow::Geometry &
CompWindow::geometry () const
{
    return priv->geometry;
}

int
CompWindow::x () const
{
    return priv->geometry.x ();
}

int
CompWindow::y () const
{
    return priv->geometry.y ();
}

CompPoint
CompWindow::pos () const
{
    return CompPoint (priv->geometry.x (), priv->geometry.y ());
}

/* With border */
int
CompWindow::width () const
{
    return priv->geometry.width () +
	    priv->geometry.border ()  * 2;
}

int
CompWindow::height () const
{
    return priv->geometry.height () +
	    priv->geometry.border ()  * 2;;
}

CompSize
CompWindow::size () const
{
    return CompSize (priv->geometry.width () + priv->geometry.border ()  * 2,
		     priv->geometry.height () + priv->geometry.border ()  * 2);
}

int
CompWindow::serverX () const
{
    return priv->serverGeometry.x ();
}

int
CompWindow::serverY () const
{
    return priv->serverGeometry.y ();
}

CompPoint
CompWindow::serverPos () const
{
    return CompPoint (priv->serverGeometry.x (),
		      priv->serverGeometry.y ());
}

/* With border */
int
CompWindow::serverWidth () const
{
    return priv->serverGeometry.widthIncBorders ();
}

int
CompWindow::serverHeight () const
{
    return priv->serverGeometry.heightIncBorders ();
}

const CompSize
CompWindow::serverSize () const
{
    return CompSize (priv->serverGeometry.widthIncBorders (),
		     priv->serverGeometry.heightIncBorders ());
}

CompRect
CompWindow::borderRect () const
{
    return CompRect (priv->geometry.xMinusBorder () - priv->border.left,
		     priv->geometry.yMinusBorder () - priv->border.top,
		     priv->geometry.widthIncBorders () +
		     priv->border.left + priv->border.right,
		     priv->geometry.heightIncBorders () +
		     priv->border.top + priv->border.bottom);
}

CompRect
CompWindow::serverBorderRect () const
{
    return CompRect (priv->serverGeometry.xMinusBorder () - priv->border.left,
		     priv->serverGeometry.yMinusBorder () - priv->border.top,
		     priv->serverGeometry.widthIncBorders () +
		     priv->border.left + priv->border.right,
		     priv->serverGeometry.heightIncBorders() * 2 +
		     priv->border.top + priv->border.bottom);
}

CompRect
CompWindow::inputRect () const
{
    return CompRect (priv->geometry.xMinusBorder () - priv->serverInput.left,
		     priv->geometry.yMinusBorder () - priv->serverInput.top,
		     priv->geometry.widthIncBorders () +
		     priv->serverInput.left + priv->serverInput.right,
		     priv->geometry.heightIncBorders () +
		     priv->serverInput.top + priv->serverInput.bottom);
}

CompRect
CompWindow::serverInputRect () const
{
    return CompRect (priv->serverGeometry.xMinusBorder () - priv->serverInput.left,
		     priv->serverGeometry.yMinusBorder () - priv->serverInput.top,
		     priv->serverGeometry.widthIncBorders () +
		     priv->serverInput.left + priv->serverInput.right,
		     priv->serverGeometry.heightIncBorders () +
		     priv->serverInput.top + priv->serverInput.bottom);
}

CompRect
CompWindow::outputRect () const
{
    return CompRect (priv->geometry.xMinusBorder ()- priv->output.left,
		     priv->geometry.yMinusBorder () - priv->output.top,
		     priv->geometry.widthIncBorders () +
		     priv->output.left + priv->output.right,
		     priv->geometry.heightIncBorders () +
		     priv->output.top + priv->output.bottom);
}

CompRect
CompWindow::serverOutputRect () const
{
    return CompRect (priv->serverGeometry.xMinusBorder () -  priv->output.left,
		     priv->serverGeometry.yMinusBorder () - priv->output.top,
		     priv->serverGeometry.widthIncBorders () +
		     priv->output.left + priv->output.right,
		     priv->serverGeometry.heightIncBorders () +
		     priv->output.top + priv->output.bottom);
}
