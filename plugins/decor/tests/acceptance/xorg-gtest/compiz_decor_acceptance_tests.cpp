/*
 * Compiz XOrg GTest Decoration Acceptance Tests
 *
 * Copyright (C) 2013 Sam Spilsbury.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authored By:
 * Sam Spilsbury <smspillaz@gmail.com>
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sstream>
#include <vector>

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/bind.hpp>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "decoration.h"

#include <core/point.h>
#include <core/size.h>

#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

namespace xt = xorg::testing;
namespace ct = compiz::testing;

using ::testing::AtLeast;
using ::testing::ReturnNull;
using ::testing::Return;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MakeMatcher;
using ::testing::StrEq;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;
using ::testing::_;

class BaseDecorAcceptance :
    public ct::AutostartCompizXorgSystemTestWithTestHelper
{
    public:

	BaseDecorAcceptance ();

	virtual void SetUp ();
	virtual ct::CompizProcess::PluginList GetPluginList ();

    protected:

	Window mRootWindow;

	Atom   mUtf8StringAtom;
	Atom   mDecorationManagerNameAtom;
	Atom   mDecorationTypeAtom;
	Atom   mDecorationTypePixmap;
	Atom   mDecorationTypeWindow;
	Atom   mDecorationInputFrameAtom;
	Atom   mDecorationOutputFrameAtom;
};

BaseDecorAcceptance::BaseDecorAcceptance () :
    mRootWindow (0),
    mUtf8StringAtom (0),
    mDecorationManagerNameAtom (0),
    mDecorationTypeAtom (0),
    mDecorationTypePixmap (0),
    mDecorationTypeWindow (0),
    mDecorationInputFrameAtom (0),
    mDecorationOutputFrameAtom (0)
{
}

void
BaseDecorAcceptance::SetUp ()
{
    ct::AutostartCompizXorgSystemTestWithTestHelper::SetUp ();

    mUtf8StringAtom = XInternAtom (Display (),
				  "UTF8_STRING",
				  1);

    ASSERT_NE (0, mUtf8StringAtom);

    mDecorationManagerNameAtom = XInternAtom (Display (),
					     "_COMPIZ_DM_NAME",
					     0);

    ASSERT_NE (0, mDecorationManagerNameAtom);

    mDecorationTypePixmap = XInternAtom (Display (),
					 DECOR_TYPE_PIXMAP_ATOM_NAME,
					 0);

    ASSERT_NE (0, mDecorationTypePixmap);

    mDecorationTypeWindow = XInternAtom (Display (),
					 DECOR_TYPE_WINDOW_ATOM_NAME,
					 0);

    ASSERT_NE (0, mDecorationTypeWindow);

    mDecorationTypeAtom = XInternAtom (Display (),
				       DECOR_TYPE_ATOM_NAME,
				       0);

    ASSERT_NE (0, mDecorationTypeAtom);

    mDecorationInputFrameAtom = XInternAtom (Display (),
					     DECOR_INPUT_FRAME_ATOM_NAME,
					     0);

    ASSERT_NE (0, mDecorationInputFrameAtom);

    mDecorationOutputFrameAtom = XInternAtom (Display (),
					      DECOR_OUTPUT_FRAME_ATOM_NAME,
					      0);

    ASSERT_NE (0, mDecorationOutputFrameAtom);

    mRootWindow = DefaultRootWindow (Display ());

    ASSERT_NE (0, mRootWindow);
}

ct::CompizProcess::PluginList
BaseDecorAcceptance::GetPluginList ()
{
    typedef ct::AutostartCompizXorgSystemTestWithTestHelper Base;
    ct::CompizProcess::PluginList baseList (Base::GetPluginList ());
    ct::CompizProcess::PluginList list;

    list.push_back ("composite");
    list.push_back ("opengl");
    list.push_back ("decor");

    for (ct::CompizProcess::PluginList::iterator it = baseList.begin ();
	 it != baseList.end ();
	 ++it)
	list.push_back (*it);

    return list;
}

namespace compiz
{
namespace decor
{
namespace testing
{

const char *DecorationClientName = "fake";

const char *FailedToAcquireMessage =
	"Could not acquire selection for an unknown reason";
const char *OtherDecorationManagerRunningMessage =
	"Another decoration manager is already running";

struct MWMHints
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
};

static const unsigned int MwmDecor = 1L << 0;

class FakeDecorator
{
    public:

	FakeDecorator (Display *d,
		       int supporting = WINDOW_DECORATION_TYPE_WINDOW);
	~FakeDecorator ();

	Window currentSelectionOwner ();
	Atom   sessionOwnerAtom ();

	Window supportingCheckWindow ();

    private:

	Display *mDpy;
	int     mScreenNumber;
	Window  mRootWindow;
	int     mSessionStatus;
	Time    mDecorationManagerTimestamp;

	Atom    mSessionOwnerAtom;
	Window  mSessionOwner;

	Atom    mSupportingHintAtom;
	Atom    mSupportingHintWindowAtom;
	Window  mSupportingHintWindow;

};

class FakeDecoration
{
    public:

	typedef boost::shared_ptr <FakeDecoration> Ptr;

	FakeDecoration (unsigned int type,
			unsigned int state,
			unsigned int actions,
			unsigned int minWidth,
			unsigned int minHeight);

	virtual ~FakeDecoration ();

	virtual size_t propertyDataSize () const = 0;
	virtual void addPropertyData (std::vector <long> &) const = 0;

	bool match (unsigned int type,
		    unsigned int state,
		    unsigned int actions) const;

	virtual CompPoint restoredFrameWindowOffset () const = 0;
	virtual CompSize restoredDecorationSize () const = 0;

    protected:

	void insertBaseData (std::vector <long> &) const;

	static const size_t BaseDataSize = 5;

	unsigned int mType;
	unsigned int mState;
	unsigned int mActions;

	unsigned int mMinWidth;
	unsigned int mMinHeight;
};

class FakeWindowTypeDecoration :
    public FakeDecoration
{
    public:

	FakeWindowTypeDecoration (unsigned int type,
				  unsigned int state,
				  unsigned int actions,
				  unsigned int minWidth,
				  unsigned int minHeight,
				  const decor_extents_t &restored,
				  const decor_extents_t &maximized);

    protected:

	size_t propertyDataSize () const;
	void addPropertyData (std::vector<long> &) const;
	CompPoint restoredFrameWindowOffset () const;
	CompSize restoredDecorationSize () const;

	static const unsigned int WindowDecorationSize = 8;

    private:

	decor_extents_t mRestored;
	decor_extents_t mMaximized;
};

class FakePixmapTypeDecoration :
    public FakeDecoration
{
    public:

	typedef boost::shared_ptr <FakePixmapTypeDecoration> Ptr;

	FakePixmapTypeDecoration (unsigned int type,
				  unsigned int state,
				  unsigned int actions,
				  unsigned int minWidth,
				  unsigned int minHeight,
				  const decor_extents_t &restoredBorder,
				  const decor_extents_t &restoredInput,
				  const decor_extents_t &maximizedBorder,
				  const decor_extents_t &maximizedInput,
				  Display               *dpy);
	~FakePixmapTypeDecoration ();

	void changeRestoredBorder (const decor_extents_t &border);
	void changeRestoredInput (const decor_extents_t &input);

	CompPoint restoredFrameWindowOffset () const;
	CompSize restoredDecorationSize () const;

    protected:

	size_t propertyDataSize () const;
	void addPropertyData (std::vector<long> &) const;

    private:

	decor_extents_t mRestoredBorder;
	decor_extents_t mRestoredInput;
	decor_extents_t mMaximizedBorder;
	decor_extents_t mMaximizedInput;

	decor_context_t mContext;
	decor_quad_t    mQuads[N_QUADS_MAX];
	decor_layout_t  mLayout;
	unsigned int    mNQuads;

	Display         *mDpy;
	Pixmap          mPixmap;
};

class FakeDecorationList
{
    public:

	FakeDecorationList () :
	    mDecorationType (WINDOW_DECORATION_TYPE_PIXMAP)
	{
	}

	FakeDecorationList (unsigned int type) :
	    mDecorationType (type)
	{
	}

	void AddDecoration (const FakeDecoration::Ptr &);
	void RemoveDecoration (unsigned int type,
			       unsigned int state,
			       unsigned int actions);
	void RemoveAllDecorations ();

	void SetPropertyOnWindow (Display *dpy,
				  Window  w,
				  Atom    property);

    private:

	unsigned int                      mDecorationType;
	std::vector <FakeDecoration::Ptr> decorations;
};

class AcquisitionFailed :
    public std::exception
{
    public:

	AcquisitionFailed (int status);

	const char * what () const throw ();

    private:

	int mStatus;
};

}
}
}

namespace
{
bool Advance (Display *d, bool result)
{
    return ct::AdvanceToNextEventOnSuccess (d, result);
}

class ManagerMessageMatcher :
    public ct::ClientMessageXEventMatcher
{
    public:

	ManagerMessageMatcher (Display *dpy,
			       Atom managed);

	virtual bool MatchAndExplain (const XEvent &event,
				      MatchResultListener *listener) const;

	virtual void DescribeTo (std::ostream *os) const;

    private:

	Display *mDpy;
	Atom    mManager;
	Atom    mManaged;
};

void FreePropertyData (unsigned char *array)
{
    if (array)
	XFree (array);
}

int SafeFetchProperty (Display       *dpy,
		       Window        w,
		       Atom          property,
		       long          offset,
		       long          length,
		       Atom          requestedType,
		       Atom          &returnType,
		       int           &returnFormat,
		       unsigned long &returnNItems,
		       unsigned long &returnBytesRemaining,
		       boost::shared_ptr <unsigned char> &data)
{
    unsigned char *dataLocation;

    int status = XGetWindowProperty (dpy, w, property, offset, length, 0,
				     requestedType, &returnType,
				     &returnFormat, &returnNItems,
				     &returnBytesRemaining,
				     &dataLocation);

    data.reset (dataLocation,
		boost::bind (FreePropertyData, _1));

    return status;
}

void FetchAndVerifyProperty (Display       *dpy,
			     Window        w,
			     Atom          property,
			     Atom          expectedType,
			     int           expectedFormat,
			     unsigned long expectedItems,
			     unsigned long expectedBytesAfter,
			     boost::shared_ptr <unsigned char> &data)
{
    Atom          returnedType = 0;
    int           returnedFormat = 0;
    unsigned long returnedNItems = 0;
    unsigned long returnedBytesRemaining = 0;

    std::stringstream ss;

    int status = SafeFetchProperty (dpy,
				    w,
				    property,
				    0L,
				    1024L,
				    expectedType,
				    returnedType,
				    returnedFormat,
				    returnedNItems,
				    returnedBytesRemaining,
				    data);

    if (status != Success)
	ss << "XGetWindowProperty failed:" << std::endl;

    if (returnedType != expectedType)
	ss << "Expected type of " << XGetAtomName (dpy, expectedType)
	   << std::endl;

    if (returnedFormat != expectedFormat)
	ss << "Expected format of " << expectedFormat
	   << " but got " << returnedFormat << std::endl;

    if (returnedNItems != expectedItems)
	ss  << "Expected " << expectedItems << " items"
	    << " but got " << returnedNItems << " items" << std::endl;

    if (returnedBytesRemaining != expectedBytesAfter)
	ss << "Expected " << expectedBytesAfter << " bytes remaining"
	   << " but got " << returnedBytesRemaining << " bytes remaining"
	   << std::endl;

    if (!ss.str ().empty ())
	throw std::logic_error (ss.str ());
}
}

ManagerMessageMatcher::ManagerMessageMatcher (Display *dpy,
					      Atom managed) :
    ct::ClientMessageXEventMatcher (dpy,
				    XInternAtom (dpy,
						 "MANAGER",
						 1),
				    DefaultRootWindow (dpy)),
    mDpy (dpy),
    mManaged (managed)
{
}

bool
ManagerMessageMatcher::MatchAndExplain (const XEvent &event,
					MatchResultListener *listener) const
{
    if (ct::ClientMessageXEventMatcher::MatchAndExplain (event, listener))
    {
	/* Evaluate the second field for the atom */
	return event.xclient.data.l[1] ==
	    static_cast <long> (mManaged);
    }

    return false;
}

