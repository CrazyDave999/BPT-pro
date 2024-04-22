#ifndef BPT_PRO_UTILS_H
#define BPT_PRO_UTILS_H
#include <cstring>
#include <string>
namespace CrazyDave {

template <size_t L>
class String {
  char str_[L]{'\0'};
 public:
  String() = default;
  String(const char *s) { strcpy(str_, s); }
  String(const std::string &s) { strcpy(str_, s.c_str()); }
  explicit operator const char *() { return str_; }
  operator const std::string() { return std::move(std::string(str_)); }
  const char *c_str() { return str_; }
  auto operator[](int pos) -> char & { return str_[pos]; }
  auto operator=(const String &rhs) -> String & {
    if (this == &rhs) {
      return *this;
    }
    strcpy(str_, rhs.str_);
    return *this;
  }
  auto operator=(const char *s) -> String & {
    strcpy(str_, s);
    return *this;
  }
  auto operator=(const std::string &s) -> String & {
    strcpy(str_, s.c_str());
    return *this;
  }
  auto operator==(const String &rhs) const -> bool { return !strcmp(str_, rhs.str_); }
  auto operator!=(const String &rhs) const -> bool { return strcmp(str_, rhs.str_); }
  auto operator<(const String &rhs) const -> bool { return strcmp(str_, rhs.str_) < 0; }
  friend auto operator>>(std::istream &is, String &rhs) -> std::istream & { return is >> rhs.str_; }
  friend auto operator<<(std::ostream &os, const String &rhs) -> std::ostream & { return os << rhs.str_; }
};

class Comparator;

template <class Tp1, class Tp2>
class Pair {
  friend class Comparator;

 private:
  Tp1 first_{};
  Tp2 second_{};

 public:
  Pair() = default;
  Pair(const Tp1 &first, const Tp2 &second) : first_(first), second_(second) {}
  auto GetFirst() const -> const Tp1 & { return first_; }
  auto GetSecond() const -> const Tp2 & { return second_; }
  auto operator<(const Pair &rhs) const -> bool {
    if (first_ != rhs.first_) {
      return first_ < rhs.first_;
    }
    return second_ < rhs.second_;
  }
};

template class Pair<String<65>, page_id_t>;

using str_t = String<65>;
using key_t = Pair<str_t, page_id_t>;
class Comparator {
 public:
  auto operator()(const key_t &k1, const key_t &k2) const -> int {
    if (k1 < k2) {
      return -1;
    }
    if (k2 < k1) {
      return 1;
    }
    return 0;
  }
  auto operator()(const str_t &s, const key_t &k) const -> int {
    if (s < k.first_) {
      return -1;
    }
    if (k.first_ < s) {
      return 1;
    }
    return 0;
  }
  auto operator()(const key_t &k, const str_t &s) const -> int {
    if (k.first_ < s) {
      return -1;
    }
    if (s < k.first_) {
      return 1;
    }
    return 0;
  }
  auto operator()(const str_t &s1, const str_t &s2) const -> int {
    if (s1 < s2) {
      return -1;
    }
    if (s2 < s1) {
      return 1;
    }
    return 0;
  }
};
template <class T1, class T2>
class pair {
 public:
  T1 first;
  T2 second;
  constexpr pair() : first(), second() {}
  pair(const pair &other) = default;
  pair(pair &&other)  noexcept = default;
  pair(const T1 &x, const T2 &y) : first(x), second(y) {}
  template <class U1, class U2>
  pair(U1 &&x, U2 &&y) : first(x), second(y) {}
  template <class U1, class U2>
  explicit pair(const pair<U1, U2> &other) : first(other.first), second(other.second) {}
  template <class U1, class U2>
  explicit pair(pair<U1, U2> &&other) : first(other.first), second(other.second) {}
};
}  // namespace CrazyDave
#endif  // BPT_PRO_UTILS_H
