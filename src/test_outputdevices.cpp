/*
 * Compiz Core: OutputDevices class: Unit tests
 *
 * Copyright (c) 2012 Canonical Ltd.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gtest/gtest.h"
#include "outputdevices.h"

using namespace compiz::private_screen;

TEST (OutputDevices, TrivialSingleMonitor)
{
    OutputDevices d;
    CompSize s (1024, 768);
    CompWindow::Geometry w;

    d.setGeometryOnDevice (0, 0, 0, 1024, 768);

    w.set (50, 50, 100, 100, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-50, 50, 10, 10, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-50, 50, 100, 10, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-10, -10, 1034, 778, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (99999, 100, 123, 456, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));
}

TEST (OutputDevices, SideBySide)
{
    OutputDevices d;
    CompSize s (2048, 768);
    CompWindow::Geometry w;

    d.setGeometryOnDevice (0, 0, 0, 1024, 768);
    d.setGeometryOnDevice (1, 1024, 0, 1024, 768);

    w.set (50, 50, 100, 100, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    /* FIXME... but not until the initial refactoring has landed
    w.set (-50, 50, 10, 10, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));
    */

    w.set (-50, 50, 100, 10, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-10, -10, 1034, 778, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (10, 0, 3000, 768, 0);
    EXPECT_EQ (1, d.outputDeviceForGeometry (w, 0, &s));

    w.set (99999, 100, 123, 456, 0);
    EXPECT_EQ (1, d.outputDeviceForGeometry (w, 0, &s));
}

TEST (OutputDevices, LaptopBelowMonitor)
{
    OutputDevices d;
    CompSize s (1920, 2100);
    CompWindow::Geometry w;

    d.setGeometryOnDevice (0, 0, 0, 1920, 1200);
    d.setGeometryOnDevice (1, 160, 1200, 1600, 900);

    w.set (50, 50, 100, 100, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-50, 50, 100, 10, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (-10, -10, 1034, 778, 0);
    EXPECT_EQ (0, d.outputDeviceForGeometry (w, 0, &s));

    w.set (200, 1500, 20, 20, 0);
    EXPECT_EQ (1, d.outputDeviceForGeometry (w, 0, &s));
}