void
ManagerMessageMatcher::DescribeTo (std::ostream *os) const
{
    *os << "manager of selection atom " << XGetAtomName (mDpy, mManaged);
}



namespace cdt = compiz::decor::testing;

cdt::FakeDecoration::FakeDecoration (unsigned int type,
				     unsigned int state,
				     unsigned int actions,
				     unsigned int minWidth,
				     unsigned int minHeight) :
    mType (type),
    mState (state),
    mActions (actions),
    mMinWidth (minWidth),
    mMinHeight (minHeight)
{
}

cdt::FakeDecoration::~FakeDecoration ()
{
}

bool
cdt::FakeDecoration::match (unsigned int type,
			    unsigned int state,
			    unsigned int actions) const
{
    return type == mType &&
	   state == mState &&
	   actions == mActions;
}

cdt::FakeWindowTypeDecoration::FakeWindowTypeDecoration (unsigned int type,
							 unsigned int state,
							 unsigned int actions,
							 unsigned int minWidth,
							 unsigned int minHeight,
							 const decor_extents_t &restored,
							 const decor_extents_t &maximized) :
    FakeDecoration (type, state, actions, minWidth, minHeight),
    mRestored (restored),
    mMaximized (maximized)
{
}

void
cdt::FakeWindowTypeDecoration::addPropertyData (std::vector<long> &vec) const
{
    long propData[PROP_HEADER_SIZE + propertyDataSize ()];

    decor_gen_window_property (propData,
			       0,
			       const_cast <decor_extents_t *> (&mRestored),
			       const_cast <decor_extents_t *> (&mMaximized),
			       mMinWidth,
			       mMinHeight,
			       mState,
			       mType,
			       mActions);

    for (size_t i = PROP_HEADER_SIZE;
	 i < (PROP_HEADER_SIZE + propertyDataSize ());
	 ++i)
	vec.push_back (propData[i]);
}

size_t
cdt::FakeWindowTypeDecoration::propertyDataSize () const
{
    return WINDOW_PROP_SIZE;
}

CompPoint
cdt::FakeWindowTypeDecoration::restoredFrameWindowOffset () const
{
    int xOffset = -mRestored.left;
    int yOffset = -mRestored.top;

    return CompPoint (xOffset, yOffset);
}

CompSize
cdt::FakeWindowTypeDecoration::restoredDecorationSize () const
{
    return CompSize (mRestored.left + mRestored.right,
		     mRestored.top + mRestored.bottom);
}

cdt::FakePixmapTypeDecoration::FakePixmapTypeDecoration (unsigned int type,
							 unsigned int state,
							 unsigned int actions,
							 unsigned int minWidth,
							 unsigned int minHeight,
							 const decor_extents_t &restoredBorder,
							 const decor_extents_t &restoredInput,
							 const decor_extents_t &maximizedBorder,
							 const decor_extents_t &maximizedInput,
							 Display               *dpy) :
    FakeDecoration (type, state, actions, minWidth, minHeight),
    mRestoredBorder (restoredBorder),
    mRestoredInput (restoredInput),
    mMaximizedBorder (maximizedBorder),
    mMaximizedInput (maximizedInput),
    mDpy (dpy),
    mPixmap (XCreatePixmap (mDpy,
			    DefaultRootWindow (mDpy),
			    10,
			    10,
			    DefaultDepth (mDpy, (DefaultScreen (mDpy)))))
{
    /* 10x10 decoration, 0 corner space, 0 spacing, 2px each side */
    mContext.extents.left = 2;
    mContext.extents.right = 2;
    mContext.extents.top = 2;
    mContext.extents.bottom = 2;

    mContext.left_space = 0;
    mContext.right_space = 0;
    mContext.top_space = 0;
    mContext.bottom_space = 0;
    mContext.left_corner_space = 0;
    mContext.right_corner_space = 0;
    mContext.top_corner_space = 0;
    mContext.bottom_corner_space = 0;

    decor_get_default_layout (&mContext, minWidth, minHeight, &mLayout);
    mNQuads = decor_set_lSrStSbS_window_quads (mQuads, &mContext, &mLayout);
}

cdt::FakePixmapTypeDecoration::~FakePixmapTypeDecoration ()
{
    XFreePixmap (mDpy, mPixmap);
}

size_t
cdt::FakePixmapTypeDecoration::propertyDataSize () const
{
    return BASE_PROP_SIZE + (QUAD_PROP_SIZE * mNQuads);
}

void
cdt::FakePixmapTypeDecoration::addPropertyData (std::vector<long> &vec) const
{
    long propData[PROP_HEADER_SIZE + propertyDataSize ()];

    decor_quads_to_property (propData,
			     0,
			     mPixmap,
			     const_cast <decor_extents_t *> (&mRestoredInput),
			     const_cast <decor_extents_t *> (&mRestoredBorder),
			     const_cast <decor_extents_t *> (&mMaximizedInput),
			     const_cast <decor_extents_t *> (&mMaximizedBorder),
			     mMinWidth,
			     mMinHeight,
			     const_cast <decor_quad_t *> (mQuads),
			     mNQuads,
			     mType,
			     mState,
			     mActions);

    for (size_t i = PROP_HEADER_SIZE;
	 i < (PROP_HEADER_SIZE + propertyDataSize ());
	 ++i)
	vec.push_back (propData[i]);
}

CompPoint
cdt::FakePixmapTypeDecoration::restoredFrameWindowOffset () const
{
    int xOffset = mRestoredBorder.left - mRestoredInput.left;
    int yOffset = mRestoredBorder.top - mRestoredInput.top;

    return CompPoint (xOffset, yOffset);
}

