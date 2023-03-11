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
        TestParam{ .input = "yxxz",       .pattern = "[yxxz]*"                    },
        TestParam{ .input = "yy",         .pattern = "y{2}"                       },
        TestParam{ .input = "yyy",        .pattern = "y{3,}"                      },
        TestParam{ .input = "yyyyy",      .pattern = "y{3,}"                      },
        TestParam{ .input = "yyy",        .pattern = "y{,5}"                      },
        TestParam{ .input = "yyyyy",      .pattern = "y{,5}"                      },
        TestParam{ .input = "yyyy",       .pattern = "y{4,5}"                     },
        TestParam{ .input = "ab",         .pattern = "[abc]{2}"                   },
        TestParam{ .input = "cab",        .pattern = "[abc]{3,}"                  },
        TestParam{ .input = "cacb",       .pattern = "[abc]{3,}"                  },
        TestParam{ .input = "bac",        .pattern = "[abc]{,5}"                  },
        TestParam{ .input = "babca",      .pattern = "[abc]{,5}"                  },
        TestParam{ .input = "abbc",       .pattern = "[abc]{4,5}"                 },
        TestParam{ .input = "abcbc",      .pattern = "[abc]{4,5}"                 },
        TestParam{ .input = "defgh",      .pattern = "[^abc]{4,5}"                },
        TestParam{ .input = "a*",         .pattern = "[a*]{,2}"                   },
        TestParam{ .input = "+b",         .pattern = "[b+]{,2}"                   },
        TestParam{ .input = "?c??c",      .pattern = "[?c]+"                      },
        TestParam{ .input = "a?",         .pattern = "a[?c]?"                     },
        TestParam{ .input = "ac",         .pattern = "a[?c]?"                     },
        TestParam{ .input = "a",          .pattern = "a[?c]?"                     },
        TestParam{ .input = "a",          .pattern = "ac?"                        },
        TestParam{ .input = "0",          .pattern = "\\d"                        },
        TestParam{ .input = "a",          .pattern = "\\D"                        },
        TestParam{ .input = " ",          .pattern = "\\D"                        },
        TestParam{ .input = "b",          .pattern = "\\w"                        },
        TestParam{ .input = "%",          .pattern = "\\W"                        },
        TestParam{ .input = " ",          .pattern = "\\W"                        },
        TestParam{ .input = " ",          .pattern = "\\s"                        },
        TestParam{ .input = "\t",         .pattern = "\\s"                        },
        TestParam{ .input = "a",          .pattern = "\\S"                        },
        TestParam{ .input = "1",          .pattern = "\\S"                        },
        TestParam{ .input = "\t",         .pattern = "\\t"                        },
        TestParam{ .input = "\r",         .pattern = "\\r"                        },
        TestParam{ .input = "\n",         .pattern = "\\n"                        },
        TestParam{ .input = "\v",         .pattern = "\\v"                        },
        TestParam{ .input = "\f",         .pattern = "\\f"                        },
        // TestParam{ .input = "\0",         .pattern = "\\0"                        },
        TestParam{ .input = "\\",         .pattern = "\\\\"                       }
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
        TestParam{ .input = "aa",      .pattern = "a"             },
        TestParam{ .input = "aa",      .pattern = "b*a"           },
        TestParam{ .input = "aa",      .pattern = ".*ac"          },
        TestParam{ .input = "ab",      .pattern = ".c"            },
        TestParam{ .input = "abc",     .pattern = "..a"           },
        TestParam{ .input = "yxz",     .pattern = ".+.+.+.+"      },
        TestParam{ .input = "yxz",     .pattern = "y+x+z.+"       },
        TestParam{ .input = "yxxz",    .pattern = ".*[ay]x+[az]." },
        TestParam{ .input = "yxxz",    .pattern = "[yxxz]+yxxz"   },
        TestParam{ .input = "y",       .pattern = "y{2}"          },
        TestParam{ .input = "yyy",     .pattern = "y{2}"          },
        TestParam{ .input = "y",       .pattern = "y{3,}"         },
        TestParam{ .input = "yy",      .pattern = "y{3,}"         },
        TestParam{ .input = "yyyyyy",  .pattern = "y{,5}"         },
        TestParam{ .input = "yyyyyyy", .pattern = "y{,5}"         },
        TestParam{ .input = "yyy",     .pattern = "y{4,5}"        },
        TestParam{ .input = "yyyyyy",  .pattern = "y{4,5}"        },
        TestParam{ .input = "abc",     .pattern = "[abc]{2}"      },
        TestParam{ .input = "ca",      .pattern = "[abc]{3,}"     },
        TestParam{ .input = "a",       .pattern = "[abc]{3,}"     },
        TestParam{ .input = "bacbac",  .pattern = "[abc]{,5}"     },
        TestParam{ .input = "babcacc", .pattern = "[abc]{,5}"     },
        TestParam{ .input = "abc",     .pattern = "[abc]{4,5}"    },
        TestParam{ .input = "abcbcc",  .pattern = "[abc]{4,5}"    },
        TestParam{ .input = "defbh",   .pattern = "[^abc]{4,5}"   },
        TestParam{ .input = "ab",      .pattern = "a[?c]?"        },
        TestParam{ .input = "ab",      .pattern = "ac?"           },
        TestParam{ .input = "acc",     .pattern = "ac?"           }
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
        "[c[",
        "[c[d]]",
        "c{",
        "c}",
        "c{{",
        "c{4,{",
        "c{4,6{",
        "c{4,6}}",
        "[cab]{",
        "[cab]}",
        "[cab]{{",
        "[cab]{4,{",
        "[cab]{4,6{",
        "[cab]{4,6}}",
        "[c^ab]{4,6}",
        "[cab^]{4,6}",
        "[^^cab]{4,6}",
        "[^]{4,6}",
        "f{6,4}",
        "[abc]{6,4}"
    )
);
/* clang-format on */
