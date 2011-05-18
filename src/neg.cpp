/*
 * Copyright (c) 2006 Darryll Truchan <moppsy@comcast.net>
 *
 * Pixel shader negating by Dennis Kasprzyk <onestone@beryl-project.org>
 * Usage of matches by Danny Baumann <maniac@beryl-project.org>
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
 *
 */

#include "neg.h"

using namespace GLFragment;

COMPIZ_PLUGIN_20090315 (neg, NegPluginVTable);

void
NegWindow::toggle ()
{
    NEG_SCREEN (screen);

    /* toggle window negative flag */
    isNeg = !isNeg;

    /* check exclude list */
    if (ns->optionGetExcludeMatch ().evaluate (window))
	isNeg = false;

    /* cause repainting */
    cWindow->addDamage ();

    if (isNeg)
	gWindow->glDrawTextureSetEnabled (this, true);
    else
	gWindow->glDrawTextureSetEnabled (this, false);
}

void
NegScreen::ToggleScreen ()
{
    /* toggle screen negative flag */
    isNeg = !isNeg;

    /* toggle every window */
    foreach (CompWindow *w, screen->windows ())
    {
	NEG_WINDOW (w);

	nw->toggle ();
    }
}

bool
NegScreen::toggle (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options,
		   bool		      all)
{
    if (all)
    {
	foreach (CompWindow *w, screen->windows ())
	    NegWindow::get (w)->toggle ();
	isNeg =!isNeg;
    }
    else
    {
	Window     xid;
	CompWindow *w;

	xid = CompOption::getIntOptionNamed (options, "window");
	w   = screen->findWindow (xid);
	if (w)
	    NegWindow::get (w)->toggle ();
    }

    return true;
}

int
NegScreen::getFragmentFunction (GLTexture *texture,
				bool      alpha)
{
    int handle = 0;

    if (alpha && negAlphaFunction)
	handle = negAlphaFunction;
    else if (!alpha && negFunction)
	handle = negFunction;

    if (!handle)
    {
	FunctionData data;
	int          target;

	if (alpha)
	    data.addTempHeaderOp ("neg");

	if (texture->target () == GL_TEXTURE_2D)
	    target = COMP_FETCH_TARGET_2D;
	else
	    target = COMP_FETCH_TARGET_RECT;

	data.addFetchOp ("output", NULL, target);

	if (alpha)
	{
	    data.addDataOp ("RCP neg.a, output.a;");
	    data.addDataOp ("MAD output.rgb, -neg.a, output, 1.0;");
	}
	else
	    data.addDataOp ("SUB output.rgb, 1.0, output;");

	if (alpha)
	    data.addDataOp ("MUL output.rgb, output.a, output;");

	data.addColorOp ("output", "output");

	if (!data.status ())
	    return 0;

	handle = data.createFragmentFunction ("neg");

	if (alpha)
	    negAlphaFunction = handle;
	else
	    negFunction = handle;
    }

    return handle;
}

