#include <iterator>
#include <type_traits>

class collator_end { };

template <typename T>
class collator {
public:
  std::decay_t<decltype(std::begin(std::declval<T&>()))> it;
  int i=0, n, s;

  using iterator_category = std::forward_iterator_tag;
  using reference = decltype(*it);
  using pointer = decltype(it);
  using value_type = std::remove_reference_t<reference>;
  using difference_type =
    typename std::iterator_traits<decltype(it)>::difference_type;

public:
  collator(T& xs, int f): it(std::begin(xs)), n(std::size(xs)), s(n/f) {
    if (s < 1) s = 1;
  }

  collator&  begin() noexcept { return *this; }
  collator_end end() noexcept { return { }; }

  reference operator* () const { return *std::next(it,i); }
  pointer   operator->() const { return  std::next(it,i); }

  collator& operator++() noexcept {
    if (i >= 0) {
      i += s;
      if (i >= n) {
        ++i %= s;
        if (i==0) --i;
      }
    }
    return *this;
  }

  friend bool operator==(const collator& c, collator_end) { return c.i <  0; }
  friend bool operator==(collator_end, const collator& c) { return c.i <  0; }
  friend bool operator!=(const collator& c, collator_end) { return c.i >= 0; }
  friend bool operator!=(collator_end, const collator& c) { return c.i >= 0; }
};