CompSize
cdt::FakePixmapTypeDecoration::restoredDecorationSize () const
{
    return CompSize (mRestoredInput.left + mRestoredInput.right,
		     mRestoredInput.top + mRestoredInput.bottom);
}

void
cdt::FakePixmapTypeDecoration::changeRestoredBorder (const decor_extents_t &b)
{
    mRestoredBorder = b;
}

void
cdt::FakePixmapTypeDecoration::changeRestoredInput (const decor_extents_t &i)
{
    mRestoredInput = i;
}

void
cdt::FakeDecorationList::AddDecoration (const cdt::FakeDecoration::Ptr &decoration)
{
    decorations.push_back (decoration);
}

namespace
{
bool MatchesTypeStateAndActions (const cdt::FakeDecoration::Ptr &decoration,
				 unsigned int type,
				 unsigned int state,
				 unsigned int actions)
{
    return decoration->match (type, state, actions);
}
}

void cdt::FakeDecorationList::RemoveDecoration (unsigned int type,
						unsigned int state,
						unsigned int actions)
{
    decorations.erase (
	std::remove_if (decorations.begin (),
			decorations.end (),
			boost::bind (MatchesTypeStateAndActions,
				     _1,
				     type,
				     state,
				     actions)),
	decorations.end ());
}

void cdt::FakeDecorationList::RemoveAllDecorations ()
{
    decorations.clear ();
}

void
cdt::FakeDecorationList::SetPropertyOnWindow (Display *dpy,
					      Window  w,
					      Atom    property)
{
    size_t size = PROP_HEADER_SIZE;
    for (std::vector <FakeDecoration::Ptr>::iterator it = decorations.begin ();
	 it != decorations.end ();
	 ++it)
	size += (*it)->propertyDataSize ();

    std::vector <long> data;

    data.reserve (size);

    data.push_back (decor_version ());
    data.push_back (mDecorationType);
    data.push_back (decorations.size ());

    for (std::vector <FakeDecoration::Ptr>::iterator it = decorations.begin ();
	 it != decorations.end ();
	 ++it)
	(*it)->addPropertyData (data);

    XChangeProperty (dpy,
		     w,
		     property,
		     XA_INTEGER,
		     32,
		     PropModeReplace,
		     reinterpret_cast <unsigned char *> (&data[0]),
		     size);
}

cdt::AcquisitionFailed::AcquisitionFailed (int status) :
    mStatus (status)
{
}

const char *
cdt::AcquisitionFailed::what () const throw ()
{
    switch (mStatus)
    {
	case DECOR_ACQUIRE_STATUS_FAILED:
	    return FailedToAcquireMessage;
	case DECOR_ACQUIRE_STATUS_OTHER_DM_RUNNING:
	    return OtherDecorationManagerRunningMessage;
	default:
	    return "Unknown status";
    }

    return "Unknown status";
}

cdt::FakeDecorator::FakeDecorator (Display *d,
				   int     supports) :
    mDpy (d),
    mScreenNumber (DefaultScreen (d)),
    mRootWindow (DefaultRootWindow (d)),
    mSessionStatus (decor_acquire_dm_session (d,
					      mScreenNumber,
					      "fake",
					      1,
					      &mDecorationManagerTimestamp)),
    mSessionOwnerAtom (0),
    mSessionOwner (0),
    mSupportingHintAtom (XInternAtom (d, DECOR_TYPE_ATOM_NAME, 0)),
    mSupportingHintWindowAtom (XInternAtom (d,
					    DECOR_SUPPORTING_DM_CHECK_ATOM_NAME,
					    0))
{
    if (mSessionStatus != DECOR_ACQUIRE_STATUS_SUCCESS)
	throw AcquisitionFailed (mSessionStatus);

    std::stringstream sessionOwnerStream;

    sessionOwnerStream << "_COMPIZ_DM_S" << mScreenNumber;
    mSessionOwnerAtom = XInternAtom (d,
				     sessionOwnerStream.str ().c_str (),
				     1);

    mSessionOwner = XGetSelectionOwner (d, mSessionOwnerAtom);

    if (!mSessionOwner)
	throw std::runtime_error ("Expected a selection owner");

    decor_set_dm_check_hint (d, mScreenNumber, supports);

    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (mDpy,
			    mRootWindow,
			    mSupportingHintWindowAtom,
			    XA_WINDOW,
			    32,
			    1,
			    0,
			    data);

    mSupportingHintWindow = *(reinterpret_cast <Window *> (data.get ()));

    if (!mSupportingHintWindow)
	throw std::runtime_error ("Failed to find supporting hint window");


}

cdt::FakeDecorator::~FakeDecorator ()
{
    /* Destroy the session owner, taking the selection with it */
    XDestroyWindow (mDpy, mSessionOwner);
    XDestroyWindow (mDpy, mSupportingHintWindow);
}

Window
cdt::FakeDecorator::currentSelectionOwner ()
{
    return mSessionOwner;
}

Atom
cdt::FakeDecorator::sessionOwnerAtom ()
{
    return mSessionOwnerAtom;
}

Atom
cdt::FakeDecorator::supportingCheckWindow ()
{
    return mSupportingHintWindow;
}

TEST_F (BaseDecorAcceptance, Startup)
{
}

TEST_F (BaseDecorAcceptance, FakeDecoratorSessionOwnerNameSetOnSelectionOwner)
{
    cdt::FakeDecorator decorator (Display ());
    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (Display (),
			    decorator.currentSelectionOwner (),
			    mDecorationManagerNameAtom,
			    mUtf8StringAtom,
			    8,
			    4,
			    0,
			    data);

    std::string name (reinterpret_cast <char *> (data.get ()));

    EXPECT_THAT (name, StrEq (cdt::DecorationClientName));

}

TEST_F (BaseDecorAcceptance, FakeDecoratorReceiveClientMessage)
{
    cdt::FakeDecorator decorator (Display ());

    ManagerMessageMatcher matcher (Display (),
				   decorator.sessionOwnerAtom ());

    EXPECT_TRUE (Advance (Display (),
			  ct::WaitForEventOfTypeOnWindowMatching (Display (),
								  mRootWindow,
								  ClientMessage,
								  -1,
								  -1,
								  matcher)));
}

TEST_F (BaseDecorAcceptance, DecorationSupportsWindowType)
{
    cdt::FakeDecorator decorator (Display (),
				  WINDOW_DECORATION_TYPE_WINDOW);

    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (Display (),
			    decorator.supportingCheckWindow (),
			    mDecorationTypeAtom,
			    XA_ATOM,
			    32,
			    1,
			    0,
			    data);

    Atom *atoms = reinterpret_cast <Atom *> (data.get ());
    EXPECT_EQ (atoms[0], mDecorationTypeWindow);
}

TEST_F (BaseDecorAcceptance, DecorationSupportsPixmapType)
{
    cdt::FakeDecorator decorator (Display (),
				  WINDOW_DECORATION_TYPE_PIXMAP);

    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (Display (),
			    decorator.supportingCheckWindow (),
			    mDecorationTypeAtom,
			    XA_ATOM,
			    32,
			    1,
			    0,
			    data);

    Atom *atoms = reinterpret_cast <Atom *> (data.get ());
    EXPECT_EQ (atoms[0], mDecorationTypePixmap);
}

class DecorFakeDecoratorAcceptance :
    public BaseDecorAcceptance
{
    protected:

	virtual void SetUp ();
	virtual void TearDown ();

	virtual unsigned int SupportedDecorations () const;
	virtual bool         StartDecoratorOnSetUp () const;

	void SetUpDecorator ();

	void DisallowDecorationsOnWindow (Window window);
	void AllowDecorationsOnWindow (Window window);

	std::auto_ptr <cdt::FakeDecorator> decorator;

	Atom NETWMFrameExtentsAtom;
	Atom WindowDecorationAtom;
	Atom DefaultActiveDecorationAtom;
	Atom DefaultBareDecorationAtom;
	Atom MWMHintsAtom;
};

void
DecorFakeDecoratorAcceptance::SetUp ()
{
    BaseDecorAcceptance::SetUp ();

    NETWMFrameExtentsAtom = XInternAtom (Display (),
					 "_NET_FRAME_EXTENTS",
					 0);

    WindowDecorationAtom = XInternAtom (Display (),
					DECOR_WINDOW_ATOM_NAME,
					0);

    DefaultActiveDecorationAtom = XInternAtom (Display (),
					       DECOR_ACTIVE_ATOM_NAME,
					       0);

    DefaultBareDecorationAtom = XInternAtom (Display (),
					     DECOR_BARE_ATOM_NAME,
					     0);

    if (StartDecoratorOnSetUp ())
	SetUpDecorator ();
}

void
DecorFakeDecoratorAcceptance::SetUpDecorator ()
{
    decorator.reset (new cdt::FakeDecorator (Display (),
					     SupportedDecorations ()));
}

