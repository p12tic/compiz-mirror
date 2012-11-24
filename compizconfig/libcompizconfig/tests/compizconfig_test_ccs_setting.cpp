/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd
 * Copyright (C) 2012 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
using ::testing::InSequence;
using ::testing::WithArgs;
using ::testing::WithParamInterface;
using ::testing::Invoke;
using ::testing::ReturnNull;

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

class MockInitializerFuncs
{
    public:

	MOCK_METHOD3 (initializeInfo, void (CCSSettingType, CCSSettingInfo *, void *));
	MOCK_METHOD4 (initializeDefaultValue, void (CCSSettingType, CCSSettingInfo *, CCSSettingValue *, void *));

	static void wrapInitializeInfo (CCSSettingType type,
					CCSSettingInfo *info,
					void           *data)
	{
	    MockInitializerFuncs &funcs (*(reinterpret_cast <MockInitializerFuncs *> (data)));
	    funcs.initializeInfo (type, info, data);
	}

	static void wrapInitializeValue (CCSSettingType  type,
					 CCSSettingInfo  *info,
					 CCSSettingValue *value,
					 void            *data)
	{
	    MockInitializerFuncs &funcs (*(reinterpret_cast <MockInitializerFuncs *> (data)));
	    funcs.initializeDefaultValue (type, info, value, data);
	}
};

const std::string SETTING_NAME = "setting_name";
const std::string SETTING_SHORT_DESC = "Short Description";
const std::string SETTING_LONG_DESC = "Long Description";
const std::string SETTING_GROUP = "Group";
const std::string SETTING_SUBGROUP = "Sub Group";
const std::string SETTING_HINTS = "Hints";
const CCSSettingType SETTING_TYPE = TypeInt;
}

class CCSSettingDefaultImplTest :
    public ::testing::Test
{
    public:

	CCSSettingDefaultImplTest () :
	    plugin (AutoDestroy (ccsMockPluginNew (),
				 ccsPluginUnref)),
	    gmockPlugin (reinterpret_cast <CCSPluginGMock *> (ccsObjectGetPrivate (plugin.get ())))
	{
	}

	virtual void SetUpSetting (MockInitializerFuncs &funcs)
	{
	    void                 *vFuncs = reinterpret_cast <void *> (&funcs);

	    /* Info must be initialized before the value */
	    InSequence s;

	    EXPECT_CALL (funcs, initializeInfo (GetSettingType (), _, vFuncs));
	    EXPECT_CALL (funcs, initializeDefaultValue (GetSettingType (), _, _, vFuncs));

	    setting = AutoDestroy (ccsSettingDefaultImplNew (plugin.get (),
							     SETTING_NAME.c_str (),
							     GetSettingType (),
							     SETTING_SHORT_DESC.c_str (),
							     SETTING_LONG_DESC.c_str (),
							     SETTING_HINTS.c_str (),
							     SETTING_GROUP.c_str (),
							     SETTING_SUBGROUP.c_str (),
							     MockInitializerFuncs::wrapInitializeValue,
							     vFuncs,
							     MockInitializerFuncs::wrapInitializeInfo,
							     vFuncs,
							     &ccsDefaultObjectAllocator,
							     &ccsDefaultInterfaceTable),
				   ccsSettingUnref);
	}

	virtual void SetUp ()
	{
	    MockInitializerFuncs funcs;

	    SetUpSetting (funcs);

	}

	virtual CCSSettingType GetSettingType ()
	{
	    return SETTING_TYPE;
	}

	boost::shared_ptr <CCSPlugin>  plugin;
	CCSPluginGMock                 *gmockPlugin;
	boost::shared_ptr <CCSSetting> setting;
};

