#include <stdexcept>
#include <string_view>

#include "regexplib.hpp"

#include "gtest/gtest.h"

struct TestSuite0 : testing::TestWithParam<std::string_view> {};

TEST_P(TestSuite0, Throws) {
  EXPECT_ANY_THROW({
    regexp::does_match("SomeInputStringNotMatterWhatKindOf", GetParam());
  });
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite0Instantiation,
    TestSuite0,
    testing::Values(
        "a*^",
        ".*$"
    )
);
/* clang-format on */

struct TestParam {
  std::string_view input;
  std::string_view pattern;
};

struct TestSuite1 : testing::TestWithParam<TestParam> {};

TEST_P(TestSuite1, Matches) {
  EXPECT_NO_THROW({
    EXPECT_TRUE(regexp::does_match(GetParam().input, GetParam().pattern));
  });
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite1Instantiation,
    TestSuite1,
    testing::Values(
        TestParam{.input = "aa",      .pattern = "aa"           },
        TestParam{.input = "aa",      .pattern = "a*"           },
        TestParam{.input = "aa",      .pattern = ".*"           },
        TestParam{.input = "ab",      .pattern = "a*b*"         },
        TestParam{.input = "abc",     .pattern = "a.c*"         },
        TestParam{.input = "aabc",    .pattern = "a*a+b+c*"     },
        TestParam{.input = "aabcdef", .pattern = "a*a+b+c*.*.+" }
    )
);
/* clang-format on */

struct TestSuite2 : testing::TestWithParam<TestParam> {};

TEST_P(TestSuite2, DoesNotMatch) {
  EXPECT_NO_THROW({
    EXPECT_FALSE(regexp::does_match(GetParam().input, GetParam().pattern));
  });
}

/* clang-format off */
INSTANTIATE_TEST_SUITE_P(
    TestSuite2Instantiation,
    TestSuite2,
    testing::Values(
        TestParam{.input = "aa",  .pattern = "a"        },
        TestParam{.input = "aa",  .pattern = "b*a"      },
        TestParam{.input = "aa",  .pattern = ".*ac"     },
        TestParam{.input = "ab",  .pattern = ".c"       },
        TestParam{.input = "abc", .pattern = "..a"      },
        TestParam{.input = "yxz", .pattern = ".+.+.+.+" },
        TestParam{.input = "yxz", .pattern = "y+x+z.+"  }
    )
);
/* clang-format on */
