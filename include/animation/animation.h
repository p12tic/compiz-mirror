#ifndef _ANIMATION_H
#define _ANIMATION_H

#define ANIMATION_ABI 20091205

#include <core/pluginclasshandler.h>
#include <opengl/fragment.h>

typedef enum
{
    WindowEventOpen = 0,
    WindowEventClose,
    WindowEventMinimize,
    WindowEventUnminimize,
    WindowEventShade,
    WindowEventUnshade,
    WindowEventFocus,
    WindowEventNum,
    WindowEventNone
} WindowEvent;

typedef enum
{
    AnimEventOpen = 0,
    AnimEventClose,
    AnimEventMinimize,
    AnimEventShade,
    AnimEventFocus,
    AnimEventNum
} AnimEvent;

typedef enum
{
    AnimDirectionDown = 0,
    AnimDirectionUp,
    AnimDirectionLeft,
    AnimDirectionRight,
    AnimDirectionRandom,
    AnimDirectionAuto
} AnimDirection;
#define LAST_ANIM_DIRECTION 5

class PrivateAnimScreen;
class PrivateAnimWindow;
class Animation;
class AnimWindow;
class AnimEffectInfo;

typedef AnimEffectInfo * AnimEffect;

typedef Animation *(*CreateAnimFunc) (CompWindow *w,
				      WindowEvent curWindowEvent,
				      float duration,
				      const AnimEffect info,
				      const CompRect &icon);

/// Animation info class that holds the name, the list of supported events, and
/// the creator function for a subclass of Animation.
/// A pointer to this class is used as an identifier for each implemented
/// animation.
class AnimEffectInfo
{
public:
    AnimEffectInfo (const char *name,
		    bool usedO, bool usedC, bool usedM, bool usedS, bool usedF,
		    CreateAnimFunc create, bool isRestackAnim = false);
    ~AnimEffectInfo () {}

    bool matchesEffectName (const CompString &animName);

    bool matchesPluginName (const CompString &pluginName);

    const char *name; ///< Name of the animation effect, e.g. "animationpack:Implode".

    /// To be set to true for the window event animation list(s) that
    /// the new animation (value) should be added to
    /// (0: open, 1: close, 2: minimize, 3: shade, 4: focus)
    bool usedForEvents[AnimEventNum];

    /// Creates an instance of the Animation subclass and returns it as an
    /// Animation instance.
    CreateAnimFunc create;

    /// Is it a complex focus animation? (i.e. restacking-related,
    /// like FocusFade/Dodge)
    bool isRestackAnim;
};

template<class T>
Animation *createAnimation (CompWindow *w,
			    WindowEvent curWindowEvent,
			    float duration,
			    const AnimEffect info,
			    const CompRect &icon)
{
    return new T (w, curWindowEvent, duration, info, icon);
}

class ExtensionPluginInfo
{
public:
    ExtensionPluginInfo (const CompString &name,
			 unsigned int nEffects,
			 AnimEffect *effects,
			 CompOption::Vector *effectOptions,
			 unsigned int firstEffectOptionIndex);

    CompString name;
    unsigned int nEffects;
    AnimEffect *effects;

    /// Plugin options to be used in "effect options" strings.
    CompOption::Vector *effectOptions;

    /// Index of first effect option.
    unsigned int firstEffectOptionIndex;

    // More general and/or non-window functions (including functions that access
    // persistent animation data) to be overriden

    /// To be run at the beginning of glPaintOutput.
    virtual void prePaintOutput (CompOutput *output) {}

    /// To be run at the beginning of preparePaint.
    virtual void prePreparePaintGeneral () {}

    /// To be run at the end of preparePaint.
    virtual void postPreparePaintGeneral () {}

    /// To be run when a CompWindowNotifyRestack is handled.
    virtual void handleRestackNotify (AnimWindow *aw) {}

    /// To be run at the beginning of initiateOpenAnim.
    virtual void preInitiateOpenAnim (AnimWindow *aw) {}

