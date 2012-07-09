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

	void WriteStringAtKey (const std::string &plugin,
			       const std::string &key,
			       const std::string &value)
	{
	    mStringMap[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

    protected:

	void WriteValueToSetting (CCSSetting *setting)
	{
	    std::string key (std::string (ccsPluginGetName (ccsSettingGetParent (setting))) +
			     "/" + std::string (ccsSettingGetName (setting)));

	    switch (ccsSettingGetType (setting))
	    {
		case TypeInt:

		    ccsSetInt (setting, ValueForKeyRetreival <int> ().GetValueForKey (key, mIntMap), FALSE);
		    break;

		case TypeFloat:

		    ccsSetFloat (setting, ValueForKeyRetreival <float> ().GetValueForKey (key, mFloatMap), FALSE);
		    break;

		case TypeString:

		    ccsSetString (setting, ValueForKeyRetreival <char *> ().GetValueForKey (key, mStringMap), FALSE);
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
};

static MockCCSBackendConceptTestEnvironment mockBackendEnv;
static CCSBackendConceptTestEnvironmentInterface *backendEnv = &mockBackendEnv;

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest, ::testing::Values (backendEnv));


