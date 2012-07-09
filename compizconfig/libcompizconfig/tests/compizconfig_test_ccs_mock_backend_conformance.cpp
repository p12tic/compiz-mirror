#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs.h>

#include <compizconfig_ccs_backend_mock.h>
#include <compizconfig_backend_concept_test.h>

#include "compizconfig_ccs_backend_mock.h"

using ::testing::_;
using ::testing::Return;

class MockCCSBackendConceptTestEnvironment :
    public CCSBackendConceptTestEnvironmentInterface
{
    public:

	CCSBackend * SetUp ();
	void TearDown (CCSBackend *backend);
};

CCSBackend *
MockCCSBackendConceptTestEnvironment::SetUp ()
{
    CCSBackend *backend = ccsMockBackendNew ();
    return backend;
}

static MockCCSBackendConceptTestEnvironment mockBackendEnv;

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest, ::testing::Values (&mockBackendEnv));