void
NegWindow::glDrawTexture (GLTexture          *texture,
			  GLFragment::Attrib &attrib,
			  unsigned int       mask)
{
    GLTexture::Filter filter;
    bool              doNeg = false;
    GLTexture         *tex = NULL;

    NEG_SCREEN (screen);

    if (isNeg)
    {
	if (ns->optionGetNegDecorations ())
	{
	    doNeg = true;
	    tex   = texture;
	}
	else
	{
	    doNeg = false;
	    for (unsigned int i = 0; i < gWindow->textures ().size (); i++)
	    {
		tex = gWindow->textures ()[i];
		doNeg = (texture->name () == tex->name ());
		if (doNeg)
		    break;
	    }
	}
    }

    if (doNeg && tex)
    {
	/* Fragment program negation */
	if (GL::fragmentProgram)
	{
	    GLFragment::Attrib fa = attrib;
	    int                function;
	    bool               alpha = true;

	    if (texture->name () == tex->name ()) /* Not a decoration */
		alpha = window->alpha ();

	    function = ns->getFragmentFunction (texture, alpha);
	    if (function)
		fa.addFunction (function);

	    gWindow->glDrawTexture (texture, fa, mask);
	}
	else /* Texture manipulation negation */
	{
	    /* this is for the most part taken from paint.c */

	    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
		filter = ns->gScreen->filter (WINDOW_TRANS_FILTER);
	    else if (mask & PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK)
		filter = ns->gScreen->filter (SCREEN_TRANS_FILTER);
	    else
		filter = ns->gScreen->filter (NOTHING_TRANS_FILTER);

	    /* if we can adjust saturation, even if it's just on and off */
	    if (GL::canDoSaturated && attrib.getSaturation () != COLOR)
	    {
		GLfloat constant[4];

		/* if the paint mask has this set we want to blend */
		if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
		    glEnable (GL_BLEND);

		/* enable the texture */
		texture->enable (filter);

		/* texture combiner */
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);

		/* negate */
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB,
			   GL_ONE_MINUS_SRC_COLOR);

		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glColor4f (1.0f, 1.0f, 1.0f, 0.5f);

		/* make another texture active */
		GL::activeTexture (GL_TEXTURE1_ARB);

		/* enable that texture */
		texture->enable (filter);

		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		/* if we can do saturation that is in between min and max */
		if (GL::canDoSlightlySaturated && attrib.getSaturation () > 0)
		{
		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

    		    constant[0] = 0.5f + 0.5f * RED_SATURATION_WEIGHT;
		    constant[1] = 0.5f + 0.5f * GREEN_SATURATION_WEIGHT;
		    constant[2] = 0.5f + 0.5f * BLUE_SATURATION_WEIGHT;
		    constant[3] = 1.0;

		    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

    		    /* make another texture active */
		    GL::activeTexture (GL_TEXTURE2_ARB);

		    /* enable that texture */
		    texture->enable (filter);

	    	    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);

		    /* negate */
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB,
			       GL_ONE_MINUS_SRC_COLOR);

		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

    		    /* color constant */
		    constant[3] = attrib.getSaturation () / 65535.0f;

		    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

    		    /* if we are not opaque or not fully bright */
		    if (attrib.getOpacity () < OPAQUE ||
			attrib.getBrightness () != BRIGHT)
		    {
			/* make another texture active */
			GL::activeTexture (GL_TEXTURE3_ARB);

			/* enable that texture */
			texture->enable (filter);

			/* color constant */
			constant[3] = attrib.getOpacity () / 65535.0f;
			constant[0] = constant[1] = constant[2] =
			    constant[3] * attrib.getBrightness () / 65535.0f;

			glTexEnvfv(GL_TEXTURE_ENV,
				   GL_TEXTURE_ENV_COLOR, constant);

			glTexEnvf(GL_TEXTURE_ENV,
				  GL_TEXTURE_ENV_MODE, GL_COMBINE);

			glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
			glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
			glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB,
				   GL_SRC_COLOR);
			glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB,
				   GL_SRC_COLOR);

			glTexEnvf (GL_TEXTURE_ENV,
				   GL_COMBINE_ALPHA, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV,
				  GL_SOURCE0_ALPHA, GL_PREVIOUS);
			glTexEnvf(GL_TEXTURE_ENV,
				  GL_SOURCE1_ALPHA, GL_CONSTANT);
			glTexEnvf(GL_TEXTURE_ENV,
				  GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			glTexEnvf(GL_TEXTURE_ENV,
				  GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

			/* draw the window geometry */
			gWindow->glDrawGeometry ();

			/* disable the current texture */
			texture->disable ();

			/* set texture mode back to replace */
			glTexEnvi (GL_TEXTURE_ENV,
				   GL_TEXTURE_ENV_MODE, GL_REPLACE);

			/* re-activate last texture */
			GL::activeTexture (GL_TEXTURE2_ARB);
		    }
		    else
		    {
			/* fully opaque and bright */

			/* draw the window geometry */
			gWindow->glDrawGeometry ();
		    }

		    /* disable the current texture */
		    texture->disable ();

	    	    /* set the texture mode back to replace */
		    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		    /* re-activate last texture */
		    GL::activeTexture (GL_TEXTURE1_ARB);
		}
		else
		{
		    /* fully saturated or fully unsaturated */

		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

    		    /* color constant */
		    constant[3] = attrib.getOpacity () / 65535.0f;
		    constant[0] = constant[1] = constant[2] =
			constant[3] * attrib.getBrightness () / 65535.0f;

		    constant[0] =
			0.5f + 0.5f * RED_SATURATION_WEIGHT * constant[0];
		    constant[1] =
			0.5f + 0.5f * GREEN_SATURATION_WEIGHT * constant[1];
		    constant[2] =
			0.5f + 0.5f * BLUE_SATURATION_WEIGHT * constant[2];

		    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);

    		    /* draw the window geometry */
		    gWindow->glDrawGeometry ();
		}

		/* disable the current texture */
		texture->disable ();

		/* set the texture mode back to replace */
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		/* re-activate last texture */
		GL::activeTexture (GL_TEXTURE0_ARB);

		/* disable that texture */
		texture->disable ();

		/* set the default color */
		glColor4usv (defaultColor);

		/* set screens texture mode back to replace */
		ns->gScreen->setTexEnvMode (GL_REPLACE);

		/* if it's a translucent window, disable blending */
		if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
		    glDisable (GL_BLEND);
	    }
	    else
	    {
		/* no saturation adjustments */

		/* enable the current texture */
		texture->enable (filter);

		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);

		/* negate */
		glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB,
			  GL_ONE_MINUS_SRC_COLOR);

		/* we are not opaque or fully bright */
		if ((mask & PAINT_WINDOW_TRANSLUCENT_MASK) ||
		    attrib.getBrightness () != BRIGHT)
		{
		    GLfloat constant[4];

		    /* enable blending */
		    glEnable (GL_BLEND);

		    /* color constant */
		    constant[3] = attrib.getOpacity () / 65535.0f;
		    constant[0] = constant[1] = constant[2] =
			constant[3] * attrib.getBrightness () / 65535.0f;

		    glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constant);
		    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);

		    /* negate */
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB,
			       GL_ONE_MINUS_SRC_COLOR);

		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		    glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		    glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

    		    /* draw the window geometry */
		    gWindow->glDrawGeometry ();

		    /* disable blending */
		    glDisable (GL_BLEND);
		}
		else
		{
		    /* no adjustments to saturation, brightness or opacity */

		    /* draw the window geometry */
		    gWindow->glDrawGeometry ();
		}

		/* disable the current texture */
		texture->disable ();

		/* set the screens texture mode back to replace */
		ns->gScreen->setTexEnvMode (GL_REPLACE);
	    }
	}
    }
    else
    {
	/* not negative */
	gWindow->glDrawTexture (texture, attrib, mask);
    }
}

