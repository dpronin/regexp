#include <cstdint>

#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace
{

template <typename Range>
struct matcher_range {
    Range cs;
};

struct min_max_rule {
    uint32_t m, n;
};

struct matcher_range_strict : matcher_range<std::string_view> {
};

struct matcher_spec_char : min_max_rule {
    char c;
};

struct matcher_any_char : min_max_rule {
};

struct matcher_range_one_of_char_positive : matcher_range<std::vector<char>>, min_max_rule {
};

struct matcher_range_one_of_char_negative : matcher_range<std::vector<char>>, min_max_rule {
};

/* clang-format off */
using matcher_t = std::variant<
  matcher_range_strict,
  matcher_spec_char,
  matcher_any_char,
  matcher_range_one_of_char_positive,
  matcher_range_one_of_char_negative
>;
/* clang-format on */

using matcher_table_t = std::vector<matcher_t>;

enum converter_mode {
    kDefault,
    kOneOf,
    kOccurrencesSpecMin,
    kOccurrencesSpecMax,
    kQty,
};

template <typename InputIt>
struct converter_ctx {
    converter_ctx(InputIt b, InputIt e)
        : f(b)
        , i(b)
        , l(e)
    {
    }

    InputIt f;
    InputIt i;
    InputIt l;

    uint32_t m, n;

    converter_mode mode = converter_mode::kDefault;
};

template <typename... Args>
constexpr bool dependent_false_v = false;

constexpr auto does_allow_zero_occurrences = [](matcher_t const& matcher) {
    return std::visit(
        [](auto const& m) {
            if constexpr (std::is_base_of_v<min_max_rule, std::decay_t<decltype(m)>>)
                return 0 == m.m;
            else
                return false;
        },
        matcher);
};

constexpr auto is_zero_more_spec_char_matcher_with = [](matcher_t const& matcher, char c) {
    return std::holds_alternative<matcher_spec_char>(matcher) &&
           does_allow_zero_occurrences(matcher) && std::get<matcher_spec_char>(matcher).c == c;
};

constexpr auto is_zero_more_any_char_matcher = [](matcher_t const& matcher) {
    return std::holds_alternative<matcher_any_char>(matcher) &&
           does_allow_zero_occurrences(matcher);
};

using converter_handler_t = std::function<bool(
    std::string_view p,
    converter_ctx<std::string_view::const_iterator>& ctx,
    matcher_table_t& table)>;

constexpr auto converter_default = [](std::string_view p,
                                      converter_ctx<std::string_view::const_iterator>& ctx,
                                      matcher_table_t& table) {
    if (ctx.i >= ctx.l) {
        if (ctx.f < ctx.i)
            table.push_back(matcher_range_strict{{{ctx.f, ctx.i}}});
        return false;
    }

    switch (auto const c = *ctx.i; c) {
        case '[':
            if (ctx.f < ctx.i)
                table.push_back(matcher_range_strict{{{ctx.f, ctx.i}}});
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kOneOf;
            break;
        case '*':
        case '+':
        case '?':
        case '{':
            if (ctx.f == ctx.i)
                throw std::invalid_argument(
                    std::string{"unexpected '"} + c + "' without previous symbol or expression");

            if (ctx.f < ctx.i - 1)
                table.push_back(matcher_range_strict{{{ctx.f, ctx.i - 1}}});

            switch (c) {
                case '*':
                case '+':
                case '?':
                    switch (auto const pc = ctx.i[-1]; pc) {
                        case '.':
                            while (!table.empty() && does_allow_zero_occurrences(table.back()))
                                table.pop_back();
                            table.push_back(matcher_any_char{
                                {0, '?' == c ? 1 : std::numeric_limits<uint32_t>::max()}});
                            if ('+' == c)
                                table.push_back(matcher_any_char{{1, 1}});
                            break;
                        default:
                            if (table.empty() ||
                                !is_zero_more_any_char_matcher(table.back()) &&
                                    !is_zero_more_spec_char_matcher_with(table.back(), pc))
                                table.push_back(matcher_spec_char{
                                    {0, '?' == c ? 1 : std::numeric_limits<uint32_t>::max()},
                                    pc});
                            if ('+' == c)
                                table.push_back(matcher_spec_char{{1, 1}, pc});
                            break;
                    }
                    break;
                default:
                    switch (auto const pc = ctx.i[-1]; pc) {
                        case '.':
                            table.push_back(matcher_any_char{{1, 1}});
                            break;
                        default:
                            table.push_back(matcher_spec_char{{1, 1}, pc});
                            break;
                    }
                    ctx.m    = 0;
                    ctx.n    = 0;
                    ctx.mode = converter_mode::kOccurrencesSpecMin;
                    break;
            }

            ctx.f = ctx.i + 1;
            break;
        default:
            break;
        case ']':
            throw std::invalid_argument("unexpected ']' in not opened oneof [] expression");
        case '}':
            throw std::invalid_argument(
                "unexpected '}' in not opened occurrences specifier {} expression");
    }
    ++ctx.i;

    return true;
};

constexpr auto converter_oneof = [](std::string_view p,
                                    converter_ctx<std::string_view::const_iterator>& ctx,
                                    matcher_table_t& table) {
    if (ctx.i >= ctx.l)
        throw std::invalid_argument("not terminated oneof [] expression");

    switch (auto const c = *ctx.i; c) {
        case ']': {
            bool const negate = '^' == *ctx.f;
            if (negate)
                ++ctx.f;

            if (ctx.f == ctx.i)
                throw std::invalid_argument("empty oneof [] expression is impossible");

            std::vector<char> cs{ctx.f, ctx.i};
            std::sort(cs.begin(), cs.end());
            cs.erase(std::unique(cs.begin(), cs.end()), cs.end());

            ctx.mode = converter_mode::kDefault;

            uint32_t m = 1, n = 1;
            if (ctx.i + 1 < ctx.l) {
                switch (auto const nc = ctx.i[1]; nc) {
                    case '{':
                        ctx.m = 0;
                        ctx.n = 0;
                        ++ctx.i;
                        ctx.mode = converter_mode::kOccurrencesSpecMin;
                        break;
                    case '*':
                        m = 0, n = std::numeric_limits<decltype(n)>::max();
                        ++ctx.i;
                        break;
                    case '+':
                        m = 1, n = std::numeric_limits<decltype(n)>::max();
                        ++ctx.i;
                        break;
                    case '?':
                        m = 0, n = 1;
                        ++ctx.i;
                        break;
                    default:
                        break;
                }
            }

            table.push_back(
                negate ? matcher_t{matcher_range_one_of_char_negative({{std::move(cs)}, m, n})}
                       : matcher_t{matcher_range_one_of_char_positive({{std::move(cs)}, m, n})});
            ctx.f = ctx.i + 1;
        } break;
        case '^':
            if (ctx.f == ctx.i)
                break;
            [[fallthrough]];
        case '[':
            throw std::invalid_argument(
                std::string{"unexpected '"} + c + "' inside opened oneof [] expression");
        default:
            break;
    }
    ++ctx.i;

    return true;
};

constexpr auto converter_occur_spec_min = [](std::string_view p,
                                             converter_ctx<std::string_view::const_iterator>& ctx,
                                             matcher_table_t& table) {
    if (ctx.i >= ctx.l)
        throw std::invalid_argument("not terminated occurrences specifier {} expression");

    switch (auto const c = *ctx.i; c) {
        case '0' ... '9':
            ctx.m = ctx.m * 10 + c - '0';
            break;
        case '}':
            if (ctx.f == ctx.i)
                throw std::invalid_argument(
                    "empty occurrences specifier {} expression is impossible");
            ctx.n = ctx.m;
            std::visit(
                [&](auto& m) {
                    if constexpr (std::is_base_of_v<min_max_rule, std::decay_t<decltype(m)>>) {
                        m.m = ctx.m;
                        m.n = ctx.n;
                    } else {
                        throw std::invalid_argument("invalid type of matcher on the top of table");
                    }
                },
                table.back());
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kDefault;
            break;
        case ',':
            ctx.n    = 0;
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kOccurrencesSpecMax;
            break;
        default:
            throw std::invalid_argument(
                std::string{"unexpected '"} + c +
                "' inside opened occurrences specifier {} expression");
            break;
    }
    ++ctx.i;

    return true;
};

constexpr auto converter_occur_spec_max = [](std::string_view p,
                                             converter_ctx<std::string_view::const_iterator>& ctx,
                                             matcher_table_t& table) {
    if (ctx.i >= ctx.l)
        throw std::invalid_argument("not terminated occurrences specifier {} expression");

    switch (auto const c = *ctx.i; c) {
        case '0' ... '9':
            ctx.n = ctx.n * 10 + c - '0';
            break;
        case '}':
            ctx.n = ctx.n ?: std::numeric_limits<decltype(ctx.n)>::max();
            std::visit(
                [&](auto& m) {
                    if constexpr (std::is_base_of_v<min_max_rule, std::decay_t<decltype(m)>>) {
                        m.m = ctx.m;
                        m.n = ctx.n;
                    } else {
                        throw std::invalid_argument("invalid type of matcher on the top of table");
                    }
                },
                table.back());
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kDefault;
            break;
        default:
            throw std::invalid_argument(
                std::string{"unexpected '"} + c +
                "' inside opened occurrences specifier {} expression");
            break;
    }
    ++ctx.i;

    return true;
};

std::array<converter_handler_t, converter_mode::kQty> const handlers = {
    converter_default,
    converter_oneof,
    converter_occur_spec_min,
    converter_occur_spec_max,
};

matcher_table_t convert_to_table(std::string_view p)
{
    matcher_table_t table;
    for (converter_ctx ctx{p.cbegin(), p.cend()}; handlers[ctx.mode](p, ctx, table);)
        ;
    return table;
}

bool does_match(
    std::string_view::const_iterator s_first,
    std::string_view::const_iterator s_last,
    matcher_table_t::const_iterator tb_first,
    matcher_table_t::const_iterator tb_last);

auto constexpr does_match_with_matcher_range_strict =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        auto const cmp = [](auto sc, auto pc) { return '.' == pc || sc == pc; };
        return std::equal(
                   s_first,
                   s_first +
                       std::min(static_cast<size_t>(std::distance(s_last, s_first)), m.cs.size()),
                   m.cs.cbegin(),
                   m.cs.cend(),
                   cmp) &&
               does_match(s_first + m.cs.size(), s_last, tb_first + 1, tb_last);
    };

