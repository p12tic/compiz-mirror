#ifndef RESIZELOGIC_H
#define RESIZELOGIC_H

#include <boost/shared_ptr.hpp>
#include <core/propertywriter.h>

#include <X11/Xlib.h>
#include <X11/Xregion.h>

#include "resize-defs.h"

class ResizeOptions;
class CompositeScreen;
class GLScreen;

namespace resize
{
    class CompWindowInterface;
    class CompScreenInterface;
    class CompositeScreenInterface;
    class GLScreenInterface;
    class PropertyWriterInterface;
}

class ResizeLogic
{
    public:
	ResizeLogic();
	virtual ~ResizeLogic();

	void handleEvent (XEvent *event);

	void getPaintRectangle (BoxPtr pBox);
	void getStretchRectangle (BoxPtr pBox);

	void finishResizing ();

	Cursor cursorFromResizeMask (unsigned int mask);

	void damageRectangle (BoxPtr pBox);

	bool initiateResize (CompAction		*action,
			     CompAction::State	state,
			     CompOption::Vector	&options,
			     unsigned int	mode);

	bool terminateResize (CompAction	    *action,
			      CompAction::State	    state,
			      CompOption::Vector    &options);

	bool initiateResizeDefaultMode (CompAction		*action,
					CompAction::State	state,
					CompOption::Vector	&options);

	resize::CompScreenInterface *mScreen;

	struct _ResizeKeys {
	    const char	 *name;
	    int		 dx;
	    int		 dy;
	    unsigned int warpMask;
	    unsigned int resizeMask;
	} rKeys[NUM_KEYS];

	Atom	       resizeNotifyAtom;
	resize::PropertyWriterInterface *resizeInformationAtom;

	resize::CompWindowInterface *w;
	int			    mode;
	bool			    centered;
	XRectangle		    savedGeometry;
	XRectangle		    geometry;

        /* geometry without the vertical maximization.
           Its value is undefined when maximized_vertically == false */
	XRectangle  geometryWithoutVertMax;
        bool	    maximized_vertically;

	int		 outlineMask;
	int		 rectangleMask;
	int		 stretchMask;
	int		 centeredMask;

	int          releaseButton;
	unsigned int mask;
	int          pointerDx;
	int          pointerDy;
	KeyCode      key[NUM_KEYS];

	CompScreen::GrabHandle grabIndex;

	Cursor leftCursor;
	Cursor rightCursor;
	Cursor upCursor;
	Cursor upLeftCursor;
	Cursor upRightCursor;
	Cursor downCursor;
	Cursor downLeftCursor;
	Cursor downRightCursor;
	Cursor middleCursor;
	Cursor cursor[NUM_KEYS];

	bool       isConstrained;
	CompRegion constraintRegion;
	bool       inRegionStatus;
	int        lastGoodHotSpotY;
	CompSize   lastGoodSize;

	bool offWorkAreaConstrained;
	boost::shared_ptr <CompRect> grabWindowWorkArea;

	ResizeOptions *options;

	resize::CompositeScreenInterface *cScreen;
	resize::GLScreenInterface        *gScreen;

    private:
	void handleKeyEvent (KeyCode keycode);
	void handleMotionEvent (int xRoot, int yRoot);

	void sendResizeNotify ();
	void updateWindowSize ();
	void updateWindowProperty ();

	/* Helper functions for handleMotionEvent() */
	void snapWindowToWorkAreaBoundaries (int &wi, int &he,
					     int &wX, int &wY,
					     int &wWidth, int &wHeight);
	void setUpMask (int xRoot, int yRoot);
	void accumulatePointerMotion (int xRoot, int yRoot);
	void constrainToWorkArea (int &che, int &cwi);
	void limitMovementToConstraintRegion (int &wi, int &he,
					      int xRoot, int yRoot,
					      int wX, int wY,
					      int wWidth, int wHeight);
	void computeWindowPlusBordersRect (int &wX, int &wY,
					   int &wWidth, int &wHeight,
					   int wi, int he);
	void enableOrDisableVerticalMaximization (int yRoot);
	void computeGeometry (int wi, int he);


	int getOutputForEdge (int windowOutput,
			      unsigned int touch,
			      bool skipFirst);
	unsigned int findTouchingOutput (int touchPoint,
					 unsigned int side);
	void getPointForTp (unsigned int tp,
			    unsigned int output,
			    int		&op,
			    int		&wap);
};

#endif /* RESIZELOGIC_H */
