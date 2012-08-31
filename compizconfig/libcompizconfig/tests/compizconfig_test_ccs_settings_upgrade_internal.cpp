#include <tr1/tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "compizconfig_ccs_context_mock.h"
#include "compizconfig_ccs_plugin_mock.h"
#include "compizconfig_ccs_setting_mock.h"

#include "ccs_settings_upgrade_internal.h"
#include "gtest_shared_characterwrapper.h"
#include "gtest_shared_autodestroy.h"

using ::testing::IsNull;

class CCSSettingsUpgradeInternalTest :
    public ::testing::Test
{
};

namespace
{
    static const std::string CCS_SETTINGS_UPGRADE_TEST_CORRECT_FILENAME = "org.compiz.general.1.upgrade";
    static const std::string CCS_SETTINGS_UPGRADE_TEST_INCORRECT_FILENAME = "1.upgra";
    static const std::string CCS_SETTINGS_UPGRADE_TEST_CORRECT_DOMAIN = "org.compiz";
    static const std::string CCS_SETTINGS_UPGRADE_TEST_CORRECT_PROFILE = "general";
    static const unsigned int CCS_SETTINGS_UPGRADE_TEST_CORRECT_NUM = 1;
}

MATCHER(BoolTrue, "Bool True") { if (arg) return true; else return false; }
MATCHER(BoolFalse, "Bool False") { if (!arg) return true; else return false; }

TEST_F (CCSSettingsUpgradeInternalTest, TestDetokenizeAndSetValues)
{
    char *profileName = NULL;
    char *domainName = NULL;

    unsigned int num;

    EXPECT_THAT (ccsUpgradeGetDomainNumAndProfile (CCS_SETTINGS_UPGRADE_TEST_CORRECT_FILENAME.c_str (),
						   &domainName,
						   &num,
						   &profileName), BoolTrue ());

    CharacterWrapper profileNameC (profileName);
    CharacterWrapper domainNameC (domainName);

    EXPECT_EQ (CCS_SETTINGS_UPGRADE_TEST_CORRECT_PROFILE, profileName);
    EXPECT_EQ (CCS_SETTINGS_UPGRADE_TEST_CORRECT_DOMAIN, domainName);
    EXPECT_EQ (num, CCS_SETTINGS_UPGRADE_TEST_CORRECT_NUM);
}

TEST_F (CCSSettingsUpgradeInternalTest, TestDetokenizeAndSetValuesReturnsFalseIfInvalid)
{
    char *profileName = NULL;
    char *domainName = NULL;

    unsigned int num;

    EXPECT_THAT (ccsUpgradeGetDomainNumAndProfile (CCS_SETTINGS_UPGRADE_TEST_INCORRECT_FILENAME.c_str (),
						   &domainName,
						   &num,
						   &profileName), BoolFalse ());

    EXPECT_THAT (profileName, IsNull ());
    EXPECT_THAT (domainName, IsNull ());
}

TEST_F (CCSSettingsUpgradeInternalTest, TestDetokenizeAndReturnTrueForUpgradeFileName)
{
    EXPECT_THAT (ccsUpgradeNameFilter (CCS_SETTINGS_UPGRADE_TEST_CORRECT_FILENAME.c_str ()), BoolTrue ());
}

TEST_F (CCSSettingsUpgradeInternalTest, TestDetokenizeAndReturnFalseForNoUpgradeFileName)
{
    EXPECT_THAT (ccsUpgradeNameFilter (CCS_SETTINGS_UPGRADE_TEST_CORRECT_FILENAME.c_str ()), BoolTrue ());
}

namespace
{
    const std::string CCS_SETTINGS_UPGRADE_TEST_MOCK_PLUGIN_NAME = "mock";
    const std::string CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE = "setting_one";
    const std::string CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_TWO = "setting_two";

    typedef std::tr1::tuple <boost::shared_ptr <CCSSetting>,
			     CCSSettingGMock *> MockedSetting;

    inline CCSSetting      * Real (MockedSetting &ms) { return (std::tr1::get <0> (ms)).get (); }
    inline CCSSettingGMock & Mock (MockedSetting &ms) { return *(std::tr1::get <1> (ms)); }
}

class CCSSettingsUpgradeTestWithMockContext :
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    context = AutoDestroy <CCSContext> (ccsMockContextNew (),
						ccsFreeMockContext);
	    plugin = AutoDestroy <CCSPlugin> (ccsMockPluginNew (),
					      ccsFreeMockPlugin);

	    CCSPluginGMock *gmockPlugin = reinterpret_cast <CCSPluginGMock *> (ccsObjectGetPrivate (plugin.get ()));

	    ON_CALL (*gmockPlugin, getName ())
		    .WillByDefault (
			Return (
			    CCS_SETTINGS_UPGRADE_TEST_MOCK_PLUGIN_NAME.c_str ()));

	    ON_CALL (*gmockPlugin, getContext ())
		    .WillByDefault (
			Return (
			    context.get ()));
	}

	MockedSetting
	SpawnSetting (const std::string &name,
		      CCSSettingType    type)
	{
	    boost::shared_ptr <CCSSetting> setting (ccsMockSettingNew (),
						    ccsSettingUnref);

	    CCSSettingGMock *gmockSetting = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (setting.get ()));

	    ON_CALL (*gmockSetting, getName ())
		    .WillByDefault (
			Return (
			    name.c_str ()));
	    ON_CALL (*gmockSetting, getType ())
		    .WillByDefault (
			Return (
			    type));

	    return MockedSetting (setting, gmockSetting);
	}

    private:

	boost::shared_ptr <CCSContext> context;
	boost::shared_ptr <CCSPlugin>  plugin;
};

TEST_F (CCSSettingsUpgradeTestWithMockContext, TestClearValuesInListNonListType)
{

}
