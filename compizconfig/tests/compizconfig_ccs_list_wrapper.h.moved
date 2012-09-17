/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd.
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
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#ifndef _COMPIZCONFIG_CCS_LIST_WRAPPER_H
#define _COMPIZCONFIG_CCS_LIST_WRAPPER_H

#include <ccs-defs.h>
#include <ccs-setting-types.h>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

typedef struct _CCSSettingValueList * CCSSettingValueList;
typedef struct _CCSSetting CCSSetting;
typedef union _CCSSettingInfo CCSSettingInfo;
typedef struct _CCSSettingValue CCSSettingValue;

namespace compiz
{
    namespace config
    {
	template <typename ListType, typename DataType>
	class CCSListWrapper :
	    boost::noncopyable
	{
	    public:

		virtual ~CCSListWrapper () {}

		virtual CCSListWrapper<ListType, DataType> & append (const DataType &) = 0;
		virtual CCSListWrapper<ListType, DataType> & remove (const DataType &) = 0;

		virtual operator const ListType & () const = 0;
		virtual operator ListType & () = 0;
	};

	namespace impl
	{
	    namespace cc = compiz::config;
	    namespace cci = compiz::config::impl;

	    typedef enum _ListStorageType
	    {
		Shallow = 0,
		Deep = 1
	    } ListStorageType;

	    template <typename ListType, typename DataType>
	    class CCSListWrapper :
		public cc::CCSListWrapper <ListType, DataType>
	    {
		public:

		    typedef ListType (*ListTypeFreeFunc) (ListType, Bool);
		    typedef ListType (*ListTypeAppendFunc) (ListType, DataType);
		    typedef ListType (*ListTypeRemoveFunc) (ListType, DataType, Bool);

		    CCSListWrapper (const ListType &list,
				    ListTypeFreeFunc freeFunc,
				    ListTypeAppendFunc appendFunc,
				    ListTypeRemoveFunc removeFunc,
				    ListStorageType  storageType) :
			mList (list),
			mFree (freeFunc),
			mAppend (appendFunc),
			mRemove (removeFunc),
			mStorageType (storageType)
		    {
		    };

		    cc::CCSListWrapper<ListType, DataType> & append (DataType const &data)
		    {
			mList = (*mAppend) (mList, data);
			return *this;
		    }

		    cc::CCSListWrapper<ListType, DataType> & remove (DataType const &data)
		    {
			Bool freeObj = (mStorageType == Deep);
			mList = (*mRemove) (mList, data, freeObj);
			return *this;
		    }

		    operator const ListType & () const
		    {
			return mList;
		    }

		    operator ListType & ()
		    {
			return mList;
		    }

		    ~CCSListWrapper ()
		    {
			Bool freeObj = (mStorageType == Deep);

			(*mFree) (mList, freeObj);
		    }

		private:

		    ListType           mList;
		    ListTypeFreeFunc   mFree;
		    ListTypeAppendFunc mAppend;
		    ListTypeRemoveFunc mRemove;
		    ListStorageType    mStorageType;
	    };

	    class PrivateCCSSettingValueListWrapper;

	    class CCSSettingValueListWrapper :
		public compiz::config::CCSListWrapper <CCSSettingValueList, CCSSettingValue *>
	    {
		public:

		    typedef boost::shared_ptr <CCSSettingValueListWrapper> Ptr;
		    typedef compiz::config::CCSListWrapper <CCSSettingValueList, CCSSettingValue *> InternalWrapper;
		    typedef compiz::config::impl::CCSListWrapper <CCSSettingValueList, CCSSettingValue *> InternalWrapperImpl;

		    CCSSettingValueListWrapper (CCSSettingValueList                      list,
						ListStorageType                          storageType,
						CCSSettingType                           type,
						const boost::shared_ptr <CCSSettingInfo> &listInfo,
						const boost::shared_ptr <CCSSetting>     &settingReference);

		    CCSSettingType type ();

		    InternalWrapper & append (CCSSettingValue * const &value);
		    InternalWrapper & remove (CCSSettingValue * const &value);
		    operator const CCSSettingValueList & () const;
		    operator CCSSettingValueList & ();
		    const boost::shared_ptr <CCSSetting> & setting ();

		private:

		    boost::shared_ptr <PrivateCCSSettingValueListWrapper> priv;
	    };
	}
    }
}



#endif