    /// To be run at the beginning of initiateCloseAnim.
    virtual void preInitiateCloseAnim (AnimWindow *aw) {}

    /// To be run at the beginning of initiateMinimizeAnim.
    virtual void preInitiateMinimizeAnim (AnimWindow *aw) {}

    /// To be run at the beginning of initiateUnminimizeAnim.
    virtual void preInitiateUnminimizeAnim (AnimWindow *aw) {}

    /// Initializes plugin's persistent animation data for a window (if any).
    virtual void initPersistentData (AnimWindow *aw) {}

    /// Destroys plugin's persistent animation data for a window (if any).
    virtual void destroyPersistentData (AnimWindow *aw) {}

    /// To be run at the end of updateEventEffects.
    virtual void postUpdateEventEffects (AnimEvent e,
					 bool forRandom) {}

    /// To be run after the startup countdown ends.
    virtual void postStartupCountdown () {}

    virtual bool paintShouldSkipWindow (CompWindow *w) { return false; }

    virtual void cleanUpAnimation (bool closing,
				   bool destructing) {}

    virtual void processAllRestacks () {}
};

class Point
{
    public:
	Point () : mX (0), mY (0) {}
	Point (float x, float y) : mX (x), mY (y) {}

	inline float x () const { return mX; }
	inline float y () const { return mY; }

	inline void setX (float x) { mX = x; }
	inline void setY (float y) { mY = y; }

	void set (float x, float y) { mX = x; mY = y; }

	inline void add (const Point &p) { mX += p.x (); mY += p.y (); }

	Point &operator= (const Point &p);
	bool operator== (const Point &p) const;
	bool operator!= (const Point &p) const;

    private:
	float mX, mY;
};

typedef Point Vector;


class Point3d
{
    public:
	Point3d () : mX (0), mY (0), mZ (0) {}
	Point3d (float x, float y, float z) : mX (x), mY (y), mZ (z) {}

	inline float x () const { return mX; }
	inline float y () const { return mY; }
	inline float z () const { return mZ; }

	inline void setX (float x) { mX = x; }
	inline void setY (float y) { mY = y; }
	inline void setZ (float z) { mZ = z; }

	inline void set (float x, float y, float z) { mX = x; mY = y; mZ = z; }

	inline void add (const Point3d &p)
	{ mX += p.x (); mY += p.y (); mZ += p.z (); }
	inline void add (float x, float y, float z)
	{ mX += x; mY += y; mZ += z; }

	Point3d &operator= (const Point3d &p);
	bool operator== (const Point3d &p) const;
	bool operator!= (const Point3d &p) const;

    private:
	float mX, mY, mZ;
};

typedef Point3d Vector3d;

/* XXX: change this to CompRect */
typedef struct
{
    float x1, x2, y1, y2;
} Boxf;


/** The base class for all animations.
Animations should derive from the closest animation class
to override as few methods as possible. Also, an animation
method should call ancestors' methods instead of duplicating
their code.
*/
class Animation
{
protected:
    CompWindow *mWindow;
    AnimWindow *mAWindow;

    float mTotalTime;
    float mRemainingTime;
    float mTimestep;		///< to store anim. timestep at anim. start
    float mTimeElapsedWithinTimeStep;

    int mTimeSinceLastPaint;    ///< in milliseconds

    int mOverrideProgressDir;	///< 0: default dir, 1: forward, 2: backward

    GLFragment::Attrib mCurPaintAttrib;
    GLushort mStoredOpacity;
    WindowEvent mCurWindowEvent;
    bool mInitialized; ///< whether the animation is initialized (in preparePaint)

    AnimEffect mInfo; ///< information about the animation class

    CompRect mIcon;

    int mDecorTopHeight;
    int mDecorBottomHeight;

    GLTexture::List *texturesCache;

    CompOption::Value &optVal (unsigned int optionId);