TEST_F (CCSSettingDefaultImplTest, Construction)
{
    EXPECT_EQ (SETTING_TYPE, ccsSettingGetType (setting.get ()));
    EXPECT_EQ (SETTING_NAME, ccsSettingGetName (setting.get ()));
    EXPECT_EQ (SETTING_SHORT_DESC, ccsSettingGetShortDesc (setting.get ()));
    EXPECT_EQ (SETTING_LONG_DESC, ccsSettingGetLongDesc (setting.get ()));
    EXPECT_EQ (SETTING_GROUP, ccsSettingGetGroup (setting.get ()));
    EXPECT_EQ (SETTING_SUBGROUP, ccsSettingGetSubGroup (setting.get ()));
    EXPECT_EQ (SETTING_HINTS, ccsSettingGetHints (setting.get ()));

    /* Pointer Equality */
    EXPECT_EQ (ccsSettingGetDefaultValue (setting.get ()),
	       ccsSettingGetValue (setting.get ()));

    /* Actual Equality */
    EXPECT_TRUE (ccsCheckValueEq (ccsSettingGetDefaultValue (setting.get ()),
				  ccsSettingGetType (setting.get ()),
				  ccsSettingGetInfo (setting.get ()),
				  ccsSettingGetValue (setting.get ()),
				  ccsSettingGetType (setting.get ()),
				  ccsSettingGetInfo (setting.get ())));

    EXPECT_EQ (ccsSettingGetParent (setting.get ()),
	       plugin.get ());
}

namespace
{

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

CCSSettingValuePtr
ListValueToSettingValueList (CCSSettingValue *listChild)
{
    listChild->isListChild = TRUE;

    CCSSettingValueList valueListHead = ccsSettingValueListAppend (NULL, listChild);
    CCSSettingValuePtr  valueListValue (AutoDestroy (NewCCSSettingValue (),
						     ccsSettingValueUnref));

    valueListValue->value.asList = valueListHead;

    return valueListValue;
}

template <typename SettingValueType>
CCSSettingValuePtr
RawValueToListValue (const SettingValueType &value)
{
    return ListValueToSettingValueList (RawValueToCCSValue (value));
}

class ContainedValueGenerator
{
    private:

	const CCSSettingValuePtr &
	InitializedSpawnedValue (const CCSSettingValuePtr &value,
				 CCSSettingType           type,
				 const CCSSettingInfoPtr  &info)
	{
	    const CCSSettingPtr &setting (GetSetting (type, info));
	    value->parent = setting.get ();
	    mContainedValues.push_back (value);

	    return mContainedValues.back ();
	}

    public:

