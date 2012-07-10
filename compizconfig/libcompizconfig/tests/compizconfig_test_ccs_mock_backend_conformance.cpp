#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs.h>

#include <compizconfig_ccs_backend_mock.h>
#include <compizconfig_backend_concept_test.h>

#include "compizconfig_ccs_backend_mock.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::WithArgs;

template <typename T>
class ValueForKeyRetreival
{
    public:

	T GetValueForKey (const std::string &key,
			  const std::map <std::string, T> &map)
	{
	    typename std::map <std::string, T>::const_iterator it = map.find (key);

	    if (it != map.end ())
		return it->second;
	    else
		throw std::exception ();
	}
};

template <>
class ValueForKeyRetreival <char *>
{
    public:

	const char * GetValueForKey (const std::string &key,
				     const std::map <std::string, std::string> &map)
	{
	    std::map <std::string, std::string>::const_iterator it = map.find (key);

	    if (it != map.end ())
		return it->second.c_str ();
	    else
		throw std::exception ();
	}
};

class MockCCSBackendConceptTestEnvironment :
    public CCSBackendConceptTestEnvironmentInterface
{
    public:

	CCSBackend * SetUp ()
	{
	    mBackend = ccsMockBackendNew ();
	    mBackendGMock = (CCSBackendGMock *) ccsObjectGetPrivate (mBackend);

	    ON_CALL (*mBackendGMock, readSetting (_,_))
		    .WillByDefault (
			WithArgs<1> (
			    Invoke (
				this,
				&MockCCSBackendConceptTestEnvironment::WriteValueToSetting)));

	    return mBackend;
	}

	void TearDown (CCSBackend *backend)
	{
	    ccsFreeMockBackend (backend);


	}

	void WriteBoolAtKey (const std::string &plugin,
			     const std::string &key,
			     const Bool &value)
	{
	    mBoolMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				int value)

	{
	    mIntMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteFloatAtKey (const std::string &plugin,
			      const std::string &key,
			      float value)
	{
	    mFloatMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteMatchAtKey (const std::string &plugin,
			      const std::string &key,
			      const std::string &value)
	{
	    mMatchMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteStringAtKey (const std::string &plugin,
			       const std::string &key,
			       const std::string &value)
	{
	    mStringMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteColorAtKey (const std::string &plugin,
			      const std::string &key,
			      const CCSSettingColorValue &value)
	{
	    mColorMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteKeyAtKey (const std::string &plugin,
			    const std::string &key,
			    const CCSSettingKeyValue &keyValue)
	{
	    mKeyMap[plugin + "/" + key] = keyValue;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteButtonAtKey (const std::string &plugin,
			       const std::string &key,
			       const CCSSettingButtonValue &buttonValue)
	{
	    mButtonMap[plugin + "/" + key] = buttonValue;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteEdgeAtKey (const std::string &plugin,
			     const std::string &key,
			     const unsigned int &value)
	{
	    mEdgeMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteBellAtKey (const std::string &plugin,
			     const std::string &key,
			     const Bool &value)
	{
	    mBellMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

    protected:

	void WriteValueToSetting (CCSSetting *setting)
	{
	    std::string key (std::string (ccsPluginGetName (ccsSettingGetParent (setting))) +
			     "/" + std::string (ccsSettingGetName (setting)));

	    switch (ccsSettingGetType (setting))
	    {
		case TypeBool:

		    ccsSetBool (setting, ValueForKeyRetreival <Bool> ().GetValueForKey (key, mBoolMap), FALSE);
		    break;

		case TypeInt:

		    ccsSetInt (setting, ValueForKeyRetreival <int> ().GetValueForKey (key, mIntMap), FALSE);
		    break;

		case TypeFloat:

		    ccsSetFloat (setting, ValueForKeyRetreival <float> ().GetValueForKey (key, mFloatMap), FALSE);
		    break;

		case TypeString:

		    ccsSetString (setting, ValueForKeyRetreival <char *> ().GetValueForKey (key, mStringMap), FALSE);
		    break;

		case TypeMatch:

		    ccsSetMatch (setting, ValueForKeyRetreival <char *> ().GetValueForKey (key, mMatchMap), FALSE);
		    break;

		case TypeColor:

		    ccsSetColor (setting, ValueForKeyRetreival <CCSSettingColorValue> ().GetValueForKey (key, mColorMap), FALSE);
		    break;

		case TypeKey:

		    ccsSetKey (setting, ValueForKeyRetreival <CCSSettingKeyValue> ().GetValueForKey (key, mKeyMap), FALSE);
		    break;

		case TypeButton:

		    ccsSetButton (setting, ValueForKeyRetreival <CCSSettingButtonValue> ().GetValueForKey (key, mButtonMap), FALSE);
		    break;

		case TypeEdge:

		    ccsSetEdge (setting, ValueForKeyRetreival <unsigned int> ().GetValueForKey (key, mEdgeMap), FALSE);
		    break;

		case TypeBell:

		    ccsSetBell (setting, ValueForKeyRetreival <Bool> ().GetValueForKey (key, mBellMap), FALSE);
		    break;

		default:

		    throw std::exception ();
	    }
	}

    private:

	CCSBackend *mBackend;
	CCSBackendGMock *mBackendGMock;
	std::map <std::string, int> mIntMap;
	std::map <std::string, float> mFloatMap;
	std::map <std::string, std::string> mStringMap;
	std::map <std::string, std::string> mMatchMap;
	std::map <std::string, CCSSettingColorValue> mColorMap;
	std::map <std::string, CCSSettingKeyValue> mKeyMap;
	std::map <std::string, CCSSettingButtonValue> mButtonMap;
	std::map <std::string, unsigned int> mEdgeMap;
	std::map <std::string, Bool> mBoolMap;
	std::map <std::string, Bool> mBellMap;
};

static MockCCSBackendConceptTestEnvironment mockBackendEnv;
static CCSBackendConceptTestEnvironmentInterface *backendEnv = &mockBackendEnv;

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest, ::testing::Values (backendEnv));


