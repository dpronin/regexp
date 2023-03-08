#include <stdexcept>
#include <string_view>

#include "regexplib.hpp"

#include "gtest/gtest.h"

struct TestParam {
    std::string_view input;
    std::string_view pattern;
};

struct TestSuite1 : testing::TestWithParam<TestParam> {
};

TEST_P(TestSuite1, Matches)
{
    EXPECT_NO_THROW({ EXPECT_TRUE(regexp::does_match(GetParam().input, GetParam().pattern)); });
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite1Instantiation,
    TestSuite1,
    testing::Values(
        TestParam{ .input = "aa",         .pattern = "aa"                         },
        TestParam{ .input = "aa",         .pattern = "a*"                         },
        TestParam{ .input = "aa",         .pattern = ".*"                         },
        TestParam{ .input = "ab",         .pattern = "a*b*"                       },
        TestParam{ .input = "abc",        .pattern = "a.c*"                       },
        TestParam{ .input = "aabc",       .pattern = "a*a+b+c*"                   },
        TestParam{ .input = "aabcdef",    .pattern = "a*a+b+c*.*.+"               },
        TestParam{ .input = "aabc012ef",  .pattern = "a*a+b+c*.*.+"               },
        TestParam{ .input = "aabc012ef",  .pattern = "a*a+b+c*012.+"              },
        TestParam{ .input = "1122334455", .pattern = "1+22*33.+.*"                },
        TestParam{ .input = "1122334455", .pattern = "[123][123].*[12].+[43].+"   },
        TestParam{ .input = "1122334455", .pattern = "[123]*[123]+.*[12].+[43].+" },
        TestParam{ .input = "yxxz",       .pattern = "[yxxz]*"                    }
    )
);
/* clang-format on */

struct TestSuite2 : testing::TestWithParam<TestParam> {
};

TEST_P(TestSuite2, DoesNotMatch)
{
    EXPECT_NO_THROW({ EXPECT_FALSE(regexp::does_match(GetParam().input, GetParam().pattern)); });
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite2Instantiation,
    TestSuite2,
    testing::Values(
        TestParam{ .input = "aa",   .pattern = "a"             },
        TestParam{ .input = "aa",   .pattern = "b*a"           },
        TestParam{ .input = "aa",   .pattern = ".*ac"          },
        TestParam{ .input = "ab",   .pattern = ".c"            },
        TestParam{ .input = "abc",  .pattern = "..a"           },
        TestParam{ .input = "yxz",  .pattern = ".+.+.+.+"      },
        TestParam{ .input = "yxz",  .pattern = "y+x+z.+"       },
        TestParam{ .input = "yxxz", .pattern = ".*[ay]x+[az]." },
        TestParam{ .input = "yxxz", .pattern = "[yxxz]+yxxz"   }
    )
);
/* clang-format on */

struct TestSuite3 : testing::TestWithParam<std::string_view> {
};

TEST_P(TestSuite3, ThrowsInvalidArgument)
{
    EXPECT_THROW(
        { EXPECT_FALSE(regexp::does_match("InputStringNoMatternWhatKindOf", GetParam())); },
        std::invalid_argument);
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite3Instantiation,
    TestSuite3,
    testing::Values(
        "*",
        "+",
        "[",
        "]",
        "[]",
        "b**",
        "a++",
        "[abc]**",
        "[abc]++",
        "[abc]*+",
        "[abc]+*",
        "[a*]",
        "[b+]",
        "[c[",
        "[c[d]]"
    )
);
/* clang-format on */