    inline bool       optValB (unsigned int optionId) { return optVal (optionId).b (); }
    inline int        optValI (unsigned int optionId) { return optVal (optionId).i (); }
    inline float      optValF (unsigned int optionId) { return optVal (optionId).f (); }
    inline CompString optValS (unsigned int optionId) { return optVal (optionId).s (); }
    inline unsigned short *optValC (unsigned int optionId) { return optVal (optionId).c (); }

public:

    Animation (CompWindow *w,
	       WindowEvent curWindowEvent,
	       float duration,
	       const AnimEffect info,
	       const CompRect &icon);
    virtual ~Animation ();

    inline AnimEffect info () { return mInfo; }

    // Overridable animation methods.

    /// Needed since virtual method calls can't be done in the constructor.
    virtual void init () {}

    /// To be called during post-animation clean up.
    virtual void cleanUp (bool closing,
			  bool destructing) {}

    /// Returns true if frame should be skipped (e.g. due to
    /// higher timestep values). In that case no drawing is
    /// needed for that window in current frame.
    virtual bool shouldSkipFrame (int msSinceLastPaintActual);

    /// Advances the animation time by the given time amount.
    /// Returns true if more animation time is left.
    virtual bool advanceTime (int msSinceLastPaint);

    /// Computes new animation state based on remaining time.
    virtual void step () {}
    virtual void updateAttrib (GLWindowPaintAttrib &) {}
    virtual void updateTransform (GLMatrix &) {}
    virtual void prePaintWindow () {}
    virtual void postPaintWindow () {}
    virtual bool postPaintWindowUsed () { return false; }

    /// Returns true if the animation is still in progress.
    virtual bool prePreparePaint (int msSinceLastPaint) { return false; }
    virtual void postPreparePaint () {}

    /// Updates the bounding box of damaged region. Should be implemented for
    /// any animation that doesn't update the whole screen.
    virtual void updateBB (CompOutput &) {}
    virtual bool updateBBUsed () { return false; }

    /// Should return true for effects that make use of a region
    /// instead of just a bounding box for damaged area.
    virtual bool stepRegionUsed () { return false; }

    virtual bool shouldDamageWindowOnStart ();
    virtual bool shouldDamageWindowOnEnd ();

    /// Should return false if the animation should be stopped on move
    virtual bool moveUpdate (int dx, int dy) { return true; }

    /// Should return false if the animation should be stopped on resize
    virtual bool resizeUpdate (int dx, int dy,
                               int dwidth, int dheight) { return true; }

    virtual void adjustPointerIconSize () {}
    virtual void addGeometry (const GLTexture::MatrixList &matrix,
			      const CompRegion            &region,
			      const CompRegion            &clip,
			      unsigned int                maxGridWidth,
			      unsigned int                maxGridHeight);
    virtual void drawGeometry ();

    /// Gets info about the (extension) plugin that implements this animation.
    /// Should be overriden by a base animation class in every extension plugin.
    virtual ExtensionPluginInfo *getExtensionPluginInfo ();

    void drawTexture (GLTexture          *texture,
		      GLFragment::Attrib &attrib,
		      unsigned int       mask);

    // Utility methods

    void reverse ();
    inline bool inProgress () { return (mRemainingTime > 0); }

    inline WindowEvent curWindowEvent () { return mCurWindowEvent; }
    inline float totalTime () { return mTotalTime; }
    inline float remainingTime () { return mRemainingTime; }

    float progressLinear ();
    float progressEaseInEaseOut ();
    float progressDecelerateCustom (float progress,
				    float minx, float maxx);
    float progressDecelerate (float progress);
    AnimDirection getActualAnimDirection (AnimDirection dir,
					  bool openDir);
    void perspectiveDistortAndResetZ (GLMatrix &transform);

    static void prepareTransform (CompOutput &output,
				  GLMatrix &resultTransform,
				  GLMatrix &transform);
    void setInitialized () { mInitialized = true; }
    inline bool initialized () { return mInitialized; }
    inline void setCurPaintAttrib (GLFragment::Attrib &newAttrib)
    { mCurPaintAttrib = newAttrib; }
};