void
NegScreen::optionChanged (CompOption          *opt,
			  NegOptions::Options num)
{
    switch (num)
    {
    case NegOptions::NegMatch:
    case NegOptions::ExcludeMatch:
	{
	    foreach (CompWindow *w, screen->windows ())
	    {
		bool isNowNeg;
		NEG_WINDOW (w);

		isNowNeg = optionGetNegMatch ().evaluate (w);
		isNowNeg = isNowNeg && !optionGetExcludeMatch ().evaluate (w);

		if (isNowNeg && isNeg && !nw->isNeg)
		    nw->toggle ();
		else if (!isNowNeg && nw->isNeg)
		    nw->toggle ();
	    }
	}
	break;
    default:
	break;
    }
}

NegScreen::NegScreen (CompScreen *screen) :
    PluginClassHandler <NegScreen, CompScreen> (screen),
    NegOptions (),
    negFunction (0),
    negAlphaFunction (0),
    isNeg (false),
    gScreen (GLScreen::get (screen))
{
    optionSetWindowToggleKeyInitiate (boost::bind (&NegScreen::toggle, this,
						   _1, _2, _3,
						   false));
    optionSetScreenToggleKeyInitiate (boost::bind (&NegScreen::toggle, this,
						   _1, _2, _3,
						   true));

    optionSetNegMatchNotify (boost::bind (&NegScreen::optionChanged, this,
					  _1, _2));
    optionSetExcludeMatchNotify (boost::bind (&NegScreen::optionChanged, this,
					      _1, _2));

}

void
NegWindow::postLoad ()
{
    if (isNeg)
    {
	cWindow->addDamage ();
	gWindow->glDrawTextureSetEnabled (this, true);
    }
}
	

NegWindow::NegWindow (CompWindow *window) :
    PluginClassHandler <NegWindow, CompWindow> (window),
    PluginStateWriter <NegWindow> (this, window->id ()),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    isNeg (0)
{
    GLWindowInterface::setHandler (gWindow, false);

    NEG_SCREEN (screen);

    /* Taken from ObjectAdd, since there is no equavilent
     * we check for screenNeg == true in ctor */

    if (ns->isNeg && ns->optionGetNegMatch ().evaluate (window))
	toggle ();
}

NegWindow::~NegWindow ()
{
    writeSerializedData ();
}

bool
NegPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