	template <typename SettingValueType>
	const CCSSettingValuePtr &
	SpawnValueForInfoAndType (const SettingValueType  &rawValue,
				  CCSSettingType          type,
				  const CCSSettingInfoPtr &info)
	{

	    CCSSettingValuePtr  value (AutoDestroy (RawValueToCCSValue <SettingValueType> (rawValue),
						    ccsSettingValueUnref));

	    return InitializedSpawnedValue (value, type, info);
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

class NormalValueContainerBase
{
    protected:

	ContainedValueGenerator  mContainedValueGenerator;
	CCSSettingValuePtr       mContainedValue;
};

template <typename SettingValueType>
class NormalValueContainer :
    public NormalValueContainerBase,
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

	const SettingValueType   &mRawValue;
};

template <typename SettingValueType>
typename NormalValueContainer <SettingValueType>::Ptr
ContainNormal (const SettingValueType &value)
{
    return boost::make_shared <NormalValueContainer <SettingValueType> > (value);
}

class ListValueContainerBase :
    public ValueContainer <CCSSettingValueList>
{
    protected:

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

	const CCSSettingValueList &
	getRawValue (CCSSettingType          type,
		     const CCSSettingInfoPtr &info)
	{
	    const cci::SettingValueListWrapper::Ptr &wrapper (SetupWrapper (type, info));

	    return *wrapper;
	}

	cci::SettingValueListWrapper::Ptr mWrapper;

	/* ccsFreeSettingValue has an implicit
	 * dependency on mWrapper (CCSSettingValue -> CCSSetting ->
	 * CCSSettingInfo -> cci::SettingValueListWrapper), these should
	 * be kept after mWrapper here */
	ContainedValueGenerator           mContainedValueGenerator;
	CCSSettingValuePtr                mContainedWrapper;

    private:

	const cci::SettingValueListWrapper::Ptr &
	SetupWrapper (CCSSettingType          type,
		      const CCSSettingInfoPtr &info)
	{
	    if (!mWrapper)
	    {
		const CCSSettingPtr &setting (mContainedValueGenerator.GetSetting (type, info));
		CCSSettingValue     *value = GetValueForListWrapper ();

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

	virtual CCSSettingValue * GetValueForListWrapper () = 0;
};

template <typename SettingValueType>
class ListValueContainer :
    public ListValueContainerBase
{
    public:

	ListValueContainer (const SettingValueType &value) :
	    mRawChildValue (value)
	{
	}

    private:

	CCSSettingValue * GetValueForListWrapper ()
	{
	    return RawValueToCCSValue (mRawChildValue);
	}

	const SettingValueType  &mRawChildValue;
};

template <typename SettingValueType>
typename ValueContainer <CCSSettingValueList>::Ptr
ContainList (const SettingValueType &value)
{
    return boost::make_shared <ListValueContainer <SettingValueType> > (value);
}

class DefaultImplSetParamInterface
{
    public:

	typedef boost::shared_ptr <DefaultImplSetParamInterface> Ptr;
	typedef boost::function <void (MockInitializerFuncs &funcs)> SetUpSettingFunc;

	virtual ~DefaultImplSetParamInterface () {};

	virtual void SetUpSetting (const SetUpSettingFunc &func) = 0;
	virtual void TearDownSetting () = 0;
	virtual CCSSettingType GetSettingType () = 0;
	virtual void         SetUpParam (const CCSSettingPtr &) = 0;
	virtual Bool setWithInvalidType () = 0;
	virtual Bool setToFailValue () = 0;
	virtual Bool setToNonDefaultValue () = 0;
	virtual Bool setToDefaultValue () = 0;
};

void stubInitializeSettingInfo (CCSSettingType type,
				CCSSettingInfo *copyToInfo,
				void           *data)
{
    CCSSettingInfo *copyFromInfo = reinterpret_cast <CCSSettingInfo *> (data);

    ccsCopyInfo (copyFromInfo, copyToInfo, type);
}

void stubInitializeSettingDefaultValue (CCSSettingType  type,
					CCSSettingInfo  *info,
					CCSSettingValue *copyToValue,
					void           *data)
{
    CCSSettingValue *copyFromValue = reinterpret_cast <CCSSettingValue *> (data);
    CCSSetting      *oldParentForOtherValue = copyFromValue->parent;

    /* Change the parent to this setting that's being initialized
     * as that needs to go into the setting's default value as
     * the parent entry */
    copyFromValue->parent = copyToValue->parent;
    ccsCopyValueInto (copyFromValue, copyToValue, type, info);

    /* Restore the old parent */
    copyFromValue->parent = oldParentForOtherValue;
}

class MockInitializerFuncsWithDelegators :
    public MockInitializerFuncs
{
    public:

	MockInitializerFuncsWithDelegators (CCSSettingInfo  *info,
					    CCSSettingValue *value) :
	    MockInitializerFuncs (),
	    mInfo (info),
	    mValue (value)
	{
	    ON_CALL (*this, initializeInfo (_, _, _))
		    .WillByDefault (WithArgs <0, 1> (
					Invoke (this,
						&MockInitializerFuncsWithDelegators::initializeInfoDelegator)));

	    ON_CALL (*this, initializeDefaultValue (_, _, _, _))
		    .WillByDefault (WithArgs <0, 1, 2> (
					Invoke (this,
						&MockInitializerFuncsWithDelegators::initializeValueDelegator)));
	}

	void initializeInfoDelegator (CCSSettingType type,
				      CCSSettingInfo *info)
	{
	    stubInitializeSettingInfo (type, info, reinterpret_cast <void *> (mInfo));
	}

	void initializeValueDelegator (CCSSettingType  type,
				       CCSSettingInfo  *info,
				       CCSSettingValue *value)
	{
	    stubInitializeSettingDefaultValue (type, info, value,
					       reinterpret_cast <void *> (mValue));
	}

	CCSSettingInfo  *mInfo;
	CCSSettingValue *mValue;
};

template <typename SettingValueType>
struct SettingMutators
{
    typedef Bool (*SetFunction) (CCSSetting *setting,
				 SettingValueType data,
				 Bool);
    typedef Bool (*GetFunction) (CCSSetting *setting,
				 SettingValueType *);
};

class DefaultImplSetParamBase :
    public DefaultImplSetParamInterface
{
    public:

	DefaultImplSetParamBase (const CCSSettingInfoPtr &info,
				 CCSSettingType          type) :
	    mInfo (info),
	    mType (type)
	{
	}

	virtual void TearDownSetting ()
	{
	    if (mSetting)
		setToDefaultValue ();
	}

	void InitializeDefaultsForSetting (const SetUpSettingFunc &func)
	{
	    MockInitializerFuncsWithDelegators mockInitializers (mInfo.get (), mValue.get ());

	    func (mockInitializers);
	}

	void TakeReferenceToCreatedSetting (const CCSSettingPtr &setting)
	{
	    mSetting = setting;
	}

	const CCSSettingInterface * RedirectSettingInterface ()
	{
	    const CCSSettingInterface *settingInterface =
		GET_INTERFACE (CCSSettingInterface, mSetting.get ());
	    CCSSettingInterface tmpSettingInterface = *settingInterface;

	    tmpSettingInterface.settingGetType =
		DefaultImplSetParamBase::returnIncorrectSettingType;

	    ccsObjectRemoveInterface (mSetting.get (),
				      GET_INTERFACE_TYPE (CCSSettingInterface));
	    ccsObjectAddInterface (mSetting.get (),
				   (const CCSInterface *) &tmpSettingInterface,
				   GET_INTERFACE_TYPE (CCSSettingInterface));

	    return settingInterface;
	}

	void RestoreSettingInterface (const CCSSettingInterface *settingInterface)
	{
	    /* Restore the old interface */
	    ccsObjectRemoveInterface (mSetting.get (),
				      GET_INTERFACE_TYPE (CCSSettingInterface));
	    ccsObjectAddInterface (mSetting.get (),
				   (const CCSInterface *) settingInterface,
				   GET_INTERFACE_TYPE (CCSSettingInterface));
	}

	virtual Bool setToFailValue ()
	{
	    return TRUE;
	}

	virtual CCSSettingType GetSettingType ()
	{
	    return mType;
	}

    protected:

	CCSSettingInfoPtr  mInfo;
	CCSSettingValuePtr mValue;
	CCSSettingType     mType;
	CCSSettingPtr      mSetting;

    private:

	static const CCSSettingType incorrectSettingType = TypeNum;
	static CCSSettingType returnIncorrectSettingType (CCSSetting *setting)
	{
	    return incorrectSettingType;
	}
};

class RequireSettingInterfaceRedirection
{
    public:

	RequireSettingInterfaceRedirection (DefaultImplSetParamBase *base) :
	    mBase (base),
	    mSettingInterface (mBase->RedirectSettingInterface ())
	{
	}

	~RequireSettingInterfaceRedirection ()
	{
	    mBase->RestoreSettingInterface (mSettingInterface);
	}

    private:

	DefaultImplSetParamBase   *mBase;
	const CCSSettingInterface *mSettingInterface;
};

template <typename SettingValueType>
class DefaultImplSetParamTemplatedBase
{
    protected:

	typedef typename ValueContainer <SettingValueType>::Ptr ValueContainerPtr;

	DefaultImplSetParamTemplatedBase (const ValueContainerPtr &defaultValueContainer,
					  const ValueContainerPtr &nonDefaultValueContainer) :
	    mDefaultValueContainer (defaultValueContainer),
	    mNonDefaultValueContainer (nonDefaultValueContainer)
	{
	}

	ValueContainerPtr  mDefaultValueContainer;
	ValueContainerPtr  mNonDefaultValueContainer;
};

template <typename SettingValueType>
class DefaultImplSetParam :
    /* Do not change the order of inheritance here, DefaultImplSetParamTemplatedBase
     * must be destroyed after DefaultImplSetParamBase as DefaultImplSetParamBase
     * has indirect weak references to variables in DefaultImplSetParamTemplatedBase
     */
   private DefaultImplSetParamTemplatedBase <SettingValueType>,
   public  DefaultImplSetParamBase
{
    public:

	typedef typename SettingMutators <SettingValueType>::SetFunction SetFunction;
	typedef typename SettingMutators <SettingValueType>::GetFunction GetFunction;
	typedef typename ValueContainer <SettingValueType>::Ptr ValueContainerPtr;
	typedef DefaultImplSetParamTemplatedBase <SettingValueType> TemplateParent;

	DefaultImplSetParam (const ValueContainerPtr &defaultValueContainer,
			     CCSSettingType          type,
			     SetFunction             setFunction,
			     GetFunction             getFunction,
			     const CCSSettingInfoPtr &info,
			     const ValueContainerPtr &nonDefaultValueContainer) :
	    DefaultImplSetParamTemplatedBase <SettingValueType> (defaultValueContainer,
								 nonDefaultValueContainer),
	    DefaultImplSetParamBase (info, type),
	    mSetFunction (setFunction),
	    mGetFunction (getFunction)
	{
	}

	virtual void SetUpSetting (const SetUpSettingFunc &func)
	{
	    /* Do delayed setup here */
	    mValue = TemplateParent::mDefaultValueContainer->getContainedValue (mType, mInfo);
	    mNonDefaultValue = TemplateParent::mNonDefaultValueContainer->getRawValue (mType, mInfo);

	    InitializeDefaultsForSetting (func);
	}

	virtual void SetUpParam (const CCSSettingPtr &setting)
	{
	    ASSERT_TRUE ((*mGetFunction) (setting.get (), &mDefaultValue));

	    TakeReferenceToCreatedSetting (setting);
	}

	virtual Bool setWithInvalidType ()
	{
	    RequireSettingInterfaceRedirection redirection (this);

	    return (*mSetFunction) (mSetting.get (), mNonDefaultValue, FALSE);
	}

	virtual Bool setToNonDefaultValue ()
	{
	    return (*mSetFunction) (mSetting.get (), mNonDefaultValue, FALSE);
	}

	virtual Bool setToDefaultValue ()
	{
	    return (*mSetFunction) (mSetting.get (), mDefaultValue, FALSE);
	}

    private:

	SettingValueType   mDefaultValue;
	SettingValueType   mNonDefaultValue;

    protected:

	SetFunction        mSetFunction;
	GetFunction        mGetFunction;

};

template <typename SettingValueType>
class SetWithDisallowedValueBase
{
    protected:

	SetWithDisallowedValueBase (const CCSSettingPtr     &setting,
				    const CCSSettingInfoPtr &info) :
	    mSetting (setting),
	    mInfo (info)
	{
	}

	CCSSettingPtr     mSetting;
	CCSSettingInfoPtr mInfo;
};

template <typename SettingValueType>
class SetWithDisallowedValueTemplatedBase :
    public SetWithDisallowedValueBase
{
    protected:

	typedef typename SettingMutators <SettingValueType>::SetFunction SetFunction;

	SetWithDisallowedValueTemplatedBase (SetFunction             setFunction,
					     const CCSSettingPtr     &setting,
					     const CCSSettingInfoPtr &info) :
	    SetWithDisallowedValueBase (setting, info),
	    mSetFunction (setFunction)
	{
	}

	SetFunction       mSetFunction;
};

template <typename SettingValueType>
class SetWithDisallowedValue :
    public SetWithDisallowedValueTemplatedBase <SettingValueType>
{
    public:

	typedef typename SettingMutators <SettingValueType>::SetFunction SetFunction;

	SetWithDisallowedValue (SetFunction             setFunction,
				const CCSSettingPtr     &setting,
				const CCSSettingInfoPtr &info) :
	    SetWithDisallowedValueTemplatedBase <SettingValueType> (setFunction, setting, info)
	{
	}

	Bool operator () ()
	{
	    return FALSE;
	}
};

template <>
class SetWithDisallowedValue <int> :
    public SetWithDisallowedValueTemplatedBase <int>
{
    public:

	typedef typename SettingMutators <int>::SetFunction SetFunction;
	typedef SetWithDisallowedValueTemplatedBase <int> Parent;

	SetWithDisallowedValue (SetFunction             setFunction,
				const CCSSettingPtr     &setting,
				const CCSSettingInfoPtr &info) :
	    SetWithDisallowedValueTemplatedBase <int> (setFunction, setting, info)
	{
	}

	Bool operator () ()
	{
	    return (*Parent::mSetFunction) (Parent::mSetting.get (),
					    Parent::mInfo->forInt.min - 1,
					    FALSE);
	}
};

template <>
class SetWithDisallowedValue <float> :
    public SetWithDisallowedValueTemplatedBase <float>
{
    public:

	typedef typename SettingMutators <float>::SetFunction SetFunction;
	typedef SetWithDisallowedValueTemplatedBase <float> Parent;

	SetWithDisallowedValue (SetFunction             setFunction,
				const CCSSettingPtr     &setting,
				const CCSSettingInfoPtr &info) :
	    SetWithDisallowedValueTemplatedBase <float> (setFunction, setting, info)
	{
	}

	Bool operator () ()
	{
	    return (*Parent::mSetFunction) (Parent::mSetting.get (),
					    Parent::mInfo->forFloat.min - 1,
					    FALSE);
	}
};

template <>
class SetWithDisallowedValue <const char *> :
    public SetWithDisallowedValueTemplatedBase <const char *>
{
    public:

	typedef typename SettingMutators <const char *>::SetFunction SetFunction;
	typedef SetWithDisallowedValueTemplatedBase <const char *> Parent;

	SetWithDisallowedValue (SetFunction             setFunction,
				const CCSSettingPtr     &setting,
				const CCSSettingInfoPtr &info) :
	    SetWithDisallowedValueTemplatedBase <const char *> (setFunction, setting, info)
	{
	}

	Bool operator () ()
	{
	    return (*Parent::mSetFunction) (Parent::mSetting.get (),
					    NULL,
					    FALSE);
	}
};

template <typename SettingValueType>
class DefaultImplSetFailureParam :
    public DefaultImplSetParam <SettingValueType>
{
    public:

	typedef DefaultImplSetParam <SettingValueType> Parent;
	typedef typename DefaultImplSetParam <SettingValueType>::SetFunction SetFunction;
	typedef typename DefaultImplSetParam <SettingValueType>::GetFunction GetFunction;
	typedef typename DefaultImplSetParam <SettingValueType>::ValueContainerPtr ValueContainerPtr;

	DefaultImplSetFailureParam (const ValueContainerPtr &defaultValueContainer,
				    CCSSettingType          type,
				    SetFunction             setFunction,
				    GetFunction             getFunction,
				    const CCSSettingInfoPtr &info,
				    const ValueContainerPtr &nonDefaultValueContainer) :
	    DefaultImplSetParam <SettingValueType> (defaultValueContainer,
						    type,
						    setFunction,
						    getFunction,
						    info,
						    nonDefaultValueContainer)
	{
	}

	virtual Bool setToFailValue ()
	{
	    typedef DefaultImplSetParam <SettingValueType> Parent;
	    return SetWithDisallowedValue <SettingValueType> (Parent::mSetFunction,
							      Parent::mSetting,
							      Parent::mInfo) ();
	}
};

template <typename SettingValueType>
DefaultImplSetParamInterface::Ptr
SemanticsParamFor (const typename ValueContainer <SettingValueType>::Ptr   &defaultValue,
		   CCSSettingType                                          type,
		   typename SettingMutators<SettingValueType>::SetFunction setFunc,
		   typename SettingMutators<SettingValueType>::GetFunction getFunc,
		   const CCSSettingInfoPtr                                 &settingInfo,
		   const typename ValueContainer <SettingValueType>::Ptr   &changeTo)
{
    typedef SettingValueType T;
    return boost::make_shared <DefaultImplSetParam <T> > (defaultValue,
							  type,
							  setFunc,
							  getFunc,
							  settingInfo,
							  changeTo);
}

template <typename SettingValueType>
DefaultImplSetParamInterface::Ptr
FailureSemanticsParamFor (const typename ValueContainer <SettingValueType>::Ptr   &defaultValue,
			  CCSSettingType                                          type,
			  typename SettingMutators<SettingValueType>::SetFunction setFunc,
			  typename SettingMutators<SettingValueType>::GetFunction getFunc,
			  const CCSSettingInfoPtr                                 &settingInfo,
			  const typename ValueContainer <SettingValueType>::Ptr   &changeTo)
{
    typedef SettingValueType T;
    return boost::make_shared <DefaultImplSetFailureParam <T> > (defaultValue,
								 type,
								 setFunc,
								 getFunc,
								 settingInfo,
								 changeTo);
}

class SettingDefaultImplSet :
    public CCSSettingDefaultImplTest,
    public WithParamInterface <DefaultImplSetParamInterface::Ptr>
{
    public:

	virtual void SetUp ()
	{
	    GetParam ()->SetUpSetting (boost::bind (&CCSSettingDefaultImplTest::SetUpSetting, this, _1));
	    GetParam ()->SetUpParam (setting);
	}

	virtual void TearDown ()
	{
	    GetParam ()->TearDownSetting ();
	}

	CCSSettingType GetSettingType ()
	{
	    return GetParam ()->GetSettingType ();
	}
};

class SettingDefaulImplSetFailure :
    public SettingDefaultImplSet
{
};

}
