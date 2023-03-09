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

struct matcher_strict : matcher_range<std::string_view> {
};

struct matcher_strict_spec_one_char {
    char c;
};

struct matcher_strict_any_one_char {
};

struct matcher_zero_more_spec_char {
    char c;
};

struct matcher_zero_more_any_char {
};

struct matcher_one_of_char : matcher_range<std::vector<char>> {
    uint32_t m, n;
};

/* clang-format off */
using matcher_t = std::variant<
  matcher_strict,
  matcher_strict_spec_one_char,
  matcher_strict_any_one_char,
  matcher_zero_more_spec_char,
  matcher_zero_more_any_char,
  matcher_one_of_char
>;
/* clang-format on */

using matcher_table_t = std::vector<matcher_t>;

enum converter_mode {
    kDefault,
    kOneOf,
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

    converter_mode mode = converter_mode::kDefault;
};

template <typename... Args>
constexpr bool dependent_false_v = false;

constexpr auto is_zero_more_char_matcher = [](matcher_t const& m) {
    return std::holds_alternative<matcher_zero_more_spec_char>(m);
};

constexpr auto is_zero_more_spec_char_matcher_with = [](matcher_t const& m, char c) {
    return is_zero_more_char_matcher(m) && std::get<matcher_zero_more_spec_char>(m).c == c;
};

constexpr auto is_zero_more_any_char_matcher = [](matcher_t const& m) {
    return std::holds_alternative<matcher_zero_more_any_char>(m);
};

constexpr auto does_allow_zero_occurrences = [](matcher_t const& m) {
    return is_zero_more_char_matcher(m) || is_zero_more_any_char_matcher(m) ||
           (std::holds_alternative<matcher_one_of_char>(m) &&
            std::get<matcher_one_of_char>(m).m > 0);
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
            table.push_back(matcher_strict{{{ctx.f, ctx.i}}});
        return false;
    }

    switch (auto const c = *ctx.i; c) {
        case '[':
            if (ctx.f < ctx.i)
                table.push_back(matcher_strict{{{ctx.f, ctx.i}}});
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kOneOf;
            break;
        case ']':
            throw std::invalid_argument("unexpected ']' in not opened oneof [] expression");
        case '*':
        case '+':
            if (ctx.f == ctx.i)
                throw std::invalid_argument(
                    std::string{"unexpected '"} + c + "' without previous symbol or expression");

            if (ctx.f < ctx.i - 1)
                table.push_back(matcher_strict{{{ctx.f, ctx.i - 1}}});
            switch (auto const pc = ctx.i[-1]; pc) {
                case '.':
                    while (!table.empty() && does_allow_zero_occurrences(table.back()))
                        table.pop_back();
                    table.push_back(matcher_zero_more_any_char{});
                    if ('+' == *ctx.i)
                        table.push_back(matcher_strict_any_one_char{});
                    break;
                default:
                    if (table.empty() || !is_zero_more_any_char_matcher(table.back()) &&
                                             !is_zero_more_spec_char_matcher_with(table.back(), pc))
                        table.push_back(matcher_zero_more_spec_char{pc});
                    if ('+' == *ctx.i)
                        table.push_back(matcher_strict_spec_one_char{pc});
                    break;
            }
            ctx.f = ctx.i + 1;
            break;
        default:
            break;
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
            if (ctx.f == ctx.i)
                throw std::invalid_argument("empty oneof [] expression is impossible");
            std::vector<char> cs{ctx.f, ctx.i};
            std::sort(cs.begin(), cs.end());
            cs.erase(std::unique(cs.begin(), cs.end()), cs.end());
            matcher_one_of_char m{{std::move(cs)}, 1, 1};
            if (ctx.i + 1 < ctx.l) {
                switch (auto const nc = ctx.i[1]; nc) {
                    case '*':
                        m.m = 0;
                        [[fallthrough]];
                    case '+': {
                        m.n = std::numeric_limits<decltype(m.n)>::max();
                        ++ctx.i;
                    } break;
                    default:
                        break;
                }
            }
            table.push_back(std::move(m));
            ctx.f    = ctx.i + 1;
            ctx.mode = converter_mode::kDefault;
        } break;
        case '[':
        case '*':
        case '+':
            throw std::invalid_argument(
                std::string{"unexpected '"} + c + "' inside opened oneof [] expression");
        default:
            break;
    }
    ++ctx.i;

    return true;
};

std::array<converter_handler_t, converter_mode::kQty> const handlers = {
    converter_default,
    converter_oneof,
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

auto constexpr does_match_with_matcher_strict =
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

auto constexpr does_match_with_matcher_strict_spec_one_char =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        return m.c == *s_first && does_match(s_first + 1, s_last, tb_first + 1, tb_last);
    };

auto constexpr does_match_with_matcher_strict_any_one_char =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        return does_match(s_first + 1, s_last, tb_first + 1, tb_last);
    };

auto constexpr does_match_with_matcher_one_of_char =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        auto const any_of = [&](auto c) {
            return std::any_of(m.cs.cbegin(), m.cs.cend(), [c](auto pc) { return c == pc; });
        };

        uint32_t k = 0;
        for (; k < m.m && s_first < s_last && any_of(*s_first); ++k, ++s_first)
            ;

        if (k == m.m) {
            if (does_match(s_first, s_last, tb_first + 1, tb_last))
                return true;

            for (; k < m.n && s_first < s_last && any_of(*s_first) &&
                   !does_match(s_first + 1, s_last, tb_first + 1, tb_last);
                 ++k, ++s_first)
                ;

            return s_first < s_last && any_of(*s_first);
        }

        return false;
    };

auto constexpr does_match_with_matcher_zero_more_spec_char =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        if (does_match(s_first, s_last, tb_first + 1, tb_last))
            return true;
        for (; s_first < s_last && m.c == *s_first &&
               !does_match(s_first + 1, s_last, tb_first + 1, tb_last);
             ++s_first)
            ;
        return s_first < s_last && m.c == *s_first;
    };

auto constexpr does_match_with_matcher_zero_more_any_char =
    [](auto const& m, auto s_first, auto s_last, auto tb_first, auto tb_last) {
        if (does_match(s_first, s_last, tb_first + 1, tb_last))
            return true;
        for (; s_first < s_last && !does_match(s_first + 1, s_last, tb_first + 1, tb_last);
             ++s_first)
            ;
        return s_first < s_last;
    };

bool does_match(
    std::string_view::const_iterator s_first,
    std::string_view::const_iterator s_last,
    matcher_table_t::const_iterator tb_first,
    matcher_table_t::const_iterator tb_last)
{
    return (s_first < s_last && tb_first < tb_last &&
            std::visit(
                [&](auto const& m) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(m)>, matcher_strict>) {
                        return does_match_with_matcher_strict(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_strict_spec_one_char>) {
                        return does_match_with_matcher_strict_spec_one_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_strict_any_one_char>) {
                        return does_match_with_matcher_strict_any_one_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_one_of_char>) {
                        return does_match_with_matcher_one_of_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_zero_more_spec_char>) {
                        return does_match_with_matcher_zero_more_spec_char(
                            m,
                            s_first,
                            s_last,
                            tb_first,
                            tb_last);
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<decltype(m)>,
                                             matcher_zero_more_any_char>) {
                        return does_match_with_matcher_zero_more_any_char(
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
