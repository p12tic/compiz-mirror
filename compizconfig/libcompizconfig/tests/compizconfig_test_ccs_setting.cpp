#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gtest_shared_autodestroy.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>

#include <X11/Xlib.h>

#include <ccs.h>

#include "compizconfig_ccs_setting_mock.h"
#include "compizconfig_ccs_plugin_mock.h"
#include "compizconfig_ccs_list_wrapper.h"

namespace cci = compiz::config::impl;
namespace cc = compiz::config;


using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNull;

class CCSSettingTest :
    public ::testing::Test
{
};

TEST(CCSSettingTest, TestMock)
{
    CCSSetting *setting = ccsMockSettingNew ();
    CCSSettingGMock *mock = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    EXPECT_CALL (*mock, getName ());
    EXPECT_CALL (*mock, getShortDesc ());
    EXPECT_CALL (*mock, getLongDesc ());
    EXPECT_CALL (*mock, getType ());
    EXPECT_CALL (*mock, getInfo ());
    EXPECT_CALL (*mock, getGroup ());
    EXPECT_CALL (*mock, getSubGroup ());
    EXPECT_CALL (*mock, getHints ());
    EXPECT_CALL (*mock, getDefaultValue ());
    EXPECT_CALL (*mock, getValue ());
    EXPECT_CALL (*mock, getIsDefault ());
    EXPECT_CALL (*mock, getParent ());
    EXPECT_CALL (*mock, getPrivatePtr ());
    EXPECT_CALL (*mock, setPrivatePtr (_));
    EXPECT_CALL (*mock, setInt (_, _));
    EXPECT_CALL (*mock, setFloat (_, _));
    EXPECT_CALL (*mock, setBool (_, _));
    EXPECT_CALL (*mock, setString (_, _));
    EXPECT_CALL (*mock, setColor (_, _));
    EXPECT_CALL (*mock, setMatch (_, _));
    EXPECT_CALL (*mock, setKey (_, _));
    EXPECT_CALL (*mock, setButton (_, _));
    EXPECT_CALL (*mock, setEdge (_, _));
    EXPECT_CALL (*mock, setBell (_, _));
    EXPECT_CALL (*mock, setList (_, _));
    EXPECT_CALL (*mock, setValue (_, _));
    EXPECT_CALL (*mock, getInt (_));
    EXPECT_CALL (*mock, getFloat (_));
    EXPECT_CALL (*mock, getBool (_));
    EXPECT_CALL (*mock, getString (_));
    EXPECT_CALL (*mock, getColor (_));
    EXPECT_CALL (*mock, getMatch (_));
    EXPECT_CALL (*mock, getKey (_));
    EXPECT_CALL (*mock, getButton (_));
    EXPECT_CALL (*mock, getEdge (_));
    EXPECT_CALL (*mock, getBell (_));
    EXPECT_CALL (*mock, getList (_));
    EXPECT_CALL (*mock, resetToDefault (_));
    EXPECT_CALL (*mock, isIntegrated ());
    EXPECT_CALL (*mock, isReadOnly ());
    EXPECT_CALL (*mock, isReadableByBackend ());

    CCSSettingColorValue cv;
    CCSSettingKeyValue kv;
    CCSSettingButtonValue bv;
    CCSSettingValueList lv = NULL;

    ccsSettingGetName (setting);
    ccsSettingGetShortDesc (setting);
    ccsSettingGetLongDesc (setting);
    ccsSettingGetType (setting);
    ccsSettingGetInfo (setting);
    ccsSettingGetGroup (setting);
    ccsSettingGetSubGroup (setting);
    ccsSettingGetHints (setting);
    ccsSettingGetDefaultValue (setting);
    ccsSettingGetValue (setting);
    ccsSettingGetIsDefault (setting);
    ccsSettingGetParent (setting);
    ccsSettingGetPrivatePtr (setting);
    ccsSettingSetPrivatePtr (setting, NULL);
    ccsSetInt (setting, 1, FALSE);
    ccsSetFloat (setting, 1.0, FALSE);
    ccsSetBool (setting, TRUE, FALSE);
    ccsSetString (setting, "foo", FALSE);
    ccsSetColor (setting, cv, FALSE);
    ccsSetMatch (setting, "bar", FALSE);
    ccsSetKey (setting, kv, FALSE);
    ccsSetButton (setting, bv, FALSE);
    ccsSetEdge (setting, 1, FALSE);
    ccsSetBell (setting, TRUE, FALSE);
    ccsSetList (setting, lv, FALSE);
    ccsSetValue (setting, NULL, FALSE);
    ccsGetInt (setting, NULL);
    ccsGetFloat (setting, NULL);
    ccsGetBool (setting, NULL);
    ccsGetString (setting, NULL);
    ccsGetColor (setting, NULL);
    ccsGetMatch (setting, NULL);
    ccsGetKey (setting, NULL);
    ccsGetButton (setting, NULL);
    ccsGetEdge (setting, NULL);
    ccsGetBell (setting, NULL);
    ccsGetList (setting, NULL);
    ccsResetToDefault (setting, FALSE);
    ccsSettingIsIntegrated (setting);
    ccsSettingIsReadOnly (setting);
    ccsSettingIsReadableByBackend (setting);

    ccsSettingUnref (setting);
}