auto constexpr range_matcher_gen(auto predicate)
{
    return [=](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        uint32_t k = 0;
        for (; k < m.m && s_first < s_last && predicate(m, s_first, s_last); ++k, ++s_first)
            ;

        if (k == m.m) {
            if (does_match(s_first, s_last, tb_first + 1, tb_last))
                return true;

            for (; k < m.n && s_first < s_last && predicate(m, s_first, s_last) &&
                   !does_match(s_first + 1, s_last, tb_first + 1, tb_last);
                 ++k, ++s_first)
                ;

            return k < m.n && s_first < s_last && predicate(m, s_first, s_last);
        }

        return false;
    };
};

auto constexpr does_match_with_matcher_spec_char =
    range_matcher_gen([](auto const& m, auto s_first, auto s_last) { return m.c == *s_first; });

auto constexpr does_match_with_matcher_any_char =
    range_matcher_gen([](auto const& m, auto s_first, auto s_last) { return true; });

auto constexpr does_match_with_matcher_range_one_of_char_positive =
    range_matcher_gen([](auto const& m, auto s_first, auto s_last) {
        return std::any_of(m.cs.cbegin(), m.cs.cend(), [c = *s_first](auto pc) { return c == pc; });
    });

auto constexpr does_match_with_matcher_range_one_of_char_negative =
    range_matcher_gen([](auto const& m, auto s_first, auto s_last) {
        return std::none_of(m.cs.cbegin(), m.cs.cend(), [c = *s_first](auto pc) {
            return c == pc;
        });
    });

