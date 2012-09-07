/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd.
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
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#ifndef _COMPIZCONFIG_TEST_VALUE_COMBINERS_H
#define _COMPIZCONFIG_TEST_VALUE_COMBINERS_H

#include <X11/keysym.h>
#include <ccs.h>
#include <gtest_shared_characterwrapper.h>

namespace compizconfig
{
    namespace test
    {
	namespace impl
	{
	    Bool boolValues[] = { TRUE, FALSE, TRUE };
	    int intValues[] = { 1, 2, 3 };
	    float floatValues[] = { 1.0, 2.0, 3.0 };
	    const char * stringValues[] = { "foo", "grill", "bar" };
	    const char * matchValues[] = { "type=foo", "class=bar", "xid=42" };

	    const unsigned int NUM_COLOR_VALUES = 3;

	    CCSSettingColorValue *
	    getColorValueList ()
	    {
		static const unsigned short max = std::numeric_limits <unsigned short>::max ();
		static const unsigned short maxD2 = max / 2;
		static const unsigned short maxD4 = max / 4;
		static const unsigned short maxD8 = max / 8;

		static bool colorValueListInitialized = false;

		static CCSSettingColorValue colorValues[NUM_COLOR_VALUES];

		if (!colorValueListInitialized)
		{
		    colorValues[0].color.red = maxD2;
		    colorValues[0].color.blue = maxD4;
		    colorValues[0].color.green = maxD8;
		    colorValues[0].color.alpha = max;

		    colorValues[1].color.red = maxD8;
		    colorValues[1].color.blue = maxD4;
		    colorValues[1].color.green = maxD2;
		    colorValues[1].color.alpha = max;

		    colorValues[1].color.red = max;
		    colorValues[1].color.blue = maxD4;
		    colorValues[1].color.green = maxD2;
		    colorValues[1].color.alpha = maxD8;

		    for (unsigned int i = 0; i < NUM_COLOR_VALUES; i++)
		    {
			CharacterWrapper s (ccsColorToString (&colorValues[i]));

			ccsStringToColor (s, &colorValues[i]);
		    }

		    colorValueListInitialized = true;
		}

		return colorValues;
	    }
	    CCSSettingKeyValue keyValue = { XK_A,
					    (1 << 0)};

	    CCSSettingButtonValue buttonValue = { 1,
						  (1 << 0),
						  (1 << 1) };

	    namespace populators
	    {
		namespace list
		{
		    CCSSettingValueList boolean (CCSSetting *setting)
		    {
			return ccsGetValueListFromBoolArray (boolValues, sizeof (boolValues) / sizeof (boolValues[0]), setting);
		    }

		    CCSSettingValueList integer (CCSSetting *setting)
		    {
			return ccsGetValueListFromIntArray (intValues, sizeof (intValues) / sizeof (intValues[0]), setting);
		    }

		    CCSSettingValueList doubleprecision (CCSSetting *setting)
		    {
			return ccsGetValueListFromFloatArray (floatValues, sizeof (floatValues) / sizeof (floatValues[0]), setting);
		    }

		    CCSSettingValueList string (CCSSetting *setting)
		    {
			return ccsGetValueListFromStringArray (stringValues, sizeof (stringValues) / sizeof (stringValues[0]), setting);
		    }

		    CCSSettingValueList match (CCSSetting *setting)
		    {
			return ccsGetValueListFromMatchArray (matchValues, sizeof (matchValues) / sizeof (matchValues[0]), setting);
		    }

		    CCSSettingValueList color (CCSSetting *setting)
		    {
			return ccsGetValueListFromColorArray (getColorValueList (), 3, setting);
		    }
		}
	    }
	}
    }
}

#endif
