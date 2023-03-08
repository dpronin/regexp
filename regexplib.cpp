#include <cstdint>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {

struct matcher_range {
  std::vector<char> cs;
};

struct matcher_strict : matcher_range {};

struct matcher_strict_spec_one_char {
  char c;
};

struct matcher_strict_any_one_char {};

struct matcher_zero_more_spec_char {
  char c;
};

struct matcher_zero_more_any_char {};

struct matcher_one_of_char : matcher_range {
  int m, n;
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

static constexpr auto is_strict_matcher = [](matcher_t const &m) {
  return std::holds_alternative<matcher_strict>(m) ||
         std::holds_alternative<matcher_strict_spec_one_char>(m) ||
         std::holds_alternative<matcher_strict_any_one_char>(m) ||
         (std::holds_alternative<matcher_one_of_char>(m) &&
          std::get<matcher_one_of_char>(m).m > 0);
};

matcher_table_t convert_to_table(std::string_view p) {
  matcher_table_t table;

  auto is_zero_more_char_matcher = [](matcher_t const &m) {
    return std::holds_alternative<matcher_zero_more_spec_char>(m);
  };

  auto is_zero_more_spec_char_matcher_with = [&](matcher_t const &m, char c) {
    return is_zero_more_char_matcher(m) &&
           std::get<matcher_zero_more_spec_char>(m).c == c;
  };

  auto is_zero_more_any_char_matcher = [](matcher_t const &m) {
    return std::holds_alternative<matcher_zero_more_any_char>(m);
  };

  bool one_of = false;
  int f = 0, i = 0;
  for (; i < p.size(); ++i) {
    switch (p[i]) {
    case '[':
      if (one_of)
        throw std::invalid_argument("unexpected '[' inside [] expression");
      if (f < i)
        table.push_back(matcher_strict{{{p.cbegin() + f, p.cbegin() + i}}});
      f = i + 1;
      one_of = true;
      break;
    case ']': {
      if (f == i)
        throw std::invalid_argument("impossible empty [] expression");
      std::vector<char> cs{p.cbegin() + f, p.cbegin() + i};
      std::sort(cs.begin(), cs.end());
      cs.erase(std::unique(cs.begin(), cs.end()), cs.end());
      table.push_back(matcher_one_of_char{{std::move(cs)}, 1, 1});
      one_of = false;
      f = i + 1;
    } break;
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
        auto &m = std::get<matcher_one_of_char>(table.back());
        m.m = '*' == p[i] ? 0 : 1;
        m.n = std::numeric_limits<int>::max();
      } break;
      default:
        if (table.empty() ||
            !is_zero_more_any_char_matcher(table.back()) &&
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
  }

  if (one_of)
    throw std::invalid_argument("not terminated [] expression");

  if (f < i)
    table.push_back(matcher_strict{{{p.cbegin() + f, p.cbegin() + i}}});

  return table;
}

bool does_match(std::string_view s, int f, int l, matcher_table_t const &tb,
                int tbi) {
  return (f < l && tbi < tb.size() &&
          std::visit(
              [&](auto const &m) {
                if constexpr (std::is_same_v<std::decay_t<decltype(m)>,
                                             matcher_strict>) {
                  auto const cmp = [](auto sc, auto pc) {
                    return '.' == pc || sc == pc;
                  };
                  return std::equal(
                             s.cbegin() + f,
                             s.cbegin() + f +
                                 std::min(l - f, static_cast<int>(m.cs.size())),
                             m.cs.cbegin(), m.cs.cend(), cmp) &&
                         does_match(s, f + m.cs.size(), l, tb, tbi + 1);
                } else if constexpr (std::is_same_v<
                                         std::decay_t<decltype(m)>,
                                         matcher_strict_spec_one_char>) {
                  return m.c == s[f] && does_match(s, f + 1, l, tb, tbi + 1);
                } else if constexpr (std::is_same_v<
                                         std::decay_t<decltype(m)>,
                                         matcher_strict_any_one_char>) {
                  return does_match(s, f + 1, l, tb, tbi + 1);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(m)>,
                                                    matcher_one_of_char>) {
                  auto const any_of = [&](auto c) {
                    return std::any_of(m.cs.cbegin(), m.cs.cend(),
                                       [c](auto pc) { return c == pc; });
                  };

                  int k = 0;
                  for (; k < m.m && f < l && any_of(s[f]); ++k, ++f)
                    ;

                  if (k == m.m) {
                    if (does_match(s, f, l, tb, tbi + 1))
                      return true;

                    for (; k < m.n && f < l && any_of(s[f]) &&
                           !does_match(s, f + 1, l, tb, tbi + 1);
                         ++k, ++f)
                      ;

                    return f < l && any_of(s[f]);
                  }

                  return false;
                } else if (does_match(s, f, l, tb, tbi + 1)) {
                  return true;
                } else if constexpr (std::is_same_v<
                                         std::decay_t<decltype(m)>,
                                         matcher_zero_more_spec_char>) {
                  for (; f < l && m.c == s[f] &&
                         !does_match(s, f + 1, l, tb, tbi + 1);
                       ++f)
                    ;
                  return f < l && m.c == s[f];
                } else if constexpr (std::is_same_v<
                                         std::decay_t<decltype(m)>,
                                         matcher_zero_more_any_char>) {
                  for (; f < l && !does_match(s, f + 1, l, tb, tbi + 1); ++f)
                    ;
                  return f < l;
                } else {
                  static_assert(dependent_false_v<std::decay_t<decltype(m)>>,
                                "unhandled matcher type");
                }
              },
              tb[tbi])) ||
         (f == l &&
          std::all_of(tb.cbegin() + tbi, tb.cend(),
                      [](auto const &m) { return !is_strict_matcher(m); }));
}

} // namespace

namespace regexp {
bool does_match(std::string_view s, std::string_view p) {
  return does_match(s, 0, s.size(), convert_to_table(p), 0);
}
} // namespace regexp
