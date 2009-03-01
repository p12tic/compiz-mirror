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
 */

#include <core/core.h>
#include <core/privatehandler.h>

#include "maximumize_options.h"

/* Convenience macros to make the code more readable (hopefully) */
#define REDUCE -1
#define INCREASE 1

typedef struct
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
	typedef enum {
	    X1,
	    X2,
	    Y1,
	    Y2
	} Corner;

	bool
	substantialOverlap (const CompRect& a,
			    const CompRect& b);

	CompRect
	getWindowBox (CompWindow *w);

	CompRegion
	findEmptyRegion (CompWindow      *window,
			 const CompRect& output);

	bool
	boxCompare (const CompRect& a,
		    const CompRect& b);

	void
	growGeneric (CompWindow        *w,
		     CompRect&         tmp,
		     const CompRegion& r,
		     Corner            corner,
		     const short       inc);

	void
	addToCorner (CompRect&   rect,
		     Corner      corner,
		     const short inc);

	void
	growWidth (CompWindow        *w,
		   CompRect&         tmp,
		   const CompRegion& r,
		   const MaxSet&     mset);

	void
	growHeight (CompWindow         *w,
		    CompRect&          tmp,
		    const CompRegion&  r,
		    const MaxSet&      mset);

	CompRect
	extendBox (CompWindow        *w,
		   const CompRect&   tmp,
		   const CompRegion& r,
		   bool	             xFirst,
		   const MaxSet&     mset);

	void
	setBoxWidth (CompRect&     box,
		     const int     width,
		     const MaxSet& mset);

	void
	setBoxHeight (CompRect&      box,
		      const int      height,
		      const MaxSet&  mset);

	CompRect
	minimumize (CompWindow      *w,
		    const CompRect& box,
		    const MaxSet&   mset);

	CompRect
	findRect (CompWindow        *w,
		  const CompRegion& r,
		  const MaxSet&     mset);

	unsigned int
	computeResize (CompWindow     *w,
		       XWindowChanges *xwc,
		       const MaxSet&  mset);
};

#define MAX_SCREEN(s)							       \
    MaximumizeScreen *ms = MaximumizeScreen::get (s);

class MaximumizePluginVTable :
    public CompPlugin::VTableForScreen <MaximumizeScreen>
{
    public:

	bool init ();

	PLUGIN_OPTION_HELPER (MaximumizeScreen);
};
/* Inline */

inline void
MaximumizeScreen::addToCorner (CompRect&   rect,
			       Corner      corner,
			       const short inc)
{
    int x1 = rect.x1 (), y1 = rect.y1 ();
    int x2 = rect.x2 (), y2 = rect.y2 ();

    switch (corner) {
	case X1:
	    x1 += inc;
	    break;
	case X2:
	    x2 += inc;
	    break;
	case Y1:
	    y1 += inc;
	    break;
	case Y2:
	    y2 += inc;
	    break;
    }

    rect.setGeometry (x1, x2, y1, y2);
}

/* Grow a box in the left/right directions as much as possible without
 * overlapping other relevant windows.  */
inline void
MaximumizeScreen::growWidth (CompWindow        *w,
			     CompRect&         tmp,
			     const CompRegion& r,
			     const MaxSet&     mset)
{
    if (mset.left)
	growGeneric (w, tmp, r, X1, REDUCE);

    if (mset.right)
	growGeneric (w, tmp, r, X2, INCREASE);
}

/* Grow box in the up/down direction as much as possible without
 * overlapping other relevant windows. */
inline void 
MaximumizeScreen::growHeight (CompWindow        *w,
			      CompRect&         tmp,
			      const CompRegion& r,
			      const MaxSet&     mset)
{
    if (mset.down)
	growGeneric (w, tmp, r, Y2, INCREASE);
    
    if (mset.up)
	growGeneric (w, tmp, r, Y1, REDUCE);
}