class GridAnim :
    virtual public Animation
{
public:
    class GridModel
    {
	friend class GridAnim;

    public:
	GridModel (CompWindow *w,
		   WindowEvent curWindowEvent,
		   int height,
		   int gridWidth,
		   int gridHeight,
		   int decorTopHeight,
		   int decorBottomHeight);
	~GridModel ();

	void move (float tx, float ty);

	class GridObject
	{
	    friend class GridAnim;
	    friend class GridZoomAnim;
	    friend class GridTransformAnim;

	public:
	    GridObject ();
	    void setGridPosition (Point &gridPosition);
	    inline Point3d &position () { return mPosition; }
	    inline Point &gridPosition () { return mGridPosition; }

	    inline Point &offsetTexCoordForQuadBefore ()
	    { return mOffsetTexCoordForQuadBefore; }

	    inline Point &offsetTexCoordForQuadAfter ()
	    { return mOffsetTexCoordForQuadAfter; }

	private:
	    Point3d mPosition;	 ///< Position on screen
	    Point mGridPosition; ///< Position on window in [0,1] range

	    Point mOffsetTexCoordForQuadBefore;
	    Point mOffsetTexCoordForQuadAfter;
	    ///< Texture x, y coordinates will be offset by given amounts
	    ///< for quads that fall after and before this object in x and y directions.
	    ///< Currently only y offset can be used.
	};

	inline GridObject *objects () { return mObjects; }
	inline unsigned int numObjects () { return mNumObjects; }
	inline Point &scale () { return mScale; }

    private:
	GridObject *mObjects; // TODO: convert to vector
	unsigned int mNumObjects;

	Point mScale;
	Point mScaleOrigin;

	void initObjects (WindowEvent curWindowEvent,
			  int height,
			  int gridWidth, int gridHeight,
			  int decorTopHeight, int decorBottomHeight);
    };

protected:
    GridModel *mModel;

    int mGridWidth;	///< Number of cells along grid width
    int mGridHeight;	///< Number of cells along grid height

    /// true if effect needs Q texture coordinates.
    /// Q texture coordinates are used to avoid jagged-looking quads
    /// ( http://www.r3.nu/~cass/qcoord/ )
    bool mUseQTexCoord;

    GLWindow::Geometry mGeometry; ///< geometry for grid mesh

    void drawGeometry ();

    virtual bool using3D () { return false; }

    virtual void initGrid ();	///< Initializes grid width/height.
    				///< Default grid size is 2x2.
				///< Override for custom grid size.

public:
    GridAnim (CompWindow *w,
	      WindowEvent curWindowEvent,
	      float duration,
	      const AnimEffect info,
	      const CompRect &icon);
    ~GridAnim ();
    void init ();
    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }
    void addGeometry (const GLTexture::MatrixList &matrix,
		      const CompRegion            &region,
		      const CompRegion            &clip,
		      unsigned int                maxGridWidth,
		      unsigned int                maxGridHeight);
};

class TransformAnim :
    virtual public Animation
{
public:
    TransformAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float duration,
		   const AnimEffect info,
		   const CompRect &icon);
    void init ();
    void step ();
    void updateTransform (GLMatrix &wTransform);
    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }

protected:
    GLMatrix mTransform;

    float mTransformStartProgress;
    float mTransformProgress;

    void perspectiveDistortAndResetZ (GLMatrix &transform);
    void applyPerspectiveSkew (CompOutput &output,
			       GLMatrix &transform,
			       Point &center);
    virtual void adjustDuration () {}
    virtual void applyTransform () {}
    virtual Point getCenter ();
};

class GridTransformAnim :
    public GridAnim,
    virtual public TransformAnim
{
public:
    GridTransformAnim (CompWindow *w,
		       WindowEvent curWindowEvent,
		       float duration,
		       const AnimEffect info,
		       const CompRect &icon);
protected:
    void init ();
    void updateTransform (GLMatrix &wTransform);
    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }

    bool mUsingTransform; ///< whether transform matrix is used (default: true)
};

