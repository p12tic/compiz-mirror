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
#include <tr1/tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/shared_ptr.hpp>

#include <gtest_shared_autodestroy.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs_gnome_integrated_setting.h>
#include <ccs_gnome_integration.h>
#include <compizconfig_ccs_context_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_backend_mock.h>
#include <compizconfig_ccs_integrated_setting_factory_mock.h>
#include <compizconfig_ccs_integrated_setting_storage_mock.h>
#include <compizconfig_ccs_integrated_setting_mock.h>
#include "compizconfig_ccs_mock_gnome_integrated_setting_composition.h"

using ::testing::Return;
using ::testing::_;

namespace
{
    typedef std::tr1::tuple <const CCSIntegratedSettingGMock &,
			     CCSIntegratedSetting            *> IntegratedSettingWithMock;

    IntegratedSettingWithMock
    createIntegratedSettingCompositionFromMock (const std::string            &plugin,
						const std::string            &setting,
						CCSSettingType               type,
						SpecialOptionType            gnomeType,
						const std::string            &gnomeName,
						CCSObjectAllocationInterface *allocator)
    {
	CCSIntegratedSettingInfo *integratedSettingInfo =
		ccsSharedIntegratedSettingInfoNew (plugin.c_str (),
						   setting.c_str (),
						   type,
						   allocator);
	CCSGNOMEIntegratedSettingInfo *gnomeIntegratedSettingInfo =
		ccsGNOMEIntegratedSettingInfoNew (integratedSettingInfo,
						  gnomeType,
						  gnomeName.c_str (),
						  allocator);
	CCSIntegratedSetting          *integratedSetting =
		ccsMockIntegratedSettingNew (allocator);

	CCSIntegratedSettingGMock *mockPtr = GET_PRIVATE (CCSIntegratedSettingGMock, integratedSetting)
	const CCSIntegratedSettingGMock &mock (*mockPtr);

	CCSIntegratedSetting          *composition =
		ccsMockCompositionIntegratedSettingNew (integratedSetting,
							gnomeIntegratedSettingInfo,
							integratedSettingInfo,
							allocator);

	return IntegratedSettingWithMock (mock, composition);
    }

    const std::string MOCK_PLUGIN ("mock");
    const std::string MOCK_SETTING ("mock");
    const std::string MOCK_GNOME_NAME ("mock");
    const CCSSettingType MOCK_SETTING_TYPE = TypeInt;
    const SpecialOptionType MOCK_GNOME_SETTING_TYPE = OptionInt;
}

TEST (CCSGNOMEIntegrationTest, TestConstructComposition)
{
    IntegratedSettingWithMock settingWithMock (
	createIntegratedSettingCompositionFromMock (MOCK_PLUGIN,
						    MOCK_SETTING,
						    MOCK_SETTING_TYPE,
						    MOCK_GNOME_SETTING_TYPE,
						    MOCK_GNOME_NAME,
						    &ccsDefaultObjectAllocator));
}

TEST (CCSGNOMEIntegrationTest, TestConstructGNOMEIntegration)
{
    CCSObjectAllocationInterface                     *ai = &ccsDefaultObjectAllocator;
    boost::shared_ptr <CCSContext>                   context (AutoDestroy (ccsMockContextNew (),
									   ccsFreeContext));
    boost::shared_ptr <CCSBackend>                   backend (AutoDestroy (ccsMockBackendNew (),
									   ccsBackendUnref));
    boost::shared_ptr <CCSIntegratedSettingsStorage> storage (AutoDestroy (ccsMockIntegratedSettingsStorageNew (ai),
									   ccsIntegratedSettingsStorageUnref));
    boost::shared_ptr <CCSIntegratedSettingFactory>  factory (AutoDestroy (ccsMockIntegratedSettingFactoryNew (ai),
									   ccsIntegratedSettingFactoryUnref));
    boost::shared_ptr <CCSIntegration>               integration (AutoDestroy (ccsGNOMEIntegrationBackendNew (backend.get (),
													      context.get (),
													      factory.get (),
													      storage.get (),
													      ai),
									       ccsIntegrationUnref));
}

