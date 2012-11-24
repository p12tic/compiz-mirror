/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd
 * Copyright (C) 2012 Sam Spilsbury <smspillaz@gmail.com>
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
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gtest_shared_autodestroy.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>

#include <ccs.h>

#include "compizconfig_ccs_setting_mock.h"
#include "compizconfig_ccs_plugin_mock.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

TEST(CCSSettingTest, TestMock)
{
    CCSSetting *setting = ccsMockSettingNew ();
    CCSSettingGMock *mock = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    EXPECT_CALL (*mock, getName ());
    EXPECT_CALL (*mock, getShortDesc ());
    EXPECT_CALL (*mock, getLongDesc ());
    EXPECT_CALL (*mock, getType ());
    EXPECT_CALL (*mock, getInfo ());
    EXPECT_CALL (*mock, getGroup ());
    EXPECT_CALL (*mock, getSubGroup ());
    EXPECT_CALL (*mock, getHints ());
    EXPECT_CALL (*mock, getDefaultValue ());
    EXPECT_CALL (*mock, getValue ());
    EXPECT_CALL (*mock, getIsDefault ());
    EXPECT_CALL (*mock, getParent ());
    EXPECT_CALL (*mock, getPrivatePtr ());
    EXPECT_CALL (*mock, setPrivatePtr (_));
    EXPECT_CALL (*mock, setInt (_, _));
    EXPECT_CALL (*mock, setFloat (_, _));
    EXPECT_CALL (*mock, setBool (_, _));
    EXPECT_CALL (*mock, setString (_, _));
    EXPECT_CALL (*mock, setColor (_, _));
    EXPECT_CALL (*mock, setMatch (_, _));
    EXPECT_CALL (*mock, setKey (_, _));
    EXPECT_CALL (*mock, setButton (_, _));
    EXPECT_CALL (*mock, setEdge (_, _));
    EXPECT_CALL (*mock, setBell (_, _));
    EXPECT_CALL (*mock, setList (_, _));
    EXPECT_CALL (*mock, setValue (_, _));
    EXPECT_CALL (*mock, getInt (_));
    EXPECT_CALL (*mock, getFloat (_));
    EXPECT_CALL (*mock, getBool (_));
    EXPECT_CALL (*mock, getString (_));
    EXPECT_CALL (*mock, getColor (_));
    EXPECT_CALL (*mock, getMatch (_));
    EXPECT_CALL (*mock, getKey (_));
    EXPECT_CALL (*mock, getButton (_));
    EXPECT_CALL (*mock, getEdge (_));
    EXPECT_CALL (*mock, getBell (_));
    EXPECT_CALL (*mock, getList (_));
    EXPECT_CALL (*mock, resetToDefault (_));
    EXPECT_CALL (*mock, isIntegrated ());
    EXPECT_CALL (*mock, isReadOnly ());
    EXPECT_CALL (*mock, isReadableByBackend ());

    CCSSettingColorValue cv;
    CCSSettingKeyValue kv;
    CCSSettingButtonValue bv;
    CCSSettingValueList lv = NULL;

    ccsSettingGetName (setting);
    ccsSettingGetShortDesc (setting);
    ccsSettingGetLongDesc (setting);
    ccsSettingGetType (setting);
    ccsSettingGetInfo (setting);
    ccsSettingGetGroup (setting);
    ccsSettingGetSubGroup (setting);
    ccsSettingGetHints (setting);
    ccsSettingGetDefaultValue (setting);
    ccsSettingGetValue (setting);
    ccsSettingGetIsDefault (setting);
    ccsSettingGetParent (setting);
    ccsSettingGetPrivatePtr (setting);
    ccsSettingSetPrivatePtr (setting, NULL);
    ccsSetInt (setting, 1, FALSE);
    ccsSetFloat (setting, 1.0, FALSE);
    ccsSetBool (setting, TRUE, FALSE);
    ccsSetString (setting, "foo", FALSE);
    ccsSetColor (setting, cv, FALSE);
    ccsSetMatch (setting, "bar", FALSE);
    ccsSetKey (setting, kv, FALSE);
    ccsSetButton (setting, bv, FALSE);
    ccsSetEdge (setting, 1, FALSE);
    ccsSetBell (setting, TRUE, FALSE);
    ccsSetList (setting, lv, FALSE);
    ccsSetValue (setting, NULL, FALSE);
    ccsGetInt (setting, NULL);
    ccsGetFloat (setting, NULL);
    ccsGetBool (setting, NULL);
    ccsGetString (setting, NULL);
    ccsGetColor (setting, NULL);
    ccsGetMatch (setting, NULL);
    ccsGetKey (setting, NULL);
    ccsGetButton (setting, NULL);
    ccsGetEdge (setting, NULL);
    ccsGetBell (setting, NULL);
    ccsGetList (setting, NULL);
    ccsResetToDefault (setting, FALSE);
    ccsSettingIsIntegrated (setting);
    ccsSettingIsReadOnly (setting);
    ccsSettingIsReadableByBackend (setting);

    ccsSettingUnref (setting);
}

