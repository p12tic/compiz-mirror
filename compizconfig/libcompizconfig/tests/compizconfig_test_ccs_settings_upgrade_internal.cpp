#include <tr1/tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <ccs.h>

#include "compizconfig_ccs_context_mock.h"
#include "compizconfig_ccs_plugin_mock.h"
#include "compizconfig_ccs_setting_mock.h"

#include "ccs_settings_upgrade_internal.h"
#include "gtest_shared_characterwrapper.h"
#include "gtest_shared_autodestroy.h"

using ::testing::IsNull;
using ::testing::Eq;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::_;

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

TEST (CCSSettingsUpgradeInternalTest, TestDetokenizeAndSetValues)
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

TEST (CCSSettingsUpgradeInternalTest, TestDetokenizeAndSetValuesReturnsFalseIfInvalid)
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

TEST (CCSSettingsUpgradeInternalTest, TestDetokenizeAndReturnTrueForUpgradeFileName)
{
    EXPECT_THAT (ccsUpgradeNameFilter (CCS_SETTINGS_UPGRADE_TEST_CORRECT_FILENAME.c_str ()), BoolTrue ());
}

TEST (CCSSettingsUpgradeInternalTest, TestDetokenizeAndReturnFalseForNoUpgradeFileName)
{
    EXPECT_THAT (ccsUpgradeNameFilter (CCS_SETTINGS_UPGRADE_TEST_INCORRECT_FILENAME.c_str ()), BoolFalse ());
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

	typedef enum _AddMode
	{
	    DoNotAddSettingToPlugin,
	    AddSettingToPlugin
	} AddMode;

	virtual void SetUp ()
	{
	    context = AutoDestroy <CCSContext> (ccsMockContextNew (),
						ccsFreeMockContext);
	    plugin = AutoDestroy <CCSPlugin> (ccsMockPluginNew (),
					      ccsFreeMockPlugin);

	    ON_CALL (MockPlugin (), getName ())
		    .WillByDefault (
			Return (

			    CCS_SETTINGS_UPGRADE_TEST_MOCK_PLUGIN_NAME.c_str ()));

	    ON_CALL (MockPlugin (), getContext ())
		    .WillByDefault (
			Return (
			    context.get ()));
	}

	CCSPluginGMock & MockPlugin ()
	{
	    return *(reinterpret_cast <CCSPluginGMock *> (ccsObjectGetPrivate (plugin.get ())));
	}

	CCSContextGMock & MockContext ()
	{
	    return *(reinterpret_cast <CCSContextGMock *> (ccsObjectGetPrivate (context.get ())));
	}

	void InitializeValueCommon (CCSSettingValue &value,
				    CCSSetting      *setting)
	{
	    value.parent = setting;
	    value.refCount = 1;
	}

	void InitializeValueForSetting (CCSSettingValue &value,
					CCSSetting      *setting)
	{
	    InitializeValueCommon (value, setting);
	    value.isListChild = FALSE;
	}


	MockedSetting
	SpawnSetting (const std::string &name,
		      CCSSettingType    type,
		      AddMode		addMode = AddSettingToPlugin)
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

	    ON_CALL (*gmockSetting, getParent ())
		    .WillByDefault (
			Return (
			    plugin.get ()));

	    if (addMode == AddSettingToPlugin)
	    {
		ON_CALL (MockPlugin (), findSetting (Eq (name.c_str ())))
			.WillByDefault (
			    Return (
				setting.get ()));
	    }

	    return MockedSetting (setting, gmockSetting);
	}

    private:

	boost::shared_ptr <CCSContext> context;
	boost::shared_ptr <CCSPlugin>  plugin;
};

TEST_F (CCSSettingsUpgradeTestWithMockContext, TestNoClearValuesSettingNotFound)
{
    MockedSetting settingOne (SpawnSetting (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE,
					    TypeInt,
					    DoNotAddSettingToPlugin));

    EXPECT_CALL (MockPlugin (), findSetting (Eq (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE)));
    EXPECT_CALL (Mock (settingOne), getParent ());
    EXPECT_CALL (Mock (settingOne), getName ());

    CCSSettingList list = ccsSettingListAppend (NULL, Real (settingOne));

    ccsUpgradeClearValues (list);
}

TEST_F (CCSSettingsUpgradeTestWithMockContext, TestClearValuesInListNonListType)
{
    MockedSetting resetSettingIdentifier (SpawnSetting (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE,
							TypeInt,
							DoNotAddSettingToPlugin));
    MockedSetting settingToReset (SpawnSetting (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE,
						TypeInt));
    CCSSettingValue valueToReset;
    CCSSettingValue valueResetIdentifier;

    InitializeValueForSetting (valueToReset, Real (settingToReset));
    InitializeValueForSetting (valueResetIdentifier, Real (resetSettingIdentifier));

    valueToReset.value.asInt = 7;
    valueResetIdentifier.value.asInt = 7;

    CCSSettingList list = ccsSettingListAppend (NULL, Real (resetSettingIdentifier));

    EXPECT_CALL (MockPlugin (), findSetting (Eq (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE)))
	    .WillOnce (Return (Real (settingToReset)));
    EXPECT_CALL (Mock (resetSettingIdentifier), getParent ());
    EXPECT_CALL (Mock (resetSettingIdentifier), getName ());

    CCSSettingInfo info;

    info.forInt.max = 0;
    info.forInt.min = 10;

    /* ccsCheckValueEq needs to know the type and info about this type */
    EXPECT_CALL (Mock (settingToReset), getType ()).Times (AtLeast (1));
    EXPECT_CALL (Mock (resetSettingIdentifier), getType ()).Times (AtLeast (1));

    EXPECT_CALL (Mock (settingToReset), getInfo ()).WillRepeatedly (Return (&info));
    EXPECT_CALL (Mock (resetSettingIdentifier), getInfo ()).WillRepeatedly (Return (&info));

    EXPECT_CALL (Mock (resetSettingIdentifier), getValue ()).WillOnce (Return (&valueResetIdentifier));
    EXPECT_CALL (Mock (settingToReset), getValue ()).WillOnce (Return (&valueToReset));

    EXPECT_CALL (Mock (settingToReset), resetToDefault (BoolTrue ()));

    ccsUpgradeClearValues (list);
}

