#include <core/wrapsystem.h>

#include <gtest/gtest.h>

namespace {

class TestImplementation;

class TestInterface : public WrapableInterface<TestImplementation, TestInterface> {
public:
    TestInterface();
    ~TestInterface();

    virtual void testMethod() /* const */ = 0;

    static int testMethodCalls;

private:
    TestInterface(TestInterface const&);
    TestInterface& operator=(TestInterface const&);
};

class TestImplementation : public WrapableHandler<TestInterface, 1> {
public:
    static int testMethodCalls;

    // 1. needs for magic numbers
    // 2. why can't we just pass &TestInterface::testMethod (and deduce return etc.
    // 3. relies on __VA_ARGS__ when an extra set of parentheses would be enough
    WRAPABLE_HND (0, TestInterface, void, testMethod)
};

class TestWrapper : public TestInterface {
    TestImplementation& impl;
public:
    static int testMethodCalls;

    TestWrapper(TestImplementation& impl)
        : impl(impl)
    { setHandler(&impl, true); }  // The need to remember this is a PITA

    ~TestWrapper()
    { setHandler(&impl, false); }  // The need to remember this is a PITA

    virtual void testMethod();

    void disableTestMethod() {
        impl.testMethodSetEnabled (this, false);
    }
};
} // (abstract) namespace


int TestWrapper::testMethodCalls = 0;
int TestInterface::testMethodCalls = 0;
int TestImplementation::testMethodCalls = 0;

// A pain these need definition after TestImplementation definition
TestInterface::TestInterface() {}
TestInterface::~TestInterface() {}

void TestInterface::testMethod() /* const */ {
    WRAPABLE_DEF (testMethod);
    testMethodCalls++;
}

void TestImplementation::testMethod() /* const */ {
    WRAPABLE_HND_FUNC(0, testMethod)
    testMethodCalls++;
}

void TestWrapper::testMethod() /* const */ {
    impl.testMethod();
    testMethodCalls++;
}


TEST(WrapSystem, a_wrapper_gets_functions_called)
{
    TestWrapper::testMethodCalls = 0;
    TestInterface::testMethodCalls = 0;
    TestImplementation::testMethodCalls = 0;

    TestImplementation imp;
    {
        TestWrapper wrap(imp);

        imp.testMethod();

        ASSERT_EQ(1, TestImplementation::testMethodCalls);
        ASSERT_EQ(0, TestInterface::testMethodCalls);
        ASSERT_EQ(1, TestWrapper::testMethodCalls);
    }

    imp.testMethod();

    ASSERT_EQ(2, TestImplementation::testMethodCalls);
    ASSERT_EQ(0, TestInterface::testMethodCalls);
    ASSERT_EQ(1, TestWrapper::testMethodCalls);
}

TEST(WrapSystem, a_wrapper_doesnt_get_disabled_functions_called)
{
    TestWrapper::testMethodCalls = 0;

    TestImplementation imp;
    {
        TestWrapper wrap(imp);

        wrap.disableTestMethod();

        imp.testMethod();

        ASSERT_EQ(0, TestWrapper::testMethodCalls);
    }
}