namespace
{
void ChangeDecorationState (Display      *display,
			    Window       window,
			    unsigned int decorationFlags)
{
    Atom          mwmHintsAtom = XInternAtom (display, "_MOTIF_WM_HINTS", 0);
    cdt::MWMHints hints;

    hints.flags = 1L << 1;
    hints.decorations = decorationFlags;

    XChangeProperty (display,
		     window,
		     mwmHintsAtom,
		     mwmHintsAtom,
		     32,
		     PropModeReplace,
		     reinterpret_cast <unsigned char *> (&hints),
		     3);
}
}

void
DecorFakeDecoratorAcceptance::DisallowDecorationsOnWindow (Window window)
{
    ChangeDecorationState (Display (), window, 0);
}

void
DecorFakeDecoratorAcceptance::AllowDecorationsOnWindow (Window window)
{
    ChangeDecorationState (Display (), window, cdt::MwmDecor);
}

void
DecorFakeDecoratorAcceptance::TearDown ()
{
    decorator.reset ();

    BaseDecorAcceptance::TearDown ();
}

unsigned int
DecorFakeDecoratorAcceptance::SupportedDecorations () const
{
    return WINDOW_DECORATION_TYPE_WINDOW;
}

bool
DecorFakeDecoratorAcceptance::StartDecoratorOnSetUp () const
{
    return true;
}

namespace
{
void RecievePropertyNotifyEvents (Display *dpy,
				  Window  w)
{
    XWindowAttributes attrib;

    XGetWindowAttributes (dpy, w, &attrib);
    XSelectInput (dpy, w, attrib.your_event_mask | PropertyChangeMask);
}

void WaitForPropertyNotify (Display           *dpy,
			    Window            w,
			    std::string const &prop)
{
    RecievePropertyNotifyEvents (dpy, w);

    ct::PropertyNotifyXEventMatcher matcher (dpy,
					     prop);

    ASSERT_TRUE (Advance (dpy,
			  ct::WaitForEventOfTypeOnWindowMatching (dpy,
								  w,
								  PropertyNotify,
								  -1,
								  -1,
								  matcher)));
}

void WaitForConfigureOn (Display      *display,
			 Window       window,
			 int          x,
			 int          y,
			 unsigned int width,
			 unsigned int height,
			 unsigned int mask)
{
    /* Wait for the ConfigureNotify on the frame window
     * after an offset move */
    ct::ConfigureNotifyXEventMatcher matcher (None,
					      0,
					      x,
					      y,
					      width,
					      height,
					      mask);

    /* The use of the timeout is currently because some functionality
     * is broken which would cause these barrier conditions to never
     * eventuate. For now just get to the fail condition */
    Advance (display,
	     ct::WaitForEventOfTypeOnWindowMatching (display,
						     window,
						     ConfigureNotify,
						     -1,
						     -1,
						     matcher,
						     1000));
}

void WaitForFrameExtents (Display *dpy,
			  Window  w)
{
    RecievePropertyNotifyEvents (dpy, w);

    XMapRaised (dpy, w);

    WaitForPropertyNotify (dpy, w, "_NET_FRAME_EXTENTS");
}

Window FindParent (Display *dpy,
		   Window  w)
{
    Window parent = 0;
    Window next = w;
    Window root = DefaultRootWindow (dpy);

    while (next != root)
    {
	parent = next;

	Window       dummy;
	Window       *children;
	unsigned int nchildren;

	int status = XQueryTree (dpy, parent, &dummy, &next, &children, &nchildren);
	XFree (children);

	if (!status)
	    throw std::logic_error ("XQueryTree failed");
    }

    return parent;
}

void FreeWindowArray (Window *array)
{
    if (array)
	XFree (array);
}

boost::shared_array <Window> FetchChildren (Display      *dpy,
					    Window       w,
					    unsigned int &n)
{
    Window *children;
    Window dummy;

    int status = XQueryTree (dpy,
			     w,
			     &dummy,
			     &dummy,
			     &children,
			     &n);

    if (!status)
	throw std::logic_error ("XQueryTree failed");

    return boost::shared_array <Window> (children,
					 boost::bind (FreeWindowArray, _1));
}

class FrameExtentsMatcher :
    public MatcherInterface <unsigned long *>
{
    public:

	FrameExtentsMatcher (unsigned int left,
			     unsigned int right,
			     unsigned int top,
			     unsigned int bottom);

	bool MatchAndExplain (unsigned long *extents,
			      MatchResultListener *listener) const;

	void DescribeTo (std::ostream *os) const;

    private:

	unsigned int mLeft;
	unsigned int mRight;
	unsigned int mTop;
	unsigned int mBottom;
};

Matcher <unsigned long *>
IsExtents (unsigned int left,
	   unsigned int right,
	   unsigned int top,
	   unsigned int bottom)
{
    return MakeMatcher (new FrameExtentsMatcher (left, right, top, bottom));
}
}

FrameExtentsMatcher::FrameExtentsMatcher (unsigned int left,
					  unsigned int right,
					  unsigned int top,
					  unsigned int bottom) :
    mLeft (left),
    mRight (right),
    mTop (top),
    mBottom (bottom)
{
}

bool
FrameExtentsMatcher::MatchAndExplain (unsigned long *extents,
				      MatchResultListener *listener) const
{
    return mLeft == extents[0] &&
	   mRight == extents[1] &&
	   mTop == extents[2] &&
	   mBottom == extents[3];
}

void
FrameExtentsMatcher::DescribeTo (std::ostream *os) const
{
    *os << "Expected frame extents of :" << std::endl
	<< " left: " << mLeft << std::endl
	<< " right: " << mRight << std::endl
	<< " top: " << mTop << std::endl
	<< " bottom: " << mBottom << std::endl;
}

TEST_F (DecorFakeDecoratorAcceptance, WindowDefaultFallbackNoExtents)
{
    Window w = ct::CreateNormalWindow (Display ());
    WaitForFrameExtents (Display (), w);

    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (Display (),
			    w,
			    NETWMFrameExtentsAtom,
			    XA_CARDINAL,
			    32,
			    4,
			    0,
			    data);

    unsigned long *frameExtents =
	reinterpret_cast <unsigned long *> (data.get ());

    EXPECT_THAT (frameExtents, IsExtents (0, 0, 0, 0));
}

class DecorWithPixmapDefaultsAcceptance :
    public DecorFakeDecoratorAcceptance
{
    protected:

	DecorWithPixmapDefaultsAcceptance ();

	virtual void SetUp ();
	virtual void TearDown ();
	virtual unsigned int SupportedDecorations () const;

    private:

	Window mRoot;

    protected:

	cdt::FakeDecorationList rootActiveDecorationList;
	cdt::FakeDecorationList rootBareDecorationList;

	static const unsigned int MaximizedBorderExtent = 1;
	static const unsigned int ActiveBorderExtent = 2;
	static const unsigned int ActiveInputExtent = 4;
};

namespace
{
cdt::FakePixmapTypeDecoration::Ptr
MakeFakePixmapTypeDecoration (unsigned int type,
			      unsigned int state,
			      unsigned int actions,
			      unsigned int minWidth,
			      unsigned int minHeight,
			      const decor_extents_t &restoredBorder,
			      const decor_extents_t &restoredInput,
			      const decor_extents_t &maximizedBorder,
			      const decor_extents_t &maximizedInput,
			      Display               *dpy)
{
    cdt::FakePixmapTypeDecoration *decoration =
	new cdt::FakePixmapTypeDecoration (type,
					   state,
					   actions,
					   minWidth,
					   minHeight,
					   restoredBorder,
					   restoredInput,
					   maximizedBorder,
					   maximizedInput,
					   dpy);

    return boost::shared_ptr <cdt::FakePixmapTypeDecoration> (decoration);
}

decor_extents_t
DecorationExtents (unsigned int    left,
		   unsigned int    right,
		   unsigned int    top,
		   unsigned int    bottom)
{
    decor_extents_t extents;

    extents.left = left;
    extents.right = right;
    extents.top = top;
    extents.bottom = bottom;

    return extents;
}

class ExtentsFromMatcher :
    public MatcherInterface <Window>
{
    public:

	ExtentsFromMatcher (Display               *dpy,
			    Window                w,
			    const decor_extents_t &extents);

	bool MatchAndExplain (Window window,
			      MatchResultListener *listener) const;

	void DescribeTo (std::ostream *os) const;

    private:

	Display         *mDpy;
	Window          mWindow;
	decor_extents_t mExpectedExtents;
};

Matcher <Window>
ExtendsFromWindowBy (Display               *dpy,
		     Window                w,
		     const decor_extents_t &extents)
{
    return MakeMatcher (new ExtentsFromMatcher (dpy, w, extents));
}
}

std::ostream &
operator<< (std::ostream &lhs, const decor_extents_t &extents)
{
    return lhs << "Extents: " << std::endl
	       << " left: " << extents.left << std::endl
	       << " right: " << extents.right << std::endl
	       << " top: " << extents.top << std::endl
	       << " bottom: " << extents.bottom << std::endl;
}

ExtentsFromMatcher::ExtentsFromMatcher (Display               *dpy,
					Window                w,
					const decor_extents_t &extents) :
    mDpy (dpy),
    mWindow (w),
    mExpectedExtents (extents)
{
}

