/**
 *
 * Compiz benchmark plugin
 *
 * bench.c
 *
 * Copyright : (C) 2006 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
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

#include "bench.h"

COMPIZ_PLUGIN_20090315 (bench, BenchPluginVTable)

void
BenchScreen::preparePaint (int msSinceLastPaint)
{
    float nRrVal;
    float ratio = 0.05;
    int   timediff;

    struct timeval now;

    gettimeofday (&now, 0);

    timediff = TIMEVALDIFF (&now, &mLastRedraw);

    nRrVal = MIN (1.1,
		  (float) cScreen->optimalRedrawTime () / (float) timediff);

    mRrVal = (mRrVal * (1.0 - ratio) ) + (nRrVal * ratio);

    mFps = (mFps * (1.0 - ratio) ) +
	      (1000000.0 / TIMEVALDIFFU (&now, &mLastRedraw) * ratio);

    mLastRedraw = now;

    if (optionGetOutputConsole () && mActive)
    {
	mFrames++;
	mCtime += timediff;

	if (mCtime > optionGetConsoleUpdateTime () * 1000)
	{
	    printf ("[BENCH] : %.0f frames in %.1f seconds = %.3f FPS\n",
		    mFrames, mCtime / 1000.0,
		    mFrames / (mCtime / 1000.0) );
	    mFrames = 0;
	    mCtime = 0;
	}
    }

    cScreen->preparePaint ((mAlpha > 0.0) ? timediff : msSinceLastPaint);

    if (mActive)
	mAlpha += timediff / 1000.0;
    else
	mAlpha -= timediff / 1000.0;

    mAlpha = MIN (1.0, MAX (0.0, mAlpha) );
}

void
BenchScreen::donePaint ()
{
    if (mAlpha > 0.0)
    {
	cScreen->damageScreen ();
	glFlush();
	XSync (::screen->dpy (), false);

	if (optionGetDisableLimiter ())
	{
	    cScreen->setLastRedraw (mInitTime);
	    cScreen->resetTimeMult ();
	}

	if (!mActive)
	    cScreen->resetTimeMult ();
    }

    cScreen->donePaint ();
}

bool
BenchScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			    const GLMatrix            &transform,
			    const CompRegion          &region,
			    CompOutput                *output,
			    unsigned int              mask)
{
    bool status;
    bool isSet;
    unsigned int  fps;
    GLMatrix sTransform (transform);

    status = gScreen->glPaintOutput (sAttrib, transform, region, output, mask);

    if (mAlpha <= 0.0 || !optionGetOutputScreen ())
	return status;

    glGetError();
    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
    GLERR;

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    glPushMatrix ();
    glLoadMatrixf (sTransform.getMatrix ());

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f (1.0, 1.0, 1.0, mAlpha);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTranslatef (optionGetPositionX (), optionGetPositionY (), 0);

    glEnable (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, mBackTex);

    glBegin (GL_QUADS);
    glTexCoord2f (0, 0);
    glVertex2f (0, 0);
    glTexCoord2f (0, 1);
    glVertex2f (0, 256);
    glTexCoord2f (1, 1);
    glVertex2f (512, 256);
    glTexCoord2f (1, 0);
    glVertex2f (512, 0);
    glEnd();

    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable (GL_TEXTURE_2D);

    glTranslatef (53, 83, 0);

    float rrVal = MIN (1.0, MAX (0.0, mRrVal) );

    if (rrVal < 0.5)
    {
	glBegin (GL_QUADS);
	glColor4f (1.0, 0.0, 0.0, mAlpha);
	glVertex2f (0.0, 0.0);
	glVertex2f (0.0, 25.0);
	glColor4f (1.0, rrVal * 2.0, 0.0, mAlpha);
	glVertex2f (330.0 * rrVal, 25.0);
	glVertex2f (330.0 * rrVal, 0.0);
	glEnd();
    }
    else
    {
	glBegin (GL_QUADS);
	glColor4f (1.0, 0.0, 0.0, mAlpha);
	glVertex2f (0.0, 0.0);
	glVertex2f (0.0, 25.0);
	glColor4f (1.0, 1.0, 0.0, mAlpha);
	glVertex2f (165.0, 25.0);
	glVertex2f (165.0, 0.0);
	glEnd();

	glBegin (GL_QUADS);
	glColor4f (1.0, 1.0, 0.0, mAlpha);
	glVertex2f (165.0, 0.0);
	glVertex2f (165.0, 25.0);
	glColor4f (1.0 - ( (rrVal - 0.5) * 2.0), 1.0, 0.0, mAlpha);
	glVertex2f (165.0 + 330.0 * (rrVal - 0.5), 25.0);
	glVertex2f (165.0 + 330.0 * (rrVal - 0.5), 0.0);
	glEnd();
    }

    glColor4f (0.0, 0.0, 0.0, mAlpha);
    glCallList (mDList);
    glTranslatef (72, 45, 0);

    float red;

    if (mFps > 30.0)
	red = 0.0;
    else
	red = 1.0;

    if (mFps <= 30.0 && mFps > 20.0)
	red = 1.0 - ( (mFps - 20.0) / 10.0);

    glColor4f (red, 0.0, 0.0, mAlpha);
    glEnable (GL_TEXTURE_2D);

    isSet = false;

    fps = (mFps * 100.0);
    fps = MIN (999999, fps);

    if (fps >= 100000)
    {
	glBindTexture (GL_TEXTURE_2D, mNumTex[fps / 100000]);
	glCallList (mDList + 1);
	isSet = true;
    }

    fps %= 100000;

    glTranslatef (12, 0, 0);

    if (fps >= 10000 || isSet)
    {
	glBindTexture (GL_TEXTURE_2D, mNumTex[fps / 10000]);
	glCallList (mDList + 1);
	isSet = true;
    }

    fps %= 10000;

    glTranslatef (12, 0, 0);

    if (fps >= 1000 || isSet)
    {
	glBindTexture (GL_TEXTURE_2D, mNumTex[fps / 1000]);
	glCallList (mDList + 1);
    }

    fps %= 1000;

    glTranslatef (12, 0, 0);

    glBindTexture (GL_TEXTURE_2D, mNumTex[fps / 100]);
    glCallList (mDList + 1);
    fps %= 100;

    glTranslatef (19, 0, 0);

    glBindTexture (GL_TEXTURE_2D, mNumTex[fps / 10]);
    glCallList (mDList + 1);
    fps %= 10;

    glTranslatef (12, 0, 0);

    glBindTexture (GL_TEXTURE_2D, mNumTex[fps]);
    glCallList (mDList + 1);

    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable (GL_TEXTURE_2D);

    glPopMatrix();

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glColor4f (1.0, 1.0, 1.0, 1.0);

    glPopAttrib();
    glGetError();

    return status;
}

BenchScreen::BenchScreen (CompScreen *screen) :
    PluginClassHandler<BenchScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mAlpha (0),
    mCtime (0),
    mFrames (0),
    mActive (false)
{
    optionSetInitiateKeyInitiate (boost::bind (&BenchScreen::initiate, this,
					       _3));

    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    glGenTextures (10, mNumTex);
    glGenTextures (1, &mBackTex);

    glGetError();

    glEnable (GL_TEXTURE_2D);

    for (int i = 0; i < 10; i++)
    {
	//Bind the texture
	glBindTexture (GL_TEXTURE_2D, mNumTex[i]);

	//Load the parameters
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_ALPHA, 16, 32, 0,
		      GL_RGBA, GL_UNSIGNED_BYTE, number_data[i]);
	GLERR;
    }

    glBindTexture (GL_TEXTURE_2D, mBackTex);

    //Load the parameters
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D (GL_TEXTURE_2D, 0, 4, 512, 256, 0, GL_RGBA,
		  GL_UNSIGNED_BYTE, image_data);
    GLERR;

    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable (GL_TEXTURE_2D);

    mDList = glGenLists (2);
    glNewList (mDList, GL_COMPILE);

    glLineWidth (2.0);

    glBegin (GL_LINE_LOOP);
    glVertex2f (0, 0);
    glVertex2f (0, 25);
    glVertex2f (330, 25);
    glVertex2f (330, 0);
    glEnd();

    glLineWidth (1.0);

    glBegin (GL_LINES);

    for (int i = 33; i < 330; i += 33)
    {
	glVertex2f (i, 15);
	glVertex2f (i, 25);
    }

    for (int i = 16; i < 330; i += 33)
    {
	glVertex2f (i, 20);
	glVertex2f (i, 25);
    }

    glEnd();

    glEndList();

    glNewList (mDList + 1, GL_COMPILE);
    glBegin (GL_QUADS);
    glTexCoord2f (0, 0);
    glVertex2f (0, 0);
    glTexCoord2f (0, 1);
    glVertex2f (0, 32);
    glTexCoord2f (1, 1);
    glVertex2f (16, 32);
    glTexCoord2f (1, 0);
    glVertex2f (16, 0);
    glEnd();
    glEndList();

    gettimeofday (&mInitTime, 0);
    gettimeofday (&mLastRedraw, 0);
}

BenchScreen::~BenchScreen ()
{
    glDeleteLists (mDList, 2);

    glDeleteTextures (10, mNumTex);
    glDeleteTextures (1, &mBackTex);
}

bool
BenchScreen::initiate (CompOption::Vector &options)
{
    mActive = !mActive;
    mActive &= optionGetOutputScreen () || optionGetOutputConsole ();

    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "root");

    if (xid != ::screen->root ())
	return false;

    cScreen->preparePaintSetEnabled (this, mActive);
    cScreen->donePaintSetEnabled (this, mActive);
    gScreen->glPaintOutputSetEnabled (this, mActive);

    cScreen->damageScreen ();
    mCtime = 0;
    mFrames = 0;

    return false;
}

bool
BenchPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}

