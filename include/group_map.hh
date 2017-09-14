#ifndef IVANP_GROUP_MAP_HH
#define IVANP_GROUP_MAP_HH

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "error.hh"

template <typename T>
struct deref_pred : T {
  using T::T;
  template <typename... Args>
  inline auto operator()(const Args&... args) const {
    return T::operator()(*args...);
  }
};

template <typename Iterator>
class deref_iterator {
  Iterator it;
public:
  deref_iterator(Iterator it): it(it) { }
  inline auto& operator*() noexcept { return **it; }
  inline auto* operator->() noexcept { return *it; }
  inline auto& operator++() noexcept(noexcept(++it)) { ++it; return *this; }
  inline bool operator==(const deref_iterator& r) const noexcept
  { return it == r.it; }
  inline bool operator!=(const deref_iterator& r) const noexcept
  { return it != r.it; }
};

template <typename T, typename Key = std::string,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key> >
class group_map {
  std::unordered_map<Key,std::vector<T>,Hash,KeyEqual> map;
  std::vector<typename decltype(map)::iterator> groups;

  class error : ivanp::error { using ivanp::error::error; };

public:
  using iterator = deref_iterator<typename decltype(groups)::iterator>;

  template <typename K>
  auto& operator[](K&& key) {
    auto it = map.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(std::forward<K>(key)),
      std::forward_as_tuple());
    if (it.second) groups.push_back(it.first);
    return it.first->second;
  }
  const auto& operator[](const Key& key) const {
    try {
      return map.at(key);
    } catch (const std::out_of_range&) {
      throw error("no key \"",key,'\"');
    }
  }

  inline iterator begin() noexcept { return groups.begin(); }
  inline iterator   end() noexcept { return groups.  end(); }
  inline auto      size() noexcept { return groups. size(); }

  void sort() {
    using it_t = typename decltype(groups)::value_type;
    std::sort( groups.begin(), groups.end(),
      [](const it_t& a, const it_t& b) { return a->first < b->first; }
    );
  }
  template <typename Pred>
  void sort(Pred&& pred) {
    using it_t = typename decltype(groups)::value_type;
    std::sort( groups.begin(), groups.end(),
      [&pred](const it_t& a, const it_t& b) {
        return pred(a->first, b->first);
      }
    );
  }
};

#endif
