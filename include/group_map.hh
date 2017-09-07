#ifndef IVANP_GROUP_MAP_HH
#define IVANP_GROUP_MAP_HH

#include <vector>
#include <string>
#include <unordered_map>

#include "error.hh"

template <typename T>
struct deref_adapter : T {
  using T::T;
  template <typename... Args>
  inline auto operator()(const Args&... args) const {
    return T::operator()(*args...);
  }
};

template <typename T, typename Key = std::string,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key> >
class group_map {
  std::unordered_map<Key,std::vector<T>,Hash,KeyEqual> map;
  std::vector<typename decltype(map)::iterator> groups;

  class iter {
    typename decltype(groups)::iterator it;
  public:
    iter(decltype(it) it): it(it) { }
    inline auto& operator*() noexcept { return **it; }
    inline auto* operator->() noexcept { return *it; }
    inline iter& operator++() noexcept(noexcept(++it)) { ++it; return *this; }
    inline bool operator==(const iter& r) const noexcept { return it == r.it; }
    inline bool operator!=(const iter& r) const noexcept { return it != r.it; }
  };

  class error : ivanp::error { using ivanp::error::error; };

public:
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

  inline iter begin() noexcept { return groups.begin(); }
  inline iter   end() noexcept { return groups.  end(); }
  inline auto  size() noexcept { return groups. size(); }
};

#endif
