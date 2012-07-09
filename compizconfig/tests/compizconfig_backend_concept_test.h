#ifndef _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST
#define _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST

#include <list>

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



#endif

