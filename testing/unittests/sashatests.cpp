/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2018 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

/*
 * Sasha Quick Regression Suite: Tests Sasha functionality on a programmatic way.
 *
 */

#ifdef _USE_CPPUNIT

#include "unittests.hpp"

//#define COMPAT

//#define MY_DEBUG

// ======================================================================= Support Functions / Classes

// ================================================================================== UNIT TESTS

static void setupABC(const IContextLogger &logctx, unsigned &testNum, StringAttr &testString)
{
#ifdef MY_DEBUG
    //logctx.CTXLOG("Enter setupABC."); //Not sure why logctx.CTXLOG does not work
    printf("Enter setupABC.\n");
#endif

    testNum = 2;
    testString.set("abc");

    // Make sure it got created
    ASSERT(testString.length() == 3 && "Can't set testString");

#ifdef MY_DEBUG
    printf("Leace setupABC: testNum=%d; testString=%s.\n", testNum, testString.get());
#endif
}

class CSashaWUiterateTests : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CSashaWUiterateTests);
        CPPUNIT_TEST(testInit);
        CPPUNIT_TEST(testWUiterate);
    CPPUNIT_TEST_SUITE_END();

    const IContextLogger &logctx;
    unsigned testNum = 1;
    StringAttr testString;

    const char *getTestString() { return testString.get(); };
public:
    CSashaWUiterateTests() : logctx(queryDummyContextLogger())
    {
    }
    ~CSashaWUiterateTests()
    {
        ;//daliClientEnd();
    }
    void testInit()
    {
        ;//daliClientInit();
    }

    void testWUiterate()
    {
#ifdef MY_DEBUG
        //logctx.CTXLOG("Enter testWUiterate.\n"); //Not sure why logctx.CTXLOG does not work
        printf("\nEnter testWUiterate.\n");
#endif

        setupABC(logctx, testNum, testString);

#ifdef MY_DEBUG
        printf("Start testing\n");
#endif

        const char *str = getTestString();
        ASSERT(nullptr != str && "The testString should not be empty.");
        ASSERT(0 != streq(str, "abc") && "The testString should be 'abc'.");

        ASSERT(0 == testNum && "testNum should be 0.");

#ifdef MY_DEBUG
        printf("End testing\n");
        printf("Leave testWUiterate.\n");
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSashaWUiterateTests );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CSashaWUiterateTests, "CSashaWUiterateTests" );


#endif // _USE_CPPUNIT
