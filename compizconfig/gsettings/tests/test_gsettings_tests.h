#include <gtest/gtest.h>

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

class CCSGSettingsTest :
    public CCSGSettingsTestingEnv,
    public ::testing::TestWithParam <CCSGSettingsTeardownSetupInterface *>
{
    public:

	CCSGSettingsTest () :
	    mFuncs (GetParam ())
	{
	}

	virtual void SetUp ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();
	    mFuncs->SetUp ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestingEnv::TearDownEnv ();
	    mFuncs->TearDown ();
	}

    private:

	CCSGSettingsTeardownSetupInterface *mFuncs;
};

class CCSGSettingsTestIndependent :
    public CCSGSettingsTestingEnv,
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestingEnv::TearDownEnv ();
	}
};

