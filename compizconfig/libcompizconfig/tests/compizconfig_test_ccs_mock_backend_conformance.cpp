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
using ::testing::Combine;
using ::testing::ValuesIn;
using ::testing::Values;

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
			     const VariantTypes &value)
	{
	    mBoolMap[plugin + "/" + key] = boolToBool (boost::get <bool> (value));
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value)

	{
	    mIntMap[plugin + "/" + key] = boost::get <int> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteFloatAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mFloatMap[plugin + "/" + key] = boost::get <float> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteMatchAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mMatchMap[plugin + "/" + key] = boost::get <const char *> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteStringAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mStringMap[plugin + "/" + key] = boost::get <const char *> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteColorAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mColorMap[plugin + "/" + key] = boost::get <CCSSettingColorValue> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteKeyAtKey (const std::string &plugin,
			    const std::string &key,
			    const VariantTypes &value)
	{
	    mKeyMap[plugin + "/" + key] = boost::get <CCSSettingKeyValue> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteButtonAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mButtonMap[plugin + "/" + key] = boost::get <CCSSettingButtonValue> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteEdgeAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mEdgeMap[plugin + "/" + key] = boost::get <unsigned int> (value);
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteBellAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mBellMap[plugin + "/" + key] = boolToBool (boost::get <bool> (value));
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteListAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mListMap[plugin + "/" + key] = *(boost::get <boost::shared_ptr <CCSListWrapper> > (value));
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

		case TypeList:

		    ccsSetList (setting, ValueForKeyRetreival <CCSSettingValueList> ().GetValueForKey (key, mListMap), FALSE);
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
	std::map <std::string, CCSSettingValueList> mListMap;
};

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <MockCCSBackendConceptTestEnvironment> ());


