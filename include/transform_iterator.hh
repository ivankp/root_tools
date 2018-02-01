#ifndef IVANP_TRANSFORM_ITERATOR_HH
#define IVANP_TRANSFORM_ITERATOR_HH

namespace ivanp {

template <typename It, typename F>
class transform_iterator {
  It it;
  F f;

public:
  template <typename _It, typename _F>
  transform_iterator(_It&& it, _F&& f)
  : it(std::forward<_It>(it)), f(std::forward<_F>(f)) { }

  inline auto operator*() { return f(*it); }
  inline auto operator*() const { return f(*it); }

  inline It& operator++() { return ++it; }
  inline It& operator--() { return --it; }

  inline bool operator!=(const transform_iterator& r) const {
    return it != r.it;
  }
  inline bool operator!=(const It& r) const {
    return it != r;
  }
  inline bool operator==(const transform_iterator& r) const {
    return it == r.it;
  }
  inline bool operator==(const It& r) const {
    return it == r;
  }
};

template <typename It, typename F>
inline transform_iterator<It,F> make_transform_iterator(It&& it, F&& f) {
  return { std::forward<It>(it), std::forward<F>(f) };
}

}

#endif
