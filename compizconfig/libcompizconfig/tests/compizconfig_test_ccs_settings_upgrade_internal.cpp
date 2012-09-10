#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ccs_settings_upgrade_internal.h"
#include "gtest_shared_characterwrapper.h"

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
