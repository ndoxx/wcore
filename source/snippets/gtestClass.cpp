#include <gtest/gtest.h>
#include "SomeClass.h"

class SomeClassTest : testing::Test
{
    SomeClass* some_class;
    SomeClassTest()
    {
        some_class = new SomeClass;
    }
    virtual ~SomeClassTest()
    {
        delete some_class;
    }
};

struct Parameters;
class SomeClassOtherTest : SomeClassTest,
                           testing::WithParamInterface<SomeClassState>
{
    SomeClass* some_class;
    SomeClassOtherTest()
    {
        some_class->member1 = GetParam().param1;
    }
    ~SomeClassOtherTest()
    {
        delete some_class;
    }
};

// Parameter struct for parameterized tests
struct SomeClassState
{
    int param1;
    int param2;
    int some_value;
    bool success;
    // Must be displayable
    friend std::ostream& operator<<(std::ostream& stream, const SomeClassState& state)
    {
        //...
        return stream
    }
}

// Non parameterized test
TEST_F(SomeClassTest, NameOfTestCase)
{
    //code
    some_class->method0();
    //code
    //EXPECT_EQ(...);
}

// Parameterized test
TEST_P(SomeClassOtherTest, NameOfTestCase2)
{
    auto as = GetParam();
    auto success = some_class->method1(as.param1);
    EXPECT_EQ(as.some_value, some_class->member1);
    EXPECT_EQ(as.success, success);
}

INSTANTIATE_TEST_CASE_P(Default, SomeClassOtherTest,
                        testing::Values(
                        SomeClassState{10/*param1*/, 20/*param2*/, 50/*expect*/, true/*success*/},
                        SomeClassState{12/*param1*/, 40/*param2*/, 15/*expect*/, true/*success*/},
                        // ...
                        SomeClassState{1/*param1*/, 0/*param2*/, 28/*expect*/, false/*success*/}
                        ));

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
