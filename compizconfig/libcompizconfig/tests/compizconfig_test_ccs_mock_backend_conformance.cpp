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

namespace
{
std::string keynameFromPluginKey (const std::string &plugin,
				  const std::string &key)
{
    return plugin + "/" + key;
}
}

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

	    ON_CALL (*mBackendGMock, readSetting (_, _))
		    .WillByDefault (
			WithArgs<1> (
			    Invoke (
				this,
				&MockCCSBackendConceptTestEnvironment::ReadValueIntoSetting)));

	    ON_CALL (*mBackendGMock, writeSetting (_, _))
		    .WillByDefault (
			WithArgs<1> (
			    Invoke (
				this,
				&MockCCSBackendConceptTestEnvironment::WriteValueToMap)));

	    return mBackend;
	}

	void TearDown (CCSBackend *backend)
	{
	    ccsFreeMockBackend (backend);

	}

	void PreWrite ()
	{
	    EXPECT_CALL (*mBackendGMock, writeSetting (_, _));
	}

	void PostWrite ()
	{
	}

	void WriteBoolAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value)

	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteFloatAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteMatchAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteStringAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteColorAtKey (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteKeyAtKey (const std::string &plugin,
			    const std::string &key,
			    const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteButtonAtKey (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteEdgeAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteBellAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void WriteListAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mValues[keynameFromPluginKey (plugin, key)] = value;
	}

	void PreRead ()
	{
	    EXPECT_CALL (*mBackendGMock, readSetting (_, _));
	}

	void PostRead ()
	{
	}

	virtual Bool ReadBoolAtKey (const std::string &plugin,
				    const std::string &key)
	{
	    return boolToBool (ValueForKeyRetreival <bool> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues));
	}

	virtual int ReadIntegerAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    return ValueForKeyRetreival <int> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    return ValueForKeyRetreival <float> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual const char * ReadStringAtKey (const std::string &plugin,
					      const std::string &key)
	{
	    return ValueForKeyRetreival <const char *> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
						     const std::string &key)
	{
	    return ValueForKeyRetreival <CCSSettingColorValue> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
						 const std::string &key)
	{
	    return ValueForKeyRetreival <CCSSettingKeyValue> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
						       const std::string &key)
	{
	    return ValueForKeyRetreival <CCSSettingButtonValue> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return ValueForKeyRetreival <unsigned int> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual const char * ReadMatchAtKey (const std::string &plugin,
					     const std::string &key)
	{
	    return ValueForKeyRetreival <const char *> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues);
	}

	virtual Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return boolToBool (ValueForKeyRetreival <bool> ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues));
	}

	virtual CCSSettingValueList ReadListAtKey (const std::string &plugin,
						   const std::string &key)
	{
	    return *(ValueForKeyRetreival <boost::shared_ptr <CCSListWrapper> > ().GetValueForKey (keynameFromPluginKey (plugin, key), mValues));
	}

    protected:

	void ReadValueIntoSetting (CCSSetting *setting)
	{
	    std::string plugin (ccsPluginGetName (ccsSettingGetParent (setting)));
	    std::string key (ccsSettingGetName (setting));

	    switch (ccsSettingGetType (setting))
	    {
		case TypeBool:

		    ccsSetBool (setting, ReadBoolAtKey (plugin, key), FALSE);
		    break;

		case TypeInt:

		    ccsSetInt (setting, ReadIntegerAtKey (plugin, key), FALSE);
		    break;

		case TypeFloat:

		    ccsSetFloat (setting, ReadFloatAtKey (plugin, key), FALSE);
		    break;

		case TypeString:

		    ccsSetString (setting, ReadStringAtKey (plugin, key), FALSE);
		    break;

		case TypeMatch:

		    ccsSetMatch (setting, ReadMatchAtKey (plugin, key), FALSE);
		    break;

		case TypeColor:

		    ccsSetColor (setting, ReadColorAtKey (plugin, key), FALSE);
		    break;

		case TypeKey:

		    ccsSetKey (setting, ReadKeyAtKey (plugin, key), FALSE);
		    break;

		case TypeButton:

		    ccsSetButton (setting, ReadButtonAtKey (plugin, key), FALSE);
		    break;

		case TypeEdge:

		    ccsSetEdge (setting, ReadEdgeAtKey (plugin, key), FALSE);
		    break;

		case TypeBell:

		    ccsSetBell (setting, ReadBellAtKey (plugin, key), FALSE);
		    break;

		case TypeList:

		    ccsSetList (setting, ReadListAtKey (plugin, key), FALSE);
		    break;

		default:

		    throw std::exception ();
	    }
	}

	void WriteValueToMap (CCSSetting *setting)
	{
	    std::string plugin (ccsPluginGetName (ccsSettingGetParent (setting)));
	    std::string key (ccsSettingGetName (setting));

	    Bool vBool;
	    int  vInt;
	    float vFloat;
	    char *vString;
	    CCSSettingColorValue vColor;
	    CCSSettingKeyValue vKey;
	    CCSSettingButtonValue vButton;
	    unsigned int vEdge;
	    CCSSettingValueList vList;

	    switch (ccsSettingGetType (setting))
	    {
		case TypeBool:

		    ccsGetBool (setting, &vBool);
		    WriteBoolAtKey (plugin, key, VariantTypes (vBool ? true : false));
		    break;

		case TypeInt:

		    ccsGetInt (setting, &vInt);
		    WriteIntegerAtKey (plugin, key, VariantTypes (vInt));
		    break;

		case TypeFloat:

		    ccsGetFloat (setting, &vFloat);
		    WriteFloatAtKey (plugin, key, VariantTypes (vFloat));
		    break;

		case TypeString:

		    ccsGetString (setting, &vString);
		    WriteStringAtKey (plugin, key, VariantTypes (static_cast <const char *> (vString)));
		    break;

		case TypeMatch:

		    ccsGetMatch (setting, &vString);
		    WriteStringAtKey (plugin, key, VariantTypes (static_cast <const char *> (vString)));
		    break;

		case TypeColor:

		    ccsGetColor (setting, &vColor);
		    WriteColorAtKey (plugin, key, VariantTypes (vColor));
		    break;

		case TypeKey:

		    ccsGetKey (setting, &vKey);
		    WriteKeyAtKey (plugin, key, VariantTypes (vKey));
		    break;

		case TypeButton:

		    ccsGetButton (setting, &vButton);
		    WriteButtonAtKey (plugin, key, VariantTypes (vButton));
		    break;

		case TypeEdge:

		    ccsGetEdge (setting, &vEdge);
		    WriteEdgeAtKey (plugin, key, VariantTypes (vEdge));
		    break;

		case TypeBell:

		    ccsGetBell (setting, &vBool);
		    WriteBellAtKey (plugin, key, VariantTypes (vBool ? true : false));
		    break;

		case TypeList:

		    ccsGetList (setting, &vList);
		    WriteListAtKey (plugin, key, VariantTypes (boost::make_shared <CCSListWrapper> (vList, false)));
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