namespace
{

typedef boost::shared_ptr <CCSSettingInfo> CCSSettingInfoPtr;
typedef boost::shared_ptr <CCSSettingValue> CCSSettingValuePtr;
typedef boost::shared_ptr <CCSSetting> CCSSettingPtr;

/* Used to copy different raw values */
template <typename SettingValueType>
class CopyRawValueBase
{
    public:

	CopyRawValueBase (const SettingValueType &value) :
	    mValue (value)
	{
	}

    protected:

	const SettingValueType &mValue;
};

template <typename SettingValueType>
class CopyRawValue :
    public CopyRawValueBase <SettingValueType>
{
    public:

	typedef SettingValueType ReturnType;
	typedef CopyRawValueBase <SettingValueType> Parent;

	CopyRawValue (const SettingValueType &value) :
	    CopyRawValueBase <SettingValueType> (value)
	{
	}

	SettingValueType operator () ()
	{
	    return Parent::mValue;
	}
};

template <>
class CopyRawValue <const char *> :
    public CopyRawValueBase <const char *>
{
    public:

	typedef const char * ReturnType;
	typedef CopyRawValueBase <const char *> Parent;

	CopyRawValue (const char * value) :
	    CopyRawValueBase <const char *> (value)
	{
	}

	ReturnType operator () ()
	{
	    /* XXX: Valgrind complains here that mValue is uninitialized, but
	     * verification using gdb confirms that isn't true */
	    return strdup (Parent::mValue);
	}
};

template <>
class CopyRawValue <cci::SettingValueListWrapper::Ptr> :
    public CopyRawValueBase <cci::SettingValueListWrapper::Ptr>
{
    public:

	typedef CCSSettingValueList ReturnType;
	typedef CopyRawValueBase <cci::SettingValueListWrapper::Ptr> Parent;

	CopyRawValue (const cci::SettingValueListWrapper::Ptr &value) :
	    CopyRawValueBase <cci::SettingValueListWrapper::Ptr> (value)
	{
	}

	ReturnType operator () ()
	{
	    return ccsCopyList (*Parent::mValue,
				Parent::mValue->setting ().get ());
	}
};

CCSSettingValue *
NewCCSSettingValue ()
{
    CCSSettingValue *value =
	    reinterpret_cast <CCSSettingValue *> (
		calloc (1, sizeof (CCSSettingValue)));

    value->refCount = 1;

    return value;
}

template <typename SettingValueType>
CCSSettingValue *
RawValueToCCSValue (const SettingValueType &value)
{
    typedef typename CopyRawValue <SettingValueType>::ReturnType UnionType;

    CCSSettingValue  *settingValue = NewCCSSettingValue ();
    UnionType *unionMember =
	    reinterpret_cast <UnionType *> (&settingValue->value);

    *unionMember = (CopyRawValue <SettingValueType> (value)) ();

    return settingValue;
}

template <typename SettingValueType>
CCSSettingValuePtr
RawValueToListValue (const SettingValueType &value)
{
    CCSSettingValue     *listChild = RawValueToCCSValue (value);

    listChild->isListChild = TRUE;

    CCSSettingValueList valueListHead = ccsSettingValueListAppend (NULL, listChild);
    CCSSettingValuePtr  valueListValue (AutoDestroy (NewCCSSettingValue (),
						     ccsSettingValueUnref));

    valueListValue->value.asList = valueListHead;

    return valueListValue;
}

class ContainedValueGenerator
{
    public:

	template <typename SettingValueType>
	const CCSSettingValuePtr &
	SpawnValueForInfoAndType (const SettingValueType  &rawValue,
				  CCSSettingType          type,
				  const CCSSettingInfoPtr &info)
	{
	    const CCSSettingPtr &setting (GetSetting (type, info));
	    CCSSettingValuePtr  value (AutoDestroy (RawValueToCCSValue <SettingValueType> (rawValue),
						    ccsSettingValueUnref));

	    value->parent = setting.get ();
	    mContainedValues.push_back (value);

	    return mContainedValues.back ();
	}

	const CCSSettingPtr &
	GetSetting (CCSSettingType          type,
		    const CCSSettingInfoPtr &info)
	{
	    if (!mSetting)
		SetupMockSetting (type, info);

	    return mSetting;
	}

    private:

	void SetupMockSetting (CCSSettingType          type,
			       const CCSSettingInfoPtr &info)
	{
	    mSetting = AutoDestroy (ccsMockSettingNew (),
				    ccsSettingUnref);

	    CCSSettingGMock *settingMock =
		    reinterpret_cast <CCSSettingGMock *> (
			ccsObjectGetPrivate (mSetting.get ()));

	    EXPECT_CALL (*settingMock, getType ())
		.WillRepeatedly (Return (type));

	    EXPECT_CALL (*settingMock, getInfo ())
		.WillRepeatedly (Return (info.get ()));

	    EXPECT_CALL (*settingMock, getDefaultValue ())
		.WillRepeatedly (ReturnNull ());
	}

	/* This must always be before the value
	 * as the values hold a weak reference to
	 * it */
	CCSSettingPtr                    mSetting;
	std::vector <CCSSettingValuePtr> mContainedValues;

};

/* ValueContainer Interface */
template <typename SettingValueType>
class ValueContainer
{
    public:

	typedef boost::shared_ptr <ValueContainer> Ptr;

	virtual const SettingValueType &
	getRawValue (CCSSettingType          type,
		     const CCSSettingInfoPtr &info) = 0;
	virtual const CCSSettingValuePtr &
	getContainedValue (CCSSettingType          type,
			   const CCSSettingInfoPtr &info) = 0;
};

template <typename SettingValueType>
class NormalValueContainer :
    public ValueContainer <SettingValueType>
{
    public:

	NormalValueContainer (const SettingValueType &value) :
	    mRawValue (value)
	{
	}

	const SettingValueType &
	getRawValue (CCSSettingType          type,
		     const CCSSettingInfoPtr &info)
	{
	    return mRawValue;
	}

	const CCSSettingValuePtr &
	getContainedValue (CCSSettingType          type,
			   const CCSSettingInfoPtr &info)
	{
	    if (!mContainedValue)
		mContainedValue = mContainedValueGenerator.SpawnValueForInfoAndType (mRawValue,
										     type,
										     info);

	    return mContainedValue;
	}

    private:

	ContainedValueGenerator  mContainedValueGenerator;
	const SettingValueType   &mRawValue;
	CCSSettingValuePtr       mContainedValue;
};

template <typename SettingValueType>
typename NormalValueContainer <SettingValueType>::Ptr
ContainNormal (const SettingValueType &value)
{
    return boost::make_shared <NormalValueContainer <SettingValueType> > (value);
}

template <typename SettingValueType>
class ListValueContainer :
    public ValueContainer <CCSSettingValueList>
{
    public:

	ListValueContainer (const SettingValueType &value) :
	    mRawChildValue (value)
	{
	}

	const CCSSettingValueList &
	getRawValue (CCSSettingType          type,
		     const CCSSettingInfoPtr &info)
	{
	    const cci::SettingValueListWrapper::Ptr &wrapper (SetupWrapper (type, info));

	    return *wrapper;
	}

	const CCSSettingValuePtr &
	getContainedValue (CCSSettingType          type,
			   const CCSSettingInfoPtr &info)
	{
	    if (!mContainedWrapper)
	    {
		const cci::SettingValueListWrapper::Ptr &wrapper (SetupWrapper (type, info));

		mContainedWrapper =
			mContainedValueGenerator.SpawnValueForInfoAndType (wrapper,
									   type,
									   info);
	    }

	    return mContainedWrapper;
	}

    private:

	const cci::SettingValueListWrapper::Ptr &
	SetupWrapper (CCSSettingType          type,
		      const CCSSettingInfoPtr &info)
	{
	    if (!mWrapper)
	    {
		const CCSSettingPtr &setting (mContainedValueGenerator.GetSetting (type, info));
		CCSSettingValue     *value = RawValueToCCSValue (mRawChildValue);

		value->parent = setting.get ();
		value->isListChild = TRUE;
		mWrapper.reset (new cci::SettingValueListWrapper (NULL,
								  cci::Deep,
								  type,
								  info,
								  setting));
		mWrapper->append (value);
	    }

	    return mWrapper;
	}

	typedef cc::ListWrapper <CCSSettingValueList, CCSSettingValue *> SVLInterface;

	cci::SettingValueListWrapper::Ptr mWrapper;

	/* ccsFreeSettingValue has an implicit
	 * dependency on mWrapper (CCSSettingValue -> CCSSetting ->
	 * CCSSettingInfo -> cci::SettingValueListWrapper), these should
	 * be kept after mWrapper here */
	ContainedValueGenerator           mContainedValueGenerator;
	CCSSettingValuePtr                mContainedWrapper;
	const SettingValueType            &mRawChildValue;
};

template <typename SettingValueType>
typename ValueContainer <CCSSettingValueList>::Ptr
ContainList (const SettingValueType &value)
{
    return boost::make_shared <ListValueContainer <SettingValueType> > (value);
}
}
