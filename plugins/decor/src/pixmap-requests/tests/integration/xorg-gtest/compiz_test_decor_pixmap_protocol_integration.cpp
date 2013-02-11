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

#include "pixmap-requests.h"

namespace cd = compiz::decor;
namespace cdp = compiz::decor::protocol;

class DecorPixmapProtocol :
    public xorg::testing::Test
{
    public:
};

TEST_F (DecorPixmapProtocol, Check)
{
}
