#include <gtest/gtest.h>

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

