#ifndef _COMPIZCONFIG_CCS_LIST_WRAPPER_H
#define _COMPIZCONFIG_CCS_LIST_WRAPPER_H

#include <ccs-defs.h>
#include <ccs-setting-types.h>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

typedef struct _CCSSettingValueList * CCSSettingValueList;
typedef struct _CCSSetting CCSSetting;
typedef union _CCSSettingInfo CCSSettingInfo;

class CCSListWrapper :
    boost::noncopyable
{
    public:

	typedef boost::shared_ptr <CCSListWrapper> Ptr;

	CCSListWrapper (CCSSettingValueList list,
			bool freeItems,
			CCSSettingType type,
			const boost::shared_ptr <CCSSettingInfo> &listInfo,
			const boost::shared_ptr <CCSSetting> &settingReference) :
	    mList (list),
	    mFreeItems (freeItems),
	    mType (type),
	    mListInfo (listInfo),
	    mSettingReference (settingReference)
	{
	}

	CCSSettingType type () { return mType; }

	operator CCSSettingValueList ()
	{
	    return mList;
	}

	operator CCSSettingValueList () const
	{
	    return mList;
	}

	~CCSListWrapper ()
	{
	    ccsSettingValueListFree (mList, mFreeItems ? TRUE : FALSE);
	}

	const boost::shared_ptr <CCSSetting> &
	setting ()
	{
	    return mSettingReference;
	}

    private:

	CCSSettingValueList mList;
	bool		    mFreeItems;
	CCSSettingType      mType;
	boost::shared_ptr <CCSSettingInfo> mListInfo;
	boost::shared_ptr <CCSSetting> mSettingReference;
};

#endif
