#ifndef IVANP_SHARED_STR_HH
#define IVANP_SHARED_STR_HH

#include <memory>

using shared_str = std::shared_ptr<std::string>;

template <typename... Args>
inline auto make_shared_str(Args&&... args) {
  return std::make_shared<typename shared_str::element_type>(
    std::forward<Args>(args)... );
}

#endif