bool does_match(
    std::string_view::const_iterator s_first,
    std::string_view::const_iterator s_last,
    matcher_table_t::const_iterator tb_first,
    matcher_table_t::const_iterator tb_last)
{
    return (s_first < s_last && tb_first < tb_last &&
            std::visit(
                [&](auto const& m) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(m)>, matcher_range_strict>) {
                        return does_match_with_matcher_range_strict(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_spec_char>) {
                        return does_match_with_matcher_spec_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_any_char>) {
                        return does_match_with_matcher_any_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_range_one_of_char_positive>) {
                        return does_match_with_matcher_range_one_of_char_positive(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_range_one_of_char_negative>) {
                        return does_match_with_matcher_range_one_of_char_negative(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else {
                        static_assert(
                            dependent_false_v<std::decay_t<decltype(m)>>,
                            "unhandled matcher type");
                    }
                },
                *tb_first)) ||
           (s_first == s_last && std::all_of(tb_first, tb_last, does_allow_zero_occurrences));
}

bool does_match(std::string_view s, matcher_table_t const& tb)
{
    return does_match(s.cbegin(), s.cend(), tb.cbegin(), tb.cend());
}

} // namespace

namespace regexp
{

bool does_match(std::string_view s, std::string_view p)
{
    return does_match(s, convert_to_table(p));
}

} // namespace regexp