bool
ExtentsFromMatcher::MatchAndExplain (Window window,
				     MatchResultListener *listener) const
{
    unsigned int border, depth;
    Window       root;

    int          compareX, compareY, matchX, matchY;
    unsigned int compareWidth, compareHeight, matchWidth, matchHeight;

    if (!XGetGeometry (mDpy, window, &root,
		       &matchX, &matchY, &matchWidth, &matchHeight,
		       &border, &depth))
	throw std::logic_error ("XGetGeometry failed");

    if (!XGetGeometry (mDpy, mWindow, &root,
		       &compareX, &compareY, &compareWidth, &compareHeight,
		       &border, &depth))
	throw std::logic_error ("XGetGeometry failed");

    unsigned int left = matchX - compareX;
    unsigned int top = matchY - compareY;
    unsigned int right = (matchX + matchWidth) - (compareX + compareWidth);
    unsigned int bottom = (matchY + matchHeight) - (compareY + compareHeight);

    decor_extents_t determinedExtents = DecorationExtents (left, right, top, bottom);

    return decor_extents_cmp (&determinedExtents, &mExpectedExtents);
}

void
ExtentsFromMatcher::DescribeTo (std::ostream *os) const
{
    *os << "Extends outwards from " << std::hex << mWindow << std::dec
	<< " by: " << mExpectedExtents;
}

DecorWithPixmapDefaultsAcceptance::DecorWithPixmapDefaultsAcceptance () :
    mRoot (0),
    rootActiveDecorationList (WINDOW_DECORATION_TYPE_PIXMAP),
    rootBareDecorationList (WINDOW_DECORATION_TYPE_PIXMAP)
{
}

unsigned int
DecorWithPixmapDefaultsAcceptance::SupportedDecorations () const
{
    return WINDOW_DECORATION_TYPE_PIXMAP;
}

void
DecorWithPixmapDefaultsAcceptance::SetUp ()
{
    DecorFakeDecoratorAcceptance::SetUp ();

    mRoot = DefaultRootWindow (Display ());

    unsigned int ResBo = ActiveBorderExtent;
    unsigned int ResIn = ActiveInputExtent;
    unsigned int MaxEx = MaximizedBorderExtent;

    decor_extents_t activeBorderRestored (DecorationExtents (ResBo, ResBo, ResBo, ResBo));
    decor_extents_t activeBorderMaximized (DecorationExtents (MaxEx, MaxEx, MaxEx, MaxEx));
    decor_extents_t activeInputRestored (DecorationExtents (ResIn, ResIn, ResIn, ResIn));
    decor_extents_t activeInputMaximized (DecorationExtents (MaxEx, MaxEx, MaxEx, MaxEx));

    decor_extents_t emptyExtents (DecorationExtents (0, 0, 0, 0));

    cdt::FakeDecoration::Ptr rootActiveDecoration =
	MakeFakePixmapTypeDecoration (DECOR_WINDOW_TYPE_NORMAL,
				      0,
				      0,
				      10, 10,
				      activeBorderRestored,
				      activeInputRestored,
				      activeBorderMaximized,
				      activeInputMaximized,
				      Display ());

    cdt::FakeDecoration::Ptr rootBareDecoration =
	MakeFakePixmapTypeDecoration (0, 0, 0,
				      1, 1,
				      emptyExtents,
				      emptyExtents,
				      emptyExtents,
				      emptyExtents,
				      Display ());

    rootActiveDecorationList.AddDecoration (rootActiveDecoration);
    rootBareDecorationList.AddDecoration (rootBareDecoration);

    rootActiveDecorationList.SetPropertyOnWindow (Display (),
						  mRoot,
						  DefaultActiveDecorationAtom);
    rootBareDecorationList.SetPropertyOnWindow (Display (),
						mRoot,
						DefaultBareDecorationAtom);
}

void
DecorWithPixmapDefaultsAcceptance::TearDown ()
{
    /* Remove inserted decorations */
    rootActiveDecorationList.RemoveDecoration (DECOR_WINDOW_TYPE_NORMAL,
					       0,
					       0);
    rootBareDecorationList.RemoveDecoration (0, 0, 0);

    DecorFakeDecoratorAcceptance::TearDown ();
}

TEST_F (DecorWithPixmapDefaultsAcceptance, FallbackRecieveInputFrameNotify)
{
    Window w = ct::CreateNormalWindow (Display ());
    RecievePropertyNotifyEvents (Display (), w);
    XMapRaised (Display (), w);

    ct::PropertyNotifyXEventMatcher matcher (Display (),
					     DECOR_INPUT_FRAME_ATOM_NAME);

    EXPECT_TRUE (Advance (Display (),
			  ct::WaitForEventOfTypeOnWindowMatching (Display (),
								  w,
								  PropertyNotify,
								  -1,
								  -1,
								  matcher)));
}

TEST_F (DecorWithPixmapDefaultsAcceptance, FallbackHasInputFrameInParent)
{
    Window w = ct::CreateNormalWindow (Display ());

    XMapRaised (Display (), w);
    WaitForPropertyNotify (Display (), w, DECOR_INPUT_FRAME_ATOM_NAME);

    Window parent = FindParent (Display (), w);

    unsigned int                 nChildren;
    boost::shared_array <Window> children (FetchChildren (Display (),
							  parent,
							  nChildren));

    EXPECT_EQ (2, nChildren);
}

namespace
{
Window FindDecorationWindowFromChildren (Display *dpy,
					 const boost::shared_array <Window> &c,
					 unsigned int nChildren)
{
    for (unsigned int i = 0; i < nChildren; ++i)
    {
	/* The decoration window will have no children, but
	 * the wrapper window will have one child */
	unsigned int                 n;
	boost::shared_array <Window> childChildren (FetchChildren (dpy,
								   c[i],
								   n));
	if (n == 0)
	    return c[i];
    }

    return None;
}
}

TEST_F (DecorWithPixmapDefaultsAcceptance, FallbackNormalWindowExtentOnDecoration)
{
    Window w = ct::CreateNormalWindow (Display ());

    XMapRaised (Display (), w);
    WaitForPropertyNotify (Display (), w, DECOR_INPUT_FRAME_ATOM_NAME);

    Window parent = FindParent (Display (), w);

    unsigned int                 nChildren;
    boost::shared_array <Window> children (FetchChildren (Display (),
							  parent,
							  nChildren));

    ASSERT_EQ (2, nChildren);

    Window decorationWindow = FindDecorationWindowFromChildren (Display (),
								children,
								nChildren);

    ASSERT_NE (None, decorationWindow);

    decor_extents_t borderExtents (DecorationExtents (ActiveBorderExtent,
						      ActiveBorderExtent,
						      ActiveBorderExtent,
						      ActiveBorderExtent));
    EXPECT_THAT (decorationWindow, ExtendsFromWindowBy (Display (),
							w,
							borderExtents));
}

TEST_F (DecorWithPixmapDefaultsAcceptance, FallbackNormalWindowInputOnFrame)
{
    Window w = ct::CreateNormalWindow (Display ());

    XMapRaised (Display (), w);
    WaitForPropertyNotify (Display (), w, DECOR_INPUT_FRAME_ATOM_NAME);

    Window parent = FindParent (Display (), w);

    decor_extents_t inputExtents (DecorationExtents (ActiveInputExtent,
						     ActiveInputExtent,
						     ActiveInputExtent,
						     ActiveInputExtent));
    EXPECT_THAT (parent, ExtendsFromWindowBy (Display (),
					      w,
					      inputExtents));
}

/* TODO: Get bare decorations tests */

/* Helper class with some useful member functions */
class PixmapDecoratorAcceptance :
    public DecorWithPixmapDefaultsAcceptance
{
    public:

	typedef cdt::FakeDecoration::Ptr FakeDecorPtr;
	typedef cdt::FakePixmapTypeDecoration::Ptr FakePixDecorPtr;

	Window CreateDecoratableWindow (::Display *display);
	Window MapAndReparent (::Display *display,
			       Window window);

	void DecorateWindow (::Display                *display,
			     Window                   window,
			     const FakePixDecorPtr &decoration);
	void WaitForDecoration (::Display          *display,
				Window             window,
				const FakeDecorPtr &decoration);
	void WaitForDecorationUpdate (::Display          *display,
				      Window             window,
				      const FakeDecorPtr &decor);

	cdt::FakeDecorationList GetDecorations (Window window);
	Window                  GetParent (Window window);

	void ReconfigureDecoration (::Display             *display,
				    Window                window,
				    const FakePixDecorPtr &decor,
				    cdt::FakeDecorationList &list,
				    const decor_extents_t &border,
				    const decor_extents_t &input);

	bool DestroyWindow (::Display *display,
			    Window    window);


    protected:

	std::map <Window, cdt::FakeDecorationList> windowDecorations;
	std::map <Window, Window> windowParents;
};

