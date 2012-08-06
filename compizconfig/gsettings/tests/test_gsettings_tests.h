#ifndef _COMPIZCONFIG_TEST_GSETTINGS_TESTS_H
#define _COMPIZCONFIG_TEST_GSETTINGS_TESTS_H

#include <gtest/gtest.h>
#include <glib.h>
#include <glib-object.h>

#include <gsettings-mock-schemas-config.h>

using ::testing::TestWithParam;

class CCSGSettingsTeardownSetupInterface
{
    public:
	virtual void SetUp () = 0;
	virtual void TearDown () = 0;
};

class CCSGSettingsTest :
    public ::testing::TestWithParam <CCSGSettingsTeardownSetupInterface *>
{
    public:

	CCSGSettingsTest () :
	    mFuncs (GetParam ())
	{
	}

	virtual void SetUp ()
	{
	    mFuncs->SetUp ();
	}

	virtual void TearDown ()
	{
	    mFuncs->TearDown ();
	}

    private:

	CCSGSettingsTeardownSetupInterface *mFuncs;
};

class CCSGSettingsTestIndependent :
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    g_type_init ();

	    g_setenv ("G_SLICE", "always-malloc", 1);
	}

	virtual void TearDown ()
	{
	    g_unsetenv ("G_SLICE");
	}
};

class CCSGSettingsTestWithMemoryBackend :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestIndependent::SetUp ();
	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", 1);
	}

	virtual void TearDown ()
	{
	    g_unsetenv ("GSETTINGS_BACKEND");
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");
	    CCSGSettingsTestIndependent::TearDown ();
	}
};

#endif