class PartialWindowAnim :
    virtual public Animation
{
public:
    PartialWindowAnim (CompWindow *w,
		       WindowEvent curWindowEvent,
		       float duration,
		       const AnimEffect info,
		       const CompRect &icon);

    void addGeometry (const GLTexture::MatrixList &matrix,
		      const CompRegion            &region,
		      const CompRegion            &clip,
		      unsigned int                maxGridWidth,
		      unsigned int                maxGridHeight);

protected:
    bool mUseDrawRegion;
    CompRegion mDrawRegion;
};

class FadeAnim :
    virtual public Animation
{
public:
    FadeAnim (CompWindow *w,
		WindowEvent curWindowEvent,
		float duration,
		const AnimEffect info,
		const CompRect &icon);
protected:
    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }
    void updateAttrib (GLWindowPaintAttrib &wAttrib);
    virtual float getFadeProgress () { return progressLinear (); }
};

class ZoomAnim :
    public FadeAnim,
    virtual public TransformAnim
{
public:
    ZoomAnim (CompWindow *w,
	      WindowEvent curWindowEvent,
	      float duration,
	      const AnimEffect info,
	      const CompRect &icon);

protected:
    void step () { TransformAnim::step (); }
    void adjustDuration ();
    float getFadeProgress ();
    bool updateBBUsed () { return true; }
    void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
    void applyTransform ();

    float getActualProgress ();
    Point getCenter ();
    virtual float getSpringiness ();
    virtual bool isZoomFromCenter ();
    virtual bool zoomToIcon () { return true; }
    virtual bool hasExtraTransform () { return false; }
    virtual void applyExtraTransform (float progress) {}
    virtual bool shouldAvoidParallelogramLook () { return false; }
    virtual bool scaleAroundIcon ();
    virtual bool neverSpringy () { return false; }
    void getZoomProgress (float *moveProgress,
			  float *scaleProgress,
			  bool neverSpringy);

    static const float kDurationFactor;
    static const float kSpringyDurationFactor;
    static const float kNonspringyDurationFactor;

private:
    void getCenterScaleFull (Point *pCurCenter, Point *pCurScale,
			     Point *pWinCenter, Point *pIconCenter,
			     float *pMoveProgress);
    void getCenterScale (Point *pCurCenter, Point *pCurScale);
};

class GridZoomAnim :
    public GridTransformAnim,
    public ZoomAnim
{
public:
    GridZoomAnim (CompWindow *w,
		  WindowEvent curWindowEvent,
		  float duration,
		  const AnimEffect info,
		  const CompRect &icon);
    void init () { GridTransformAnim::init (); }
    void step () { ZoomAnim::step (); }
    void updateTransform (GLMatrix &wTransform)
    { GridTransformAnim::updateTransform (wTransform); }
    void updateBB (CompOutput &output) { GridTransformAnim::updateBB (output); }
    bool updateBBUsed () { return true; }
    bool neverSpringy () { return true; }
    float getSpringiness () { return 0; }
    bool scaleAroundIcon () { return false; }
    void adjustDuration ();
};

/*
class ParticleAnim : // TODO: move to animationaddon
    public PartialWindowAnim
{
};

class PolygonAnim : // TODO: move to animationaddon
    public Animation
{
protected:
    //void *extraProperties;
};
*/


class AnimScreen :
    public PluginClassHandler<AnimScreen, CompScreen, ANIMATION_ABI>,
    public CompOption::Class
{
    friend class ExtensionPluginAnimation;
    friend class PrivateAnimScreen;
    friend class PrivateAnimWindow;

public:
    AnimScreen (CompScreen *);
    ~AnimScreen ();

    void addExtension (ExtensionPluginInfo *extensionPluginInfo);
    void removeExtension (ExtensionPluginInfo *extensionPluginInfo);
    bool getMousePointerXY (short *x, short *y);
    CompOption::Vector &getOptions ();
    bool setOption (const CompString &name, CompOption::Value &value);
    CompOutput &output ();
    AnimEffect getMatchingAnimSelection (CompWindow *w,
					 AnimEvent e,
					 int *duration);
    void enableCustomPaintList (bool enabled);
    bool isRestackAnimPossible ();
    bool isAnimEffectPossible (AnimEffect theEffect);
    bool otherPluginsActive ();
    bool initiateFocusAnim (AnimWindow *aw);

private:
    PrivateAnimScreen *priv;
};

