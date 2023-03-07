#include <algorithm>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

struct matcher_strict {
  int f, l;
};

struct matcher_zero_more_char {
  char c;
};

struct matcher_zero_more_any {};

using matcher_t =
    std::variant<matcher_strict, matcher_zero_more_char, matcher_zero_more_any>;

using matcher_table_t = std::vector<matcher_t>;

matcher_table_t convert_to_table(std::string_view p) {
  matcher_table_t table;

  auto is_zero_more_char_matcher = [](matcher_t const &m) {
    return std::holds_alternative<matcher_zero_more_char>(m);
  };

  auto is_zero_more_spec_char_matcher_with = [&](matcher_t const &m, char c) {
    return is_zero_more_char_matcher(m) &&
           std::get<matcher_zero_more_char>(m).c == c;
  };

  auto is_zero_more_any_char_matcher = [](matcher_t const &m) {
    return std::holds_alternative<matcher_zero_more_any>(m);
  };

  auto is_strict_matcher = [](matcher_t const &m) {
    return std::holds_alternative<matcher_strict>(m);
  };

  int f = 0, i = 0;
  for (; i < p.size(); ++i) {
    switch (p[i]) {
    case 'a' ... 'z':
    case '.':
      break;
    case '*':
      if (f < i - 1)
        table.push_back(matcher_strict{f, i - 1});
      switch (auto const c = p[i - 1]; c) {
      case 'a' ... 'z':
        if (table.empty() ||
            !is_zero_more_any_char_matcher(table.back()) &&
                !is_zero_more_spec_char_matcher_with(table.back(), c))
          table.push_back(matcher_zero_more_char{c});
        break;
      case '.':
        while (!table.empty() && !is_strict_matcher(table.back()))
          table.pop_back();
        table.push_back(matcher_zero_more_any{});
        break;
      default:
        return {};
      }
      f = i + 1;
      break;
    default:
      throw std::invalid_argument(
          std::string{"parser cannot recognize symbol '"} + p[i] + "'");
    }
  }

  if (f < i)
    table.push_back(matcher_strict{f, i});

  return table;
}

bool does_match(std::string_view s, int f, int l, std::string_view p,
                matcher_table_t const &tb, int tbi) {
  return (f < l && tbi < tb.size() &&
          std::visit(
              [&](auto const &m) {
                if constexpr (std::is_same_v<std::decay_t<decltype(m)>,
                                             matcher_strict>) {
                  int k = m.f;
                  for (; f < l && k < m.l && ('.' == p[k] || s[f] == p[k]);
                       ++f, ++k)
                    ;
                  return m.l == k && does_match(s, f, l, p, tb, tbi + 1);
                } else if (does_match(s, f, l, p, tb, tbi + 1)) {
                  return true;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(m)>,
                                                    matcher_zero_more_char>) {
                  for (; f < l && m.c == s[f] &&
                         !does_match(s, f + 1, l, p, tb, tbi + 1);
                       ++f)
                    ;
                  return f < l && m.c == s[f];
                } else {
                  for (; f < l && !does_match(s, f + 1, l, p, tb, tbi + 1); ++f)
                    ;
                  return f < l;
                }
              },
              tb[tbi])) ||
         (f == l &&
          std::all_of(tb.cbegin() + tbi, tb.cend(), [](auto const &m) {
            return !std::holds_alternative<matcher_strict>(m);
          }));
}

} // namespace

namespace regexp {
bool does_match(std::string_view s, std::string_view p) {
  return does_match(s, 0, s.size(), p, convert_to_table(p), 0);
}
} // namespace regexp
