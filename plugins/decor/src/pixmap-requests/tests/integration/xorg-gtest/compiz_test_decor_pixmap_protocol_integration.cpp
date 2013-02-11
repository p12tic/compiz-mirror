/*
 * Compiz XOrg GTest Decoration Pixmap Protocol Integration Tests
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

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "decoration.h"

#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

#include "pixmap-requests.h"
#include "compiz_decor_pixmap_requests_mock.h"

namespace xt = xorg::testing;
namespace ct = compiz::testing;
namespace cd = compiz::decor;
namespace cdp = compiz::decor::protocol;

using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::_;

class DecorPixmapProtocol :
    public xorg::testing::Test
{
    public:

	typedef boost::function <void (const XClientMessageEvent &)> ClientMessageReceiver;

	void SetUp ();

	void WaitForClientMessageOnAndDeliverTo (Window client,
						 Atom   message,
						 const ClientMessageReceiver &receiver);

    protected:

	Atom deletePixmapMessage;
	Atom pendingMessage;
};

namespace
{
bool Advance (Display *d, bool result)
{
    return ct::AdvanceToNextEventOnSuccess (d, result);
}

Window MOCK_WINDOW = 1;
Pixmap MOCK_PIXMAP = 2;

class MockClientMessageReceiver
{
    public:

	MOCK_METHOD1 (receiveMsg, void (const XClientMessageEvent &));
};
}

void
DecorPixmapProtocol::SetUp ()
{
    xorg::testing::Test::SetUp ();

    XSelectInput (Display (),
		  DefaultRootWindow (Display ()),
		  StructureNotifyMask);

    deletePixmapMessage = XInternAtom (Display (), DECOR_DELETE_PIXMAP_ATOM_NAME, 0);
    pendingMessage = XInternAtom (Display (), DECOR_PIXMAP_PENDING_ATOM_NAME, 0);
}

class DeliveryMatcher :
    public ct::XEventMatcher
{
    public:

	DeliveryMatcher (Atom message,
			 const DecorPixmapProtocol::ClientMessageReceiver &receiver) :
	    mMessage (message),
	    mReceiver (receiver)
	{
	}

	bool MatchAndExplain (const XEvent &xce,
			      MatchResultListener *listener) const
	{
	    if (xce.xclient.message_type == mMessage)
	    {
		mReceiver (reinterpret_cast <const XClientMessageEvent &> (xce));
		return true;
	    }
	    else
		return false;
	}

	void DescribeTo (std::ostream *os) const
	{
	    *os << "Message delivered";
	}

    private:

	Atom mMessage;
	DecorPixmapProtocol::ClientMessageReceiver mReceiver;
};

void
DecorPixmapProtocol::WaitForClientMessageOnAndDeliverTo (Window client,
							 Atom   message,
							 const ClientMessageReceiver &receiver)
{
    ::Display *d = Display ();

    DeliveryMatcher delivery (message, receiver);
    ASSERT_TRUE (Advance (d, ct::WaitForEventOfTypeOnWindowMatching (d,
								     client,
								     ClientMessage,
								     -1,
								     -1,
								     delivery)));
}

TEST_F (DecorPixmapProtocol, PostDeleteCausesClientMessage)
{
    MockClientMessageReceiver receiver;
    ::Display *d = Display ();

    decor_post_delete_pixmap (d,
			      MOCK_WINDOW,
			      MOCK_PIXMAP);

    EXPECT_CALL (receiver, receiveMsg (_)).Times (1);

    WaitForClientMessageOnAndDeliverTo (MOCK_WINDOW,
					deletePixmapMessage,
					boost::bind (&MockClientMessageReceiver::receiveMsg,
						     &receiver,
						     _1));
}

TEST_F (DecorPixmapProtocol, PostDeleteDispatchesClientMessageToReceiver)
{
    MockProtocolDispatchFuncs dispatch;
    cdp::Communicator         communicator (pendingMessage,
					    deletePixmapMessage,
					    boost::bind (&MockProtocolDispatchFuncs::handlePending,
							 &dispatch,
							 _1,
							 _2),
					    boost::bind (&MockProtocolDispatchFuncs::handleUnused,
							 &dispatch,
							 _1,
							 _2));

    decor_post_delete_pixmap (Display (),
			      MOCK_WINDOW,
			      MOCK_PIXMAP);

    EXPECT_CALL (dispatch, handleUnused (MOCK_WINDOW, MOCK_PIXMAP)).Times (1);

    WaitForClientMessageOnAndDeliverTo (MOCK_WINDOW,
					deletePixmapMessage,
					boost::bind (&cdp::Communicator::handleClientMessage,
						     &communicator,
						     _1));
}
