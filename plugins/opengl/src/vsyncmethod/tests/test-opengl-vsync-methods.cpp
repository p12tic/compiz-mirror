/*
 * Compiz, opengl plugin, vsync methods
 *
 * Copyright © 2012 Canonical Ltd.
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
 * Authors: Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vsync-method-wait-video-sync.h>
#include <vsync-method-swap-interval.h>

namespace cgl = compiz::opengl;
namespace cgli = compiz::opengl::impl;

namespace
{
    class MockOpenGLFunctionsTable
    {
	public:

	    MOCK_METHOD3 (waitVideoSyncSGI, int (int, int, int *));
	    MOCK_METHOD1 (getVideoSyncSGI, int (int *));
	    MOCK_METHOD1 (swapIntervalEXT, void (int));
    };

    cgli::GLXWaitVideoSyncSGIFunc
    GetWaitVideoSyncFuncFromMock (MockOpenGLFunctionsTable &mock)
    {
	return boost::bind (&MockOpenGLFunctionsTable::waitVideoSyncSGI, &mock, _1, _2, _3);
    }

    cgli::GLXGetVideoSyncSGIFunc
    GetGetVideoSyncFuncFromMock (MockOpenGLFunctionsTable &mock)
    {
	return boost::bind (&MockOpenGLFunctionsTable::getVideoSyncSGI, &mock, _1);
    }

    cgli::GLXSwapIntervalEXTFunc
    GetSwapIntervalFuncFromMock (MockOpenGLFunctionsTable &mock)
    {
	return boost::bind (&MockOpenGLFunctionsTable::swapIntervalEXT, &mock, _1);
    }
}

TEST (OpenGLSwapIntervalTest, TestSwapIntervalOnEnable)
{
}