namespace
{
class MockInitializerFuncs
{
    public:

	MOCK_METHOD3 (initializeInfo, void (CCSSettingType, CCSSettingInfo *, void *));
	MOCK_METHOD4 (initializeDefaultValue, void (CCSSettingType, CCSSettingInfo *, CCSSettingValue *, void *));

	static void wrapInitializeInfo (CCSSettingType type,
					CCSSettingInfo *info,
					void           *data)
	{
	    MockInitializerFuncs &funcs (*(reinterpret_cast <MockInitializerFuncs *> (data)));
	    funcs.initializeInfo (type, info, data);
	}

	static void wrapInitializeValue (CCSSettingType  type,
					 CCSSettingInfo  *info,
					 CCSSettingValue *value,
					 void            *data)
	{
	    MockInitializerFuncs &funcs (*(reinterpret_cast <MockInitializerFuncs *> (data)));
	    funcs.initializeDefaultValue (type, info, value, data);
	}
};

const std::string SETTING_NAME = "setting_name";
const std::string SETTING_SHORT_DESC = "Short Description";
const std::string SETTING_LONG_DESC = "Long Description";
const std::string SETTING_GROUP = "Group";
const std::string SETTING_SUBGROUP = "Sub Group";
const std::string SETTING_HINTS = "Hints";
const CCSSettingType SETTING_TYPE = TypeInt;
}

class CCSSettingDefaultImplTest :
    public ::testing::Test
{
    public:

	CCSSettingDefaultImplTest () :
	    plugin (AutoDestroy (ccsMockPluginNew (),
				 ccsPluginUnref)),
	    gmockPlugin (reinterpret_cast <CCSPluginGMock *> (ccsObjectGetPrivate (plugin.get ())))
	{
	}

	virtual void SetUpSetting (MockInitializerFuncs &funcs)
	{
	    void                 *vFuncs = reinterpret_cast <void *> (&funcs);

	    /* Info must be initialized before the value */
	    InSequence s;

	    EXPECT_CALL (funcs, initializeInfo (GetSettingType (), _, vFuncs));
	    EXPECT_CALL (funcs, initializeDefaultValue (GetSettingType (), _, _, vFuncs));

	    setting = AutoDestroy (ccsSettingDefaultImplNew (plugin.get (),
							     SETTING_NAME.c_str (),
							     GetSettingType (),
							     SETTING_SHORT_DESC.c_str (),
							     SETTING_LONG_DESC.c_str (),
							     SETTING_HINTS.c_str (),
							     SETTING_GROUP.c_str (),
							     SETTING_SUBGROUP.c_str (),
							     MockInitializerFuncs::wrapInitializeValue,
							     vFuncs,
							     MockInitializerFuncs::wrapInitializeInfo,
							     vFuncs,
							     &ccsDefaultObjectAllocator,
							     &ccsDefaultInterfaceTable),
				   ccsSettingUnref);
	}

	virtual void SetUp ()
	{
	    MockInitializerFuncs funcs;

	    SetUpSetting (funcs);

	}

	virtual CCSSettingType GetSettingType ()
	{
	    return SETTING_TYPE;
	}

	boost::shared_ptr <CCSPlugin>  plugin;
	CCSPluginGMock                 *gmockPlugin;
	boost::shared_ptr <CCSSetting> setting;
};

TEST_F (CCSSettingDefaultImplTest, Construction)
{
    EXPECT_EQ (SETTING_TYPE, ccsSettingGetType (setting.get ()));
    EXPECT_EQ (SETTING_NAME, ccsSettingGetName (setting.get ()));
    EXPECT_EQ (SETTING_SHORT_DESC, ccsSettingGetShortDesc (setting.get ()));
    EXPECT_EQ (SETTING_LONG_DESC, ccsSettingGetLongDesc (setting.get ()));
    EXPECT_EQ (SETTING_GROUP, ccsSettingGetGroup (setting.get ()));
    EXPECT_EQ (SETTING_SUBGROUP, ccsSettingGetSubGroup (setting.get ()));
    EXPECT_EQ (SETTING_HINTS, ccsSettingGetHints (setting.get ()));

    /* Pointer Equality */
    EXPECT_EQ (ccsSettingGetDefaultValue (setting.get ()),
	       ccsSettingGetValue (setting.get ()));

    /* Actual Equality */
    EXPECT_TRUE (ccsCheckValueEq (ccsSettingGetDefaultValue (setting.get ()),
				  ccsSettingGetType (setting.get ()),
				  ccsSettingGetInfo (setting.get ()),
				  ccsSettingGetValue (setting.get ()),
				  ccsSettingGetType (setting.get ()),
				  ccsSettingGetInfo (setting.get ())));

    EXPECT_EQ (ccsSettingGetParent (setting.get ()),
	       plugin.get ());
}