class PersistentData
{
};

typedef std::map<std::string, PersistentData *> PersistentDataMap;

class AnimWindow :
    public PluginClassHandler<AnimWindow, CompWindow, ANIMATION_ABI>
{
    friend class PrivateAnimScreen;
    friend class PrivateAnimWindow;
    friend class AnimScreen;
    friend class Animation;

public:
    AnimWindow (CompWindow *);
    ~AnimWindow ();

    BoxPtr BB ();
    CompRegion &stepRegion ();
    void resetStepRegionWithBB ();

    void expandBBWithWindow ();
    void expandBBWithScreen ();
    void expandBBWithBox (Box &source);
    void expandBBWithPoint (float fx, float fy);
    void expandBBWithPoint2DTransform (GLVector &coords,
				       GLMatrix &transformMat);
    bool expandBBWithPoints3DTransform (CompOutput     &output,
					GLMatrix       &transform,
					const float    *points,
					GridAnim::GridModel::GridObject *objects,
					unsigned int   nPoints);

    inline bool savedRectsValid () { return mSavedRectsValid; }
    inline CompRect & saveWinRect () { return mSavedWinRect; }
    inline CompRect & savedInRect () { return mSavedInRect; }
    inline CompRect & savedOutRect () { return mSavedOutRect; }
    inline CompWindowExtents & savedOutExtents () { return mSavedOutExtents; }

    Animation *curAnimation ();
    void createFocusAnimation (AnimEffect effect, int duration = 0);

    void postAnimationCleanUp ();

    // TODO: Group persistent data for a plugin and allow a plugin to only
    // delete its own data.
    void deletePersistentData (const char *name);

    /// A "string -> persistent data" map for animations that require such data,
    /// like some focus animations.
    PersistentDataMap persistentData;

    CompWindow *mWindow;    ///< Window being animated. // TODO move to private:
private:
    PrivateAnimWindow *priv;


    bool mSavedRectsValid;
    CompRect mSavedWinRect; ///< Saved window contents geometry
    CompRect mSavedInRect;   ///< Saved window input geometry
    CompRect mSavedOutRect;  ///< Saved window output geometry
    CompWindowExtents mSavedOutExtents; ///< Saved window output extents

    CompOption::Value &pluginOptVal (ExtensionPluginInfo *pluginInfo,
				     unsigned int optionId,
				     Animation *anim);
};


inline Point &
Point::operator= (const Point &p)
{
    mX = p.x (); mY = p.y ();
    return *this;
}

inline bool
Point::operator== (const Point &p) const
{
    return (mX == p.x () && mY == p.y ());
}

inline bool
Point::operator!= (const Point &p) const
{
    return !(*this == p);
}

inline Point3d &
Point3d::operator= (const Point3d &p)
{
    mX = p.x (); mY = p.y (); mZ = p.z ();
    return *this;
}

inline bool
Point3d::operator== (const Point3d &p) const
{
    return (mX == p.x () && mY == p.y () && mZ == p.z ());
}

inline bool
Point3d::operator!= (const Point3d &p) const
{
    return !(*this == p);
}


#define RAND_FLOAT() ((float)rand() / RAND_MAX)

#define sigmoid(fx) (1.0f/(1.0f+exp(-5.0f*2*((fx)-0.5))))
#define sigmoid2(fx, s) (1.0f/(1.0f+exp(-(s)*2*((fx)-0.5))))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))


// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#endif

