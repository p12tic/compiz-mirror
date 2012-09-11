#ifndef _COMPIZCONFIG_TEST_GSETTINGS_TESTS_H
#define _COMPIZCONFIG_TEST_GSETTINGS_TESTS_H

#include <gtest/gtest.h>
#include <glib.h>
#include <glib-object.h>
#include <glib_gslice_off_env.h>
#include <glib_gsettings_memory_backend_env.h>
#include <gsettings-mock-schemas-config.h>

using ::testing::TestWithParam;

class CCSGSettingsTeardownSetupInterface
{
    public:
	virtual void SetUp () = 0;
	virtual void TearDown () = 0;
};

class CCSGSettingsTestingEnv
{
    public:

	virtual void SetUpEnv ()
	{
	    setenv ("G_SLICE", "always-malloc", 1);
	}

	virtual void TearDownEnv ()
	{
	    unsetenv ("G_SLICE");
	}
};

class CCSGSettingsMemoryBackendTestingEnv :
    public CCSGSettingsTestingEnv
{
    public:

	virtual void SetUpEnv ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();

	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", 1);
	}

	virtual void TearDownEnv ()
	{
	    g_unsetenv ("GSETTINGS_BACKEND");
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");

	    CCSGSettingsTestingEnv::TearDownEnv ();
	}
};

class CCSGSettingsTestCommon :
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    env.SetUpEnv ();
	}

	virtual void TearDown ()
	{
	    env.TearDownEnv ();
	}

    private:

	CompizGLibGSliceOffEnv env;
};

class CCSGSettingsTest :
    public CCSGSettingsTestCommon,
    public ::testing::WithParamInterface <CCSGSettingsTeardownSetupInterface *>
{
    public:

	CCSGSettingsTest () :
	    mFuncs (GetParam ())
	{
	}

	virtual void SetUp ()
	{
	    CCSGSettingsTestCommon::SetUp ();
	    mFuncs->SetUp ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestCommon::TearDown ();
	    mFuncs->TearDown ();
	}

    private:

	CCSGSettingsTeardownSetupInterface *mFuncs;
};

class CCSGSettingsTestIndependent :
    public CCSGSettingsTestCommon
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestCommon::SetUp ();
	    g_type_init ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestCommon::TearDown ();
	}
};

class CCSGSettingsTestWithMemoryBackend :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestIndependent::SetUp ();
	    env.SetUpEnv (MOCK_PATH);
	}

	virtual void TearDown ()
	{
	    env.TearDownEnv ();
	}
    private:

	CompizGLibGSettingsMemoryBackendTestingEnv env;
};

#endif
