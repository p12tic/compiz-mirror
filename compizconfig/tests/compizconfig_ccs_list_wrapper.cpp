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
#include <ccs.h>
#include <compizconfig_ccs_list_wrapper.h>

namespace cci = compiz::config::impl;
namespace cc  = compiz::config;

namespace compiz
{
    namespace config
    {
	namespace impl
	{
	    class PrivateCCSSettingValueListWrapper
	    {
		public:

		    PrivateCCSSettingValueListWrapper (CCSSettingValueList                      list,
						       cci::ListStorageType                     storageType,
						       CCSSettingType                           type,
						       const boost::shared_ptr <CCSSettingInfo> &listInfo,
						       const boost::shared_ptr <CCSSetting>     &settingReference) :
			mType (type),
			mListInfo (listInfo),
			mSettingReference (settingReference),
			mListWrapper (list,
				      ccsSettingValueListFree,
				      ccsSettingValueListAppend,
				      ccsSettingValueListRemove,
				      storageType)
		    {
		    }

		    CCSSettingType                                  mType;
		    boost::shared_ptr <CCSSettingInfo>              mListInfo;
		    boost::shared_ptr <CCSSetting>                  mSettingReference;
		    CCSSettingValueListWrapper::InternalWrapperImpl mListWrapper;
	    };
	}
    }
}

cci::CCSSettingValueListWrapper::CCSSettingValueListWrapper (CCSSettingValueList                      list,
							     cci::ListStorageType                     storageType,
							     CCSSettingType                           type,
							     const boost::shared_ptr <CCSSettingInfo> &listInfo,
							     const boost::shared_ptr <CCSSetting>     &settingReference) :
    priv (new cci::PrivateCCSSettingValueListWrapper (list,
						      storageType,
						      type,
						      listInfo,
						      settingReference))
{
}

CCSSettingType
cci::CCSSettingValueListWrapper::type ()
{
    return priv->mType;
}

cci::CCSSettingValueListWrapper::InternalWrapper &
cci::CCSSettingValueListWrapper::append (CCSSettingValue * const &value)
{
    return priv->mListWrapper.append (value);
}

cci::CCSSettingValueListWrapper::InternalWrapper &
cci::CCSSettingValueListWrapper::remove (CCSSettingValue * const &value)
{
    return priv->mListWrapper.remove (value);
}

cci::CCSSettingValueListWrapper::operator const CCSSettingValueList & () const
{
    return priv->mListWrapper;
}

cci::CCSSettingValueListWrapper::operator CCSSettingValueList & ()
{
    return priv->mListWrapper;
}

const boost::shared_ptr <CCSSetting> &
cci::CCSSettingValueListWrapper::setting ()
{
    return priv->mSettingReference;
}
