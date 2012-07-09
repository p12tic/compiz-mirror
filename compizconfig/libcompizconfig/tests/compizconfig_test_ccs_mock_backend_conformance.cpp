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

void
MockCCSBackendConceptTestEnvironment::TearDown (CCSBackend *backend)
{
    ccsFreeMockBackend (backend);
}

static MockCCSBackendConceptTestEnvironment mockBackendEnv;
static CCSBackendConceptTestEnvironmentInterface *backendEnv = &mockBackendEnv;

INSTANTIATE_TEST_CASE_P (MockCCSBackendConcept, CCSBackendConformanceTest, ::testing::Values (backendEnv));