namespace
{
    boost::shared_ptr <CCSString>
    newOwnedCCSStringFromStaticCharArray (const char *cStr)
    {
	CCSString		      *string = reinterpret_cast <CCSString *> (calloc (1, sizeof (CCSString)));
	boost::shared_ptr <CCSString> str (string,
					   ccsStringUnref);

	str->value = strdup (cStr);
	ccsStringRef (str.get ());
	return str;
    }

    void
    ccsStringValueListShallowFree (CCSSettingValueList list)
    {
	ccsSettingValueListFree (list, FALSE);
    }

}

TEST_F (CCSSettingsUpgradeTestWithMockContext, TestClearValuesInListRemovesValuesFromList)
{
    const std::string valueOne ("value_one");
    const std::string valueTwo ("value_two");
    const std::string valueThree ("value_three");
    MockedSetting resetSettingIdentifier (SpawnSetting (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE,
							TypeList,
							DoNotAddSettingToPlugin));
    MockedSetting settingToRemoveValuesFrom (SpawnSetting (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE,
							   TypeList));

    boost::shared_ptr <CCSString> stringForRemovalOne (newOwnedCCSStringFromStaticCharArray (valueOne.c_str ()));
    boost::shared_ptr <CCSString> stringForRemovalTwo (newOwnedCCSStringFromStaticCharArray (valueTwo.c_str ()));
    boost::shared_ptr <CCSString> stringNotRemoved (newOwnedCCSStringFromStaticCharArray (valueThree.c_str ()));

    CCSStringList settingStrList = ccsStringListAppend (NULL, stringForRemovalOne.get ());
    settingStrList = ccsStringListAppend (settingStrList, stringForRemovalTwo.get ());
    settingStrList = ccsStringListAppend (settingStrList, stringNotRemoved.get ());

    boost::shared_ptr <_CCSSettingValueList> settingStrValueList (AutoDestroy (ccsGetValueListFromStringList (settingStrList,
													      Real (settingToRemoveValuesFrom)),
									       ccsStringValueListShallowFree));

    CCSStringList removeStrList = ccsStringListAppend (NULL, stringForRemovalOne.get ());
    settingStrList = ccsStringListAppend (removeStrList, stringForRemovalTwo.get ());

    boost::shared_ptr <_CCSSettingValueList> removeStrValueList (AutoDestroy (ccsGetValueListFromStringList (removeStrList,
													     Real (resetSettingIdentifier)),
									      ccsStringValueListShallowFree));

    CCSSettingList list = ccsSettingListAppend (NULL, Real (resetSettingIdentifier));

    CCSSettingValue valueToHaveSubValuesRemoved;
    CCSSettingValue valueSubValuesResetIdentifiers;

    InitializeValueForSetting (valueToHaveSubValuesRemoved, Real (settingToRemoveValuesFrom));
    InitializeValueForSetting (valueSubValuesResetIdentifiers, Real (resetSettingIdentifier));

    valueToHaveSubValuesRemoved.value.asList = settingStrValueList.get ();
    valueSubValuesResetIdentifiers.value.asList = removeStrValueList.get ();

    CCSSettingInfo info;

    info.forList.listType = TypeString;

    EXPECT_CALL (MockPlugin (), findSetting (Eq (CCS_SETTINGS_UPGRADE_TEST_MOCK_SETTING_NAME_ONE)))
	    .WillOnce (Return (Real (settingToRemoveValuesFrom)));
    EXPECT_CALL (Mock (resetSettingIdentifier), getParent ());
    EXPECT_CALL (Mock (resetSettingIdentifier), getName ());
    EXPECT_CALL (Mock (settingToRemoveValuesFrom), getType ());
    EXPECT_CALL (Mock (resetSettingIdentifier), getValue ()).WillOnce (Return (&valueSubValuesResetIdentifiers));
    EXPECT_CALL (Mock (settingToRemoveValuesFrom), getValue ()).WillOnce (Return (&valueToHaveSubValuesRemoved));
    EXPECT_CALL (Mock (settingToRemoveValuesFrom), getInfo ()).WillRepeatedly (Return (&info));
    EXPECT_CALL (Mock (resetSettingIdentifier), getInfo ()).WillRepeatedly (Return (&info));

    EXPECT_CALL (Mock (settingToRemoveValuesFrom), setList (_, BoolTrue ()));

    ccsUpgradeClearValues (list);
}
