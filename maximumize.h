/*
 * Compiz Fusion Maximumize plugin
 *
 * Copyright 2007-2008 Kristian Lyngstøl <kristian@bohemians.org>
 * Copyright 2008 Eduardo Gurgel Pinho <edgurgel@gmail.com>
 * Copyright 2008 Marco Diego Aurelio Mesquita <marcodiegomesquita@gmail.com>
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
 * Author(s):
 * Kristian Lyngstøl <kristian@bohemians.org>
 * Eduardo Gurgel <edgurgel@gmail.com>
 * Marco Diego Aurélio Mesquita <marcodiegomesquita@gmail.com>
 *
 * Description:
 *
 * Maximumize resizes a window so it fills as much of the free space in any
 * direction as possible without overlapping with other windows.
 *
 * TODO:
 * - CHECKREC should use CompRegion::contains instead of XRectInRegion
 * - Update Description
 * - Undo
 */

#include <core/core.h>
#include <core/privatehandler.h>

#include "maximumize_options.h"

/* Convenience macros to make the code more readable (hopefully) */
#define REDUCE -1
#define INCREASE 1

typedef struct _maximumizeSettings
{
    bool    left;
    bool    right;
    bool    up;
    bool    down;
    bool    shrink; // Shrink and grow can both be executed.
    bool    grow;   // Shrink will run first.
} MaxSet;

class MaximumizeScreen :
    public PrivateHandler <MaximumizeScreen, CompScreen>,
    public MaximumizeOptions
{
    public:

	MaximumizeScreen (CompScreen *);

	bool
	triggerGeneral (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options,
			bool		   grow);

	bool
	triggerDirection (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options,
			  bool		     left,
			  bool		     right,
			  bool		     up,
			  bool		     down,
			  bool		     grow);
    private:
	bool
	substantialOverlap (CompRect a,
			    CompRect b);
	CompRect
	setWindowBox (CompWindow *w);

	CompRegion
	findEmptyRegion (CompWindow *window,
			 CompRegion region);

	bool
	boxCompare (BOX a,
		    BOX b);
	void
	growGeneric (CompWindow      *w,
		     BOX	     *tmp,
		     CompRegion      r,
		     short int       *i,
		     const short int inc);
	inline void 
	growWidth (CompWindow   *w,
		   BOX          *tmp,
		   CompRegion   r,
		   const MaxSet mset);
	inline void 
	growHeight (CompWindow   *w,
		    BOX          *tmp,
		    CompRegion   r,
		    const MaxSet mset);

	BOX
	extendBox (CompWindow   *w,
		   BOX	        tmp,
		   CompRegion       r,
		   bool	        xFirst,
		   const MaxSet mset);

	void
	setBoxWidth (BOX          *box,
		     const int    width,
		     const MaxSet mset);

	void
	setBoxHeight (BOX	     *box,
		      const int      height,
		      const MaxSet   mset);
	void
	unmaximizeWindow (CompWindow *w);

	BOX
	minimumize (CompWindow *w,
		    BOX	       box,
		    MaxSet     mset);

	BOX
	findRect (CompWindow  *w,
		  CompRegion  r,
		  MaxSet      mset);

	unsigned int
	computeResize (CompWindow     *w,
		       XWindowChanges *xwc,
		       MaxSet	      mset);



};
/* Grow a box in the left/right directions as much as possible without
 * overlapping other relevant windows.  */
void 
MaximumizeScreen::growWidth (CompWindow   *w,
			     BOX          *tmp,
			     CompRegion   r,
			     const MaxSet mset)
{
    if (mset.left) 
	growGeneric (w, tmp, r, &tmp->x1, REDUCE);

    if (mset.right) 
	growGeneric (w, tmp, r, &tmp->x2, INCREASE);
}

/* Grow box in the up/down direction as much as possible without
 * overlapping other relevant windows. */
void 
MaximumizeScreen::growHeight (CompWindow   *w,
			      BOX          *tmp,
			      CompRegion   r,
			      const MaxSet mset)
{
    if (mset.down) 
	growGeneric (w, tmp, r, &tmp->y2, INCREASE);
    
    if (mset.up)
	growGeneric (w, tmp, r, &tmp->y1, REDUCE);
}

#define MAX_SCREEN(s)							       \
    MaximumizeScreen *ms = MaximumizeScreen::get (s);

class MaximumizePluginVTable :
    public CompPlugin::VTableForScreen <MaximumizeScreen>
{
    public:

	bool init ();

	PLUGIN_OPTION_HELPER (MaximumizeScreen);
};