Window
PixmapDecoratorAcceptance::CreateDecoratableWindow (::Display *display)
{
    return ct::CreateNormalWindow (display);
}

Window
PixmapDecoratorAcceptance::MapAndReparent (::Display *display,
					   Window    window)
{
    if (windowParents.find (window) != windowParents.end ())
	return windowParents[window];

    XSelectInput (display, window,
		  StructureNotifyMask |
		  PropertyChangeMask);
    XMapRaised (display, window);

    /* Wait for the window to be reparented */
    Advance (Display (),
	     ct::WaitForEventOfTypeOnWindow (display,
					     window,
					     ReparentNotify,
					     -1,
					     -1));

    /* Select for StructureNotify on the parent and wrapper
     * windows */
    windowParents[window] = FindParent (display, window);

    Window root = 0;
    Window wrapper = ct::GetImmediateParent (display, window, root);

    XSelectInput (display, windowParents[window], StructureNotifyMask);
    XSelectInput (display, wrapper, StructureNotifyMask);

    return windowParents[window];
}

void
PixmapDecoratorAcceptance::DecorateWindow (::Display             *display,
					   Window                window,
					   const FakePixDecorPtr &decoration)
{
    windowDecorations[window] = cdt::FakeDecorationList (WINDOW_DECORATION_TYPE_PIXMAP);
    windowDecorations[window].AddDecoration (decoration);
    windowDecorations[window].SetPropertyOnWindow (display,
						window,
						WindowDecorationAtom);
}

void
PixmapDecoratorAcceptance::WaitForDecoration (::Display          *display,
					      Window             window,
					      const FakeDecorPtr &decoration)
{
    WaitForPropertyNotify (display, window, DECOR_INPUT_FRAME_ATOM_NAME);
    WaitForDecorationUpdate (display, window, decoration);
}

void
PixmapDecoratorAcceptance::WaitForDecorationUpdate (::Display          *display,
						    Window             window,
						    const FakeDecorPtr &decor)
{
    /* Wait for the frame extents to change */
    WaitForPropertyNotify (display, window, "_NET_FRAME_EXTENTS");

    const CompPoint &framePos (decor->restoredFrameWindowOffset ());

    /* Wait for the ConfigureNotify on the frame window
     * after an offset move */
    WaitForConfigureOn (display,
			GetParent (window),
			framePos.x (),
			framePos.y (),
			0,
			0,
			CWX | CWY);
}

void
PixmapDecoratorAcceptance::ReconfigureDecoration (::Display             *display,
						  Window                window,
						  const FakePixDecorPtr &decor,
						  cdt::FakeDecorationList &list,
						  const decor_extents_t &border,
						  const decor_extents_t &input)
{
    decor->changeRestoredInput (input);
    decor->changeRestoredBorder (border);
    list.SetPropertyOnWindow (display,
			      window,
			      WindowDecorationAtom);

    WaitForDecorationUpdate (display, window, decor);
}

bool
PixmapDecoratorAcceptance::DestroyWindow (::Display *display,
					  Window    window)
{
    std::map <Window, Window>::iterator parentIterator =
	windowParents.find (window);
    std::map <Window, cdt::FakeDecorationList>::iterator decorIterator =
	windowDecorations.find (window);

    if (parentIterator != windowParents.end ())
	windowParents.erase (parentIterator);
    if (decorIterator != windowDecorations.end ())
    {
	windowDecorations[window].RemoveAllDecorations ();
	windowDecorations.erase (decorIterator);
    }

    XDestroyWindow (display, window);

    return parentIterator != windowParents.end () ||
	   decorIterator != windowDecorations.end ();
}

Window
PixmapDecoratorAcceptance::GetParent (Window window)
{
    return windowParents[window];
}

cdt::FakeDecorationList
PixmapDecoratorAcceptance::GetDecorations (Window window)
{
    return windowDecorations[window];
}

class PixmapDecoratedWindowAcceptance :
    public PixmapDecoratorAcceptance
{
    public:

	virtual void SetUp ();
	virtual void TearDown ();

	virtual bool StartDecoratorOnSetUp () const;

    protected:

	Window mTestWindow;
	Window mTestWindowParent;

	cdt::FakePixmapTypeDecoration::Ptr mTestWindowDecoration;
};

void
PixmapDecoratedWindowAcceptance::SetUp ()
{
    DecorWithPixmapDefaultsAcceptance::SetUp ();

    ::Display *display = Display ();

    mTestWindow = CreateDecoratableWindow (display);
    mTestWindowParent = MapAndReparent (display, mTestWindow);

    /* Start the decorator */
    SetUpDecorator ();

    unsigned int ResBo = ActiveBorderExtent;
    unsigned int ResIn = ActiveInputExtent;
    unsigned int MaxEx = MaximizedBorderExtent;

    mTestWindowDecoration =
	MakeFakePixmapTypeDecoration (DECOR_WINDOW_TYPE_NORMAL,
				      0,
				      0,
				      10,
				      10,
				      DecorationExtents (ResBo, ResBo, ResBo, ResBo),
				      DecorationExtents (ResIn, ResIn, ResIn, ResIn),
				      DecorationExtents (MaxEx, MaxEx, MaxEx, MaxEx),
				      DecorationExtents (MaxEx, MaxEx, MaxEx, MaxEx),
				      Display ());

    DecorateWindow (display, mTestWindow, mTestWindowDecoration);
    WaitForDecoration (display, mTestWindow, mTestWindowDecoration);
}

void
PixmapDecoratedWindowAcceptance::TearDown ()
{
    DestroyWindow (Display (), mTestWindow);

    DecorWithPixmapDefaultsAcceptance::TearDown ();
}

bool
PixmapDecoratedWindowAcceptance::StartDecoratorOnSetUp () const
{
    return false;
}

namespace
{
static const unsigned int RemoveState = 0;
static const unsigned int AddState = 1;
static const unsigned int ToggleState = 2;

void ChangeStateOfWindow (Display           *dpy,
			  unsigned int      mode,
			  const std::string &state,
			  Window            w)
{
    XEvent event;

    const long ClientTypePager = 2;

    event.type = ClientMessage;
    event.xany.window = w;

    event.xclient.format = 32;
    event.xclient.message_type = XInternAtom (dpy, "_NET_WM_STATE", 0);

    event.xclient.data.l[0] = mode;
    event.xclient.data.l[1] = XInternAtom (dpy, state.c_str (), 0);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = ClientTypePager;

    XSendEvent (dpy,
		DefaultRootWindow (dpy),
		1,
		SubstructureRedirectMask,
		&event);
    XSync (dpy, 0);
}

boost::shared_ptr <unsigned char>
FetchCardinalProperty (Display *dpy,
		       Window  w,
		       Atom    NETWMFrameExtentsAtom)
{
    boost::shared_ptr <unsigned char> data;

    FetchAndVerifyProperty (dpy,
			    w,
			    NETWMFrameExtentsAtom,
			    XA_CARDINAL,
			    32,
			    4,
			    0,
			    data);

    return data;
}
}

TEST_F (PixmapDecoratedWindowAcceptance, MaximizeBorderExtentsOnMaximize)
{
    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 mTestWindow);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 mTestWindow);

    WaitForPropertyNotify (Display (), mTestWindow, "_NET_FRAME_EXTENTS");

    boost::shared_ptr <unsigned char> data =
	FetchCardinalProperty (Display (),
			       mTestWindow,
			       NETWMFrameExtentsAtom);

    unsigned long *frameExtents =
	reinterpret_cast <unsigned long *> (data.get ());

    unsigned int MaxEx = MaximizedBorderExtent;

    EXPECT_THAT (frameExtents, IsExtents (MaxEx, MaxEx, MaxEx, MaxEx));
}

TEST_F (PixmapDecoratedWindowAcceptance, MaximizeBorderExtentsOnVertMaximize)
{
    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 mTestWindow);

    WaitForPropertyNotify (Display (), mTestWindow, "_NET_FRAME_EXTENTS");

    boost::shared_ptr <unsigned char> data =
	FetchCardinalProperty (Display (),
			       mTestWindow,
			       NETWMFrameExtentsAtom);

    unsigned long *frameExtents =
	reinterpret_cast <unsigned long *> (data.get ());

    unsigned int MaxEx = MaximizedBorderExtent;

    EXPECT_THAT (frameExtents, IsExtents (MaxEx, MaxEx, MaxEx, MaxEx));
}

TEST_F (PixmapDecoratedWindowAcceptance, MaximizeBorderExtentsOnHorzMaximize)
{
    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 mTestWindow);

    WaitForPropertyNotify (Display (), mTestWindow, "_NET_FRAME_EXTENTS");

    boost::shared_ptr <unsigned char> data =
	FetchCardinalProperty (Display (),
			       mTestWindow,
			       NETWMFrameExtentsAtom);

    unsigned long *frameExtents =
	reinterpret_cast <unsigned long *> (data.get ());

    unsigned int MaxEx = MaximizedBorderExtent;

    EXPECT_THAT (frameExtents, IsExtents (MaxEx, MaxEx, MaxEx, MaxEx));
}

