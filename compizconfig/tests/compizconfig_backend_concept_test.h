#ifndef _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST
#define _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST

#include <list>

#include <boost/scoped_array.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-backend.h>
#include <ccs.h>

#include <compizconfig_ccs_plugin_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_context_mock.h>

class CCSBackendConceptTestEnvironmentInterface
{
    public:

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp () = 0;
	virtual void TearDown (CCSBackend *) = 0;
	virtual void WriteIntegerAtKey (const std::string &plugin,
					const std::string &key,
					int		   value) = 0;
	virtual void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      float		   value) = 0;
	virtual void WriteStringAtKey (const std::string &plugin,
				       const std::string &key,
				       const std::string &value) = 0;
	virtual void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const CCSSettingColorValue &value) = 0;
	virtual void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const CCSSettingKeyValue &value) = 0;
	virtual void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const CCSSettingButtonValue &value) = 0;
	virtual void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const unsigned int &value) = 0;
	virtual void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const Bool &value) = 0;

	virtual void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const CCSSettingValueList &list) = 0;
};

class CCSBackendConformanceTest :
    public ::testing::TestWithParam <CCSBackendConceptTestEnvironmentInterface *>
{
    public:

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	void SetUp ()
	{
	    mBackend = GetParam ()->SetUp ();
	}

	void TearDown ()
	{
	    GetParam ()->TearDown (mBackend);

	    for (std::list <CCSContext *>::iterator it = mSpawnedContexts.begin ();
		 it != mSpawnedContexts.end ();
		 it++)
		ccsFreeMockContext (*it);

	    for (std::list <CCSPlugin *>::iterator it = mSpawnedPlugins.begin ();
		 it != mSpawnedPlugins.end ();
		 it++)
		ccsFreeMockPlugin (*it);

	    for (std::list <CCSSetting *>::iterator it = mSpawnedSettings.begin ();
		 it != mSpawnedSettings.end ();
		 it++)
		ccsFreeMockSetting (*it);
	}

    protected:

	CCSContext *
	SpawnContext ()
	{
	    CCSContext *context = ccsMockContextNew ();
	    mSpawnedContexts.push_back (context);
	    return context;
	}

	CCSPlugin *
	SpawnPlugin (const std::string &name = "")
	{
	    CCSPlugin *plugin = ccsMockPluginNew ();
	    mSpawnedPlugins.push_back (plugin);

	    CCSPluginGMock *gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin);

	    if (!name.empty ())
		EXPECT_CALL (*gmockPlugin, getName ()).WillRepeatedly (Return ((char *) name.c_str ()));

	    return plugin;
	}

	CCSSetting *
	SpawnSetting (const std::string &name = "",
		      CCSSettingType	type = TypeNum,
		      CCSPlugin		*plugin = NULL)
	{
	    CCSSetting *setting = ccsMockSettingNew ();
	    mSpawnedSettings.push_back (setting);

	    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

	    if (!name.empty ())
		EXPECT_CALL (*gmockSetting, getName ()).WillRepeatedly (Return ((char *) name.c_str ()));

	    if (type != TypeNum)
		EXPECT_CALL (*gmockSetting, getType ()).WillRepeatedly (Return (type));

	    if (plugin)
		EXPECT_CALL (*gmockSetting, getParent ()).WillRepeatedly (Return (plugin));

	    return setting;
	}

    private:

	std::list <CCSContext *> mSpawnedContexts;
	std::list <CCSPlugin  *> mSpawnedPlugins;
	std::list <CCSSetting *> mSpawnedSettings;

	CCSBackend *mBackend;
};

TEST_P (CCSBackendConformanceTest, TestSetup)
{
}

TEST_P (CCSBackendConformanceTest, TestReadInteger)
{
    std::string pluginName ("plugin");
    std::string settingName ("integer_setting");
    const int VALUE = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeInt, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteIntegerAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setInt (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadFloat)
{
    std::string pluginName ("plugin");
    std::string settingName ("float_setting");
    const float VALUE = 1.0;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeFloat, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteFloatAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setFloat (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadString)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    const std::string VALUE ("foo");

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeString, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteStringAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setString (VALUE.c_str (), _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

namespace
{
    bool
    operator== (const CCSSettingColorValue &lhs,
		const CCSSettingColorValue &rhs)
    {
	return (lhs.color.red == rhs.color.red &&
		lhs.color.green == rhs.color.green &&
		lhs.color.blue == rhs.color.blue &&
		lhs.color.alpha == lhs.color.alpha);
    }
}

TEST_P (CCSBackendConformanceTest, TestReadColor)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingColorValue VALUE;

    VALUE.color.red = 100;
    VALUE.color.blue = 100;
    VALUE.color.green = 100;
    VALUE.color.alpha = 100;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeColor, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteColorAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the color returned is correct */
    EXPECT_CALL (*gmockSetting, setColor (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadKey)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingKeyValue VALUE;

    VALUE.keyModMask = (1 << 0) | (1 << 1);
    VALUE.keysym = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeKey, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteKeyAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the key returned is correct */
    EXPECT_CALL (*gmockSetting, setKey (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadButton)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingButtonValue VALUE;

    VALUE.buttonModMask = (1 << 0) | (1 << 1);
    VALUE.button = 1;
    VALUE.edgeMask = (1 << 0);

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeButton, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteButtonAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the button returned is correct */
    EXPECT_CALL (*gmockSetting, setButton (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadEdge)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    unsigned int VALUE;

    VALUE = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeEdge, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteEdgeAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setEdge (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadBell)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    Bool VALUE = TRUE;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeBell, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteBellAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setBell (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadListBool)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    Bool		VBool[3] = { FALSE, TRUE, FALSE };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromBoolArray (VBool, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListInt)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    int		VInt[3] = { 1, 2, 3 };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromIntArray (VInt, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListFloat)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    float		VFloat[3] = { 1.0, 2.0, 3.0 };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromFloatArray (VFloat, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListString)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    const char *	VString[3] = { "foo",
				       "bar",
				       "baz" };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromStringArray (VString, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListMatch)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    const char *	VMatch[3] = { "foo",
				      "bar",
				      "baz" };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromMatchArray (VMatch, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListColor)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    CCSSettingColorValue VColor[3] = { { .color = { 100, 200, 300, 100 } },
					{ .color = { 50, 100, 200, 300 } },
					{ .color = { 10, 20, 30, 40 } }
				     };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromColorArray (VColor, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}




#endif

