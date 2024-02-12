#ifndef _MMAP_ALLOCATOR_H
#define _MMAP_ALLOCATOR_H


#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <cmath>
#include <metall/c_api/metall.h>


namespace mm {

template <typename T>
class Allocator {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = off_t;

  template <class U>
  friend class Allocator;

  template <class U>
  struct rebind {
    typedef Allocator<U> other;
  };


  Allocator() = default;
    

  template <class U>
  Allocator(const Allocator<U>& other) noexcept {}

  T* allocate(size_t n) {
    //std::cout << "I'm allocating!\n";
    return (T*) metall_malloc(n*sizeof(T));
  }

  void deallocate(T* p, size_t n) {
    metall_free(p);
  }

  void construct(pointer p, const_reference val) { new ((void*)p) T(val); }

  void destroy(pointer p) { p->~T(); }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new ((void*)p) U(std::forward<Args>(args)...);
  }


  template <class U>
  void destroy(U* p) {
    p->~U();
  }

};

template <typename T, typename U>
inline bool operator==(const Allocator<T>& a, const Allocator<U>& b) {return true;};

template <typename T, typename U>
inline bool operator!=(const Allocator<T>& a, const Allocator<U>& b) {return false;};

// Convenience typedefs of STL types
template <typename T>
using vector = std::vector<T, Allocator<T>>;

template <class Key, class T, class Compare = std::less<Key>>
using map = std::map<Key, T, Compare, Allocator<std::pair<const Key, T>>>;

template<class T, class Compare = std::less<T>>
using set = std::set<T, Compare, Allocator<T>>;

template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator<std::pair<const Key, T>>>;

template <class T, class Hash = std::hash<T>, class KeyEqual = std::equal_to<T>>
using unordered_set = std::unordered_set<T, Hash, KeyEqual, Allocator<T>>;

};  // namespace mm

#endif