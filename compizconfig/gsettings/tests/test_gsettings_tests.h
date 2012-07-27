#include <gtest/gtest.h>

using ::testing::TestWithParam;

class CCSGSettingsTeardownSetupInterface
{
    public:
	virtual void SetUp () = 0;
	virtual void TearDown () = 0;
};

class CCSGSettingsTestGeneral
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

class CCSGSettingsTest :
    public CCSGSettingsTestGeneral,
    public ::testing::TestWithParam <CCSGSettingsTeardownSetupInterface *>
{
    public:

	CCSGSettingsTest () :
	    mFuncs (GetParam ())
	{
	}

	virtual void SetUp ()
	{
	    CCSGSettingsTestGeneral::SetUpEnv ();
	    mFuncs->SetUp ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestGeneral::TearDownEnv ();
	    mFuncs->TearDown ();
	}

    private:

	CCSGSettingsTeardownSetupInterface *mFuncs;
};

class CCSGSettingsTestIndependent :
    public CCSGSettingsTestGeneral,
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestGeneral::SetUpEnv ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestGeneral::TearDownEnv ();
	}
};

