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
			  const std::map <std::string, VariantTypes> &map)
	{
	    std::map <std::string, VariantTypes>::const_iterator it = map.find (key);

	    if (it != map.end ())
		return boost::get <T> (it->second);
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
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value)

	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteFloatAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteMatchAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteStringAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteColorAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteKeyAtKey (const std::string &plugin,
			    const std::string &key,
			    const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteButtonAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteEdgeAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteBellAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void WriteListAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[plugin + "/" + key] = value;
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

		    ccsSetBool (setting, boolToBool (ValueForKeyRetreival <bool> ().GetValueForKey (key, mValues)), FALSE);
		    break;

		case TypeInt:

		    ccsSetInt (setting, ValueForKeyRetreival <int> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeFloat:

		    ccsSetFloat (setting, ValueForKeyRetreival <float> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeString:

		    ccsSetString (setting, ValueForKeyRetreival <const char *> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeMatch:

		    ccsSetMatch (setting, ValueForKeyRetreival <const char *> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeColor:

		    ccsSetColor (setting, ValueForKeyRetreival <CCSSettingColorValue> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeKey:

		    ccsSetKey (setting, ValueForKeyRetreival <CCSSettingKeyValue> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeButton:

		    ccsSetButton (setting, ValueForKeyRetreival <CCSSettingButtonValue> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeEdge:

		    ccsSetEdge (setting, ValueForKeyRetreival <unsigned int> ().GetValueForKey (key, mValues), FALSE);
		    break;

		case TypeBell:

		    ccsSetBell (setting, boolToBool (ValueForKeyRetreival <bool> ().GetValueForKey (key, mValues)), FALSE);
		    break;

		case TypeList:

		    ccsSetList (setting, *(ValueForKeyRetreival <boost::shared_ptr <CCSListWrapper> > ().GetValueForKey (key, mValues)), FALSE);
		    break;

		default:

		    throw std::exception ();
	    }
	}

    private:

	CCSBackend *mBackend;
	CCSBackendGMock *mBackendGMock;
	std::map <std::string, VariantTypes> mValues;
};

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <MockCCSBackendConceptTestEnvironment> ());