TEST_F (PixmapDecoratedWindowAcceptance, MaximizeFrameWindowSizeEqOutputSize)
{
    XWindowAttributes attrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &attrib);

    /* The assumption here is that there is only one output */

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 mTestWindow);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 mTestWindow);

    ct::ConfigureNotifyXEventMatcher matcher (None,
					      0,
					      0,
					      0,
					      attrib.width,
					      attrib.height,
					      CWX | CWY | CWWidth | CWHeight);

    EXPECT_TRUE (Advance (Display (),
			  ct::WaitForEventOfTypeOnWindowMatching (Display (),
								  mTestWindowParent,
								  ConfigureNotify,
								  -1,
								  -1,
								  matcher)));
}

TEST_F (PixmapDecoratedWindowAcceptance, VertMaximizeFrameWindowSizeEqOutputYHeight)
{
    XWindowAttributes attrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &attrib);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 mTestWindow);

    ct::ConfigureNotifyXEventMatcher matcher (None,
					      0,
					      0,
					      0,
					      0,
					      attrib.height,
					      CWY | CWHeight);

    EXPECT_TRUE (Advance (Display (),
			  ct::WaitForEventOfTypeOnWindowMatching (Display (),
								  mTestWindowParent,
								  ConfigureNotify,
								  -1,
								  -1,
								  matcher)));
}

TEST_F (PixmapDecoratedWindowAcceptance, HorzMaximizeFrameWindowSizeEqOutputXWidth)
{
    XWindowAttributes attrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &attrib);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 mTestWindow);

    ct::ConfigureNotifyXEventMatcher matcher (None,
					      0,
					      0,
					      0,
					      attrib.width,
					      0,
					      CWX | CWWidth);

    EXPECT_TRUE (Advance (Display (),
			  ct::WaitForEventOfTypeOnWindowMatching (Display (),
								  mTestWindowParent,
								  ConfigureNotify,
								  -1,
								  -1,
								  matcher)));
}

namespace
{
void RelativeWindowGeometry (Display      *dpy,
			     Window       w,
			     int          &x,
			     int          &y,
			     unsigned int &width,
			     unsigned int &height,
			     unsigned int &border)
{
    Window       root;
    unsigned int depth;

    if (!XGetGeometry (dpy, w, &root, &x, &y, &width, &height, &border, &depth))
	throw std::logic_error ("XGetGeometry failed");
}

void AbsoluteWindowGeometry (::Display    *display,
			     Window       window,
			     int          &x,
			     int          &y,
			     unsigned int &width,
			     unsigned int &height,
			     unsigned int &border)
{
    Window       root;
    Window       child;
    unsigned int depth;

    if (!XGetGeometry (display, window, &root,
		       &x, &y, &width, &height,
		       &border, &depth))
	throw std::logic_error ("XGetGeometry failed");

    if (!XTranslateCoordinates (display, window, root, x, y,
				&x, &y, &child))
	throw std::logic_error ("XTranslateCoordinates failed");
}

typedef void (*RetrievalFunc) (Display      *dpy,
			       Window       window,
			       int          &x,
			       int          &y,
			       unsigned int &width,
			       unsigned int &height,
			       unsigned int &border);

class WindowGeometryMatcher :
    public MatcherInterface <Window>
{
    public:

	WindowGeometryMatcher (Display             *dpy,
			       RetrievalFunc       func,
			       const Matcher <int> &x,
			       const Matcher <int> &y,
			       const Matcher <unsigned int> &width,
			       const Matcher <unsigned int> &height,
			       const Matcher <unsigned int> &border);

	bool MatchAndExplain (Window x, MatchResultListener *listener) const;
	void DescribeTo (std::ostream *os) const;

    private:

	Display       *mDpy;

	RetrievalFunc mFunc;

	Matcher <int> mX;
	Matcher <int> mY;
	Matcher <unsigned int> mWidth;
	Matcher <unsigned int> mHeight;
	Matcher <unsigned int> mBorder;
};

Matcher <Window>
HasGeometry (Display             *dpy,
	     RetrievalFunc       func,
	     const Matcher <int> &x,
	     const Matcher <int> &y,
	     const Matcher <unsigned int> &width,
	     const Matcher <unsigned int> &height,
	     const Matcher <unsigned int> &border)
{
    return MakeMatcher (new WindowGeometryMatcher (dpy,
						   func,
						   x,
						   y,
						   width,
						   height,
						   border));
}
}

WindowGeometryMatcher::WindowGeometryMatcher (Display             *dpy,
					      RetrievalFunc       func,
					      const Matcher <int> &x,
					      const Matcher <int> &y,
					      const Matcher <unsigned int> &width,
					      const Matcher <unsigned int> &height,
					      const Matcher <unsigned int> &border) :
    mDpy (dpy),
    mFunc (func),
    mX (x),
    mY (y),
    mWidth (width),
    mHeight (height),
    mBorder (border)
{
}

bool
WindowGeometryMatcher::MatchAndExplain (Window w,
					MatchResultListener *listener) const
{
    int          x, y;
    unsigned int width, height, border;

    mFunc (mDpy, w, x, y, width, height, border);

    bool match = mX.MatchAndExplain (x, listener) &&
		 mY.MatchAndExplain (y, listener) &&
		 mWidth.MatchAndExplain (width, listener) &&
		 mHeight.MatchAndExplain (height, listener) &&
		 mBorder.MatchAndExplain (border, listener);

    if (!match)
    {
	*listener << "Geometry:"
		  << " x: " << x
		  << " y: " << y
		  << " width: " << width
		  << " height: " << height
		  << " border: " << border;
    }

    return match;
}

void
WindowGeometryMatcher::DescribeTo (std::ostream *os) const
{
    *os << "Window geometry matching :";

    *os << std::endl << " - ";
    mX.DescribeTo (os);

    *os << std::endl << " - ";
    mY.DescribeTo (os);

    *os << std::endl << " - ";
    mWidth.DescribeTo (os);

    *os << std::endl << " - ";
    mHeight.DescribeTo (os);

    *os << std::endl << " - ";
    mBorder.DescribeTo (os);
}

namespace
{
void WindowBorderPositionAttributes (Display *dpy,
				     Window  w,
				     XWindowAttributes &attrib,
				     unsigned int ActiveBorderExtent,
				     unsigned int ActiveInputExtent)
{
    XGetWindowAttributes (dpy, w, &attrib);

    /* Remove border - input offset */
    attrib.x -= (ActiveBorderExtent - ActiveInputExtent);
    attrib.y -= (ActiveBorderExtent - ActiveInputExtent);
    attrib.width -= (ActiveBorderExtent - ActiveInputExtent) * 2;
    attrib.height -= (ActiveBorderExtent - ActiveInputExtent) * 2;
}
}

/* DISABLED - Upon maximization, x offset is 1, width offset is 10 */
TEST_F (PixmapDecoratedWindowAcceptance, DISABLED_VertMaximizeFrameWindowSizeSameXWidth)
{
    XWindowAttributes rootAttrib, attrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &rootAttrib);

    WindowBorderPositionAttributes (Display (),
				    mTestWindowParent,
				    attrib,
				    ActiveBorderExtent,
				    ActiveInputExtent);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 mTestWindow);

    /* Wait for the window to be maximized first */
    WaitForConfigureOn (Display (),
			mTestWindowParent,
			0,
			0,
			0,
			rootAttrib.height,
			CWY | CWHeight);

    /* Query the window geometry and ensure that the width and
     * height have remained the same (adding on any extended borders,
     * in this case 0) */
    EXPECT_THAT (mTestWindowParent,
		 HasGeometry (Display (),
			      RelativeWindowGeometry,
			      attrib.x,
			      _,
			      attrib.width,
			      _,
			      _));
}

/* DISABLED - Upon maximization, y offset is 1, height offset is 10 */
TEST_F (PixmapDecoratedWindowAcceptance, DISABLED_HorzMaximizeFrameWindowSizeSameYHeight)
{
    XWindowAttributes rootAttrib, attrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &rootAttrib);

    WindowBorderPositionAttributes (Display (),
				    mTestWindowParent,
				    attrib,
				    ActiveBorderExtent,
				    ActiveInputExtent);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 mTestWindow);

    /* Wait for the window to be maximized first */
    WaitForConfigureOn (Display (),
			mTestWindowParent,
			0,
			0,
			rootAttrib.width,
			0,
			CWX | CWWidth);

    /* Query the window geometry and ensure that the width and
     * height have remained the same (adding on any extended borders,
     * in this case 0) */
    EXPECT_THAT (mTestWindowParent,
		 HasGeometry (Display (),
			      RelativeWindowGeometry,
			      _,
			      attrib.y,
			      _,
			      attrib.height,
			      _));
}

/* Ensure that a window expands to its original size when it is
 * undecorated */
