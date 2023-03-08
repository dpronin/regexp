#include <cstdint>

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace
{

struct matcher_range {
    std::vector<char> cs;
};

struct matcher_strict : matcher_range {
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

struct matcher_one_of_char : matcher_range {
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

template <typename... Args>
static inline constexpr bool dependent_false_v = false;

static constexpr auto is_strict_matcher = [](matcher_t const& m) {
    return std::holds_alternative<matcher_strict>(m) ||
           std::holds_alternative<matcher_strict_spec_one_char>(m) ||
           std::holds_alternative<matcher_strict_any_one_char>(m) ||
           (std::holds_alternative<matcher_one_of_char>(m) &&
            std::get<matcher_one_of_char>(m).m > 0);
};

enum converter_mode {
    kDefault,
    kOneOf,
    kQty,
};

using converter_handler_t = std::function<std::tuple<uint32_t, uint32_t, converter_mode>(
    uint32_t f, uint32_t i, uint32_t l, std::string_view p, matcher_table_t& table)>;

constexpr auto converter_default =
    [](uint32_t f, uint32_t i, uint32_t l, std::string_view p, matcher_table_t& table)
    -> std::tuple<uint32_t, uint32_t, converter_mode> {
    auto is_zero_more_char_matcher = [](matcher_t const& m) {
        return std::holds_alternative<matcher_zero_more_spec_char>(m);
    };

    auto is_zero_more_spec_char_matcher_with = [&](matcher_t const& m, char c) {
        return is_zero_more_char_matcher(m) && std::get<matcher_zero_more_spec_char>(m).c == c;
    };

    auto is_zero_more_any_char_matcher = [](matcher_t const& m) {
        return std::holds_alternative<matcher_zero_more_any_char>(m);
    };

    converter_mode next_mode = converter_mode::kDefault;

    switch (p[i]) {
        case '[':
            if (f < i)
                table.push_back(matcher_strict{{{p.cbegin() + f, p.cbegin() + i}}});
            f         = i + 1;
            next_mode = converter_mode::kOneOf;
            break;
        case ']':
            throw std::invalid_argument("unexpected ']' in not opened oneof [] expression");
        case '*':
        case '+':
            if (f < i - 1)
                table.push_back(matcher_strict{{{p.cbegin() + f, p.cbegin() + i - 1}}});
            switch (auto const c = p[i - 1]; c) {
                case '.':
                    while (!table.empty() && !is_strict_matcher(table.back()))
                        table.pop_back();
                    table.push_back(matcher_zero_more_any_char{});
                    if ('+' == p[i])
                        table.push_back(matcher_strict_any_one_char{});
                    break;
                case ']': {
                    auto& m = std::get<matcher_one_of_char>(table.back());
                    m.m     = '*' == p[i] ? 0 : 1;
                    m.n     = std::numeric_limits<decltype(m.n)>::max();
                } break;
                default:
                    if (table.empty() || !is_zero_more_any_char_matcher(table.back()) &&
                                             !is_zero_more_spec_char_matcher_with(table.back(), c))
                        table.push_back(matcher_zero_more_spec_char{c});
                    if ('+' == p[i])
                        table.push_back(matcher_strict_spec_one_char{c});
                    break;
            }
            f = i + 1;
            break;
        default:
            break;
    }

    return {f, i + 1, next_mode};
};

constexpr auto converter_oneof =
    [](uint32_t f, uint32_t i, uint32_t l, std::string_view p, matcher_table_t& table)
    -> std::tuple<uint32_t, uint32_t, converter_mode> {
    auto is_zero_more_char_matcher = [](matcher_t const& m) {
        return std::holds_alternative<matcher_zero_more_spec_char>(m);
    };

    auto is_zero_more_spec_char_matcher_with = [&](matcher_t const& m, char c) {
        return is_zero_more_char_matcher(m) && std::get<matcher_zero_more_spec_char>(m).c == c;
    };

    auto is_zero_more_any_char_matcher = [](matcher_t const& m) {
        return std::holds_alternative<matcher_zero_more_any_char>(m);
    };

    converter_mode next_mode = converter_mode::kOneOf;

    switch (p[i]) {
        case ']': {
            if (f == i)
                throw std::invalid_argument("empty oneof [] expression is impossible");
            std::vector<char> cs{p.cbegin() + f, p.cbegin() + i};
            std::sort(cs.begin(), cs.end());
            cs.erase(std::unique(cs.begin(), cs.end()), cs.end());
            table.push_back(matcher_one_of_char{{std::move(cs)}, 1, 1});
            f         = i + 1;
            next_mode = converter_mode::kDefault;
        } break;
        case '[':
        case '*':
        case '+':
            throw std::invalid_argument(
                std::string{"unexpected '"} + p[i] + "' inside opened oneof [] expression");
        default:
            break;
    }

    return {f, i + 1, next_mode};
};

matcher_table_t convert_to_table(std::string_view p)
{
    matcher_table_t table;

    uint32_t f          = 0;
    uint32_t i          = 0;
    converter_mode mode = converter_mode::kDefault;

    std::array<converter_handler_t, converter_mode::kQty> const handlers = {
        converter_default,
        converter_oneof,
    };

    while (i < p.size())
        std::tie(f, i, mode) = handlers[mode](f, i, p.size(), p, table);

    if (converter_mode::kOneOf == mode)
        throw std::invalid_argument("not terminated [] expression");

    if (f < i)
        table.push_back(matcher_strict{{{p.cbegin() + f, p.cbegin() + i}}});

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
           (s_first == s_last &&
            std::all_of(tb_first, tb_last, [](auto const& m) { return !is_strict_matcher(m); }));
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