TEST_F (PixmapDecoratedWindowAcceptance, UndecoratedWindowExpandToOrigSize)
{
    DisallowDecorationsOnWindow (mTestWindow);

    WaitForConfigureOn (Display (),
			mTestWindowParent,
			ct::WINDOW_X,
			ct::WINDOW_Y,
			ct::WINDOW_WIDTH,
			ct::WINDOW_HEIGHT,
			CWX | CWY | CWWidth | CWHeight);

    EXPECT_THAT (mTestWindow,
		 HasGeometry (Display (),
			      AbsoluteWindowGeometry,
			      ct::WINDOW_X,
			      ct::WINDOW_Y,
			      ct::WINDOW_WIDTH,
			      ct::WINDOW_HEIGHT,
			      _));
}

/* DISABLED - Ensure that a window with StaticGravity expands to its
 * original size when it is undecorated
 *
 * X and Y positions do not revert back to their original state.
 */
TEST_F (PixmapDecoratedWindowAcceptance, DISABLED_UndecorateStaticGravityWindow)
{
    XSizeHints hints;

    hints.flags = PWinGravity;
    hints.win_gravity = StaticGravity;

    XSetWMNormalHints (Display (), mTestWindow, &hints);
    DisallowDecorationsOnWindow (mTestWindow);

    WaitForConfigureOn (Display (),
			mTestWindowParent,
			ct::WINDOW_X,
			ct::WINDOW_Y,
			ct::WINDOW_WIDTH,
			ct::WINDOW_HEIGHT,
			CWX | CWY | CWWidth | CWHeight);

    EXPECT_THAT (mTestWindow,
		 HasGeometry (Display (),
			      AbsoluteWindowGeometry,
			      ct::WINDOW_X,
			      ct::WINDOW_Y,
			      ct::WINDOW_WIDTH,
			      ct::WINDOW_HEIGHT,
			      _));
}

class PixmapDecorationAdjustment :
    public PixmapDecoratedWindowAcceptance,
    public WithParamInterface <decor_extents_t>
{
    public:

	void MaximizeWindow (Window window);
	void RestoreWindow (Window       window,
			    int          restoredFrameX,
			    int          restoredFrameY,
			    unsigned int restoredFrameWidth,
			    unsigned int restoredFrameHeight);
};

void
PixmapDecorationAdjustment::MaximizeWindow (Window window)
{
    XWindowAttributes rootAttrib;
    XGetWindowAttributes (Display (), DefaultRootWindow (Display ()), &rootAttrib);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 window);

    ChangeStateOfWindow (Display (),
			 AddState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 window);

    /* Wait for the window to be maximized first */
    WaitForConfigureOn (Display (),
			mTestWindowParent,
			0,
			0,
			rootAttrib.width,
			rootAttrib.height,
			CWX | CWY | CWWidth | CWHeight);
}

void
PixmapDecorationAdjustment::RestoreWindow (Window       window,
					   int          restoredFrameX,
					   int          restoredFrameY,
					   unsigned int restoredFrameWidth,
					   unsigned int restoredFrameHeight)
{
    ChangeStateOfWindow (Display (),
			 RemoveState,
			 "_NET_WM_STATE_MAXIMIZED_HORZ",
			 window);

    ChangeStateOfWindow (Display (),
			 RemoveState,
			 "_NET_WM_STATE_MAXIMIZED_VERT",
			 window);

    /* Wait for the window to be restored first */
    WaitForConfigureOn (Display (),
			mTestWindowParent,
			restoredFrameX,
			restoredFrameY,
			restoredFrameWidth,
			restoredFrameHeight,
			CWX | CWY | CWWidth | CWHeight);
}

TEST_P (PixmapDecorationAdjustment, AdjustRestoredWindowBorderMovesClient)
{
    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   GetParam (),
			   GetParam ());

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   ct::WINDOW_X + GetParam ().left,
					   ct::WINDOW_Y + GetParam ().top,
					   _,
					   _,
					   _));
}

TEST_P (PixmapDecorationAdjustment, DISABLED_AdjustRestoredWindowBorderShrinkClient)
{
    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   GetParam (),
			   GetParam ());

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   _,
					   _,
					   ct::WINDOW_WIDTH - (GetParam ().left +
							       GetParam ().right),
					   ct::WINDOW_HEIGHT - (GetParam ().top +
								GetParam ().bottom),
					   _));
}

TEST_P (PixmapDecorationAdjustment, DISABLED_ClientExpandsAsBorderShrinks)
{
    decor_extents_t newExtents = GetParam ();

    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   DecorationExtents (10, 10, 10, 10),
			   DecorationExtents (10, 10, 10, 10));

    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   newExtents,
			   DecorationExtents (10, 10, 10, 10));

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   _,
					   _,
					   ct::WINDOW_WIDTH - (newExtents.left +
							       newExtents.right),
					   ct::WINDOW_HEIGHT - (newExtents.top +
								newExtents.bottom),
					   _));
}

TEST_P (PixmapDecorationAdjustment, DISABLED_ClientExpandsAsBorderShrinksWhilstMaximized)
{
    decor_extents_t newExtents = GetParam ();

    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   DecorationExtents (10, 10, 10, 10),
			   DecorationExtents (10, 10, 10, 10));

    MaximizeWindow (mTestWindow);

    /* Set the property on the window, then demaximize without waiting
     * for a response we will continue to use the maximized window decoration */
    mTestWindowDecoration->changeRestoredBorder (newExtents);
    windowDecorations[mTestWindow].SetPropertyOnWindow (Display (),
							mTestWindow,
							WindowDecorationAtom);

    const CompPoint &off (mTestWindowDecoration->restoredFrameWindowOffset ());
    const CompSize  &size (mTestWindowDecoration->restoredDecorationSize ());

    RestoreWindow (mTestWindow,
		   ct::WINDOW_X - off.x (),
		   ct::WINDOW_Y - off.y (),
		   ct::WINDOW_WIDTH + size.width (),
		   ct::WINDOW_HEIGHT + size.height ());

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   ct::WINDOW_X + newExtents.left,
					   ct::WINDOW_Y + newExtents.right,
					   ct::WINDOW_WIDTH - (newExtents.left +
							       newExtents.right),
					   ct::WINDOW_HEIGHT - (newExtents.top +
								newExtents.bottom),
					   _));
}

TEST_P (PixmapDecorationAdjustment, DISABLED_ClientExpandsAsBorderShrinksWhilstUndecorated)
{
    decor_extents_t newExtents = GetParam ();

    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   DecorationExtents (10, 10, 10, 10),
			   DecorationExtents (10, 10, 10, 10));

    DisallowDecorationsOnWindow (mTestWindow);

    WaitForConfigureOn (Display (),
			mTestWindowParent,
			ct::WINDOW_X,
			ct::WINDOW_Y,
			ct::WINDOW_WIDTH,
			ct::WINDOW_HEIGHT,
			CWX | CWY | CWWidth | CWHeight);

    /* Set the property on the window, then decorate without waiting
     * for a response we will continue to use the maximized window decoration */
    mTestWindowDecoration->changeRestoredBorder (newExtents);
    windowDecorations[mTestWindow].SetPropertyOnWindow (Display (),
							mTestWindow,
							WindowDecorationAtom);

    AllowDecorationsOnWindow (mTestWindow);

    const CompPoint &off (mTestWindowDecoration->restoredFrameWindowOffset ());
    const CompSize  &size (mTestWindowDecoration->restoredDecorationSize ());

    WaitForConfigureOn (Display (),
			mTestWindowParent,
			ct::WINDOW_X - off.x (),
			ct::WINDOW_Y - off.y (),
			ct::WINDOW_WIDTH + size.width (),
			ct::WINDOW_HEIGHT + size.height (),
			CWX | CWY | CWWidth | CWHeight);

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   ct::WINDOW_X + newExtents.left,
					   ct::WINDOW_Y + newExtents.right,
					   ct::WINDOW_WIDTH - (newExtents.left +
							       newExtents.right),
					   ct::WINDOW_HEIGHT - (newExtents.top +
								newExtents.bottom),
					   _));
}

TEST_P (PixmapDecorationAdjustment, AdjustRestoredWindowInputNoMoveClient)
{
    decor_extents_t newExtents = GetParam ();

    ReconfigureDecoration (Display (),
			   mTestWindow,
			   mTestWindowDecoration,
			   windowDecorations[mTestWindow],
			   DecorationExtents (1, 1, 1, 1),
			   newExtents);

    EXPECT_THAT (mTestWindow, HasGeometry (Display (),
					   AbsoluteWindowGeometry,
					   ct::WINDOW_X + 1,
					   ct::WINDOW_Y + 1,
					   _,
					   _,
					   _));
}

decor_extents_t AdjustmentExtents[] =
{
    DecorationExtents (2, 0, 0, 0),
    DecorationExtents (0, 2, 0, 0),
    DecorationExtents (0, 0, 2, 0),
    DecorationExtents (0, 0, 0, 2)
};

INSTANTIATE_TEST_CASE_P (AdjustmentExtents,
			 PixmapDecorationAdjustment,
			 ValuesIn (AdjustmentExtents));
