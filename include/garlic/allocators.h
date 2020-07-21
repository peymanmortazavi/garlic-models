#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <concepts>
#include <cstdlib>

namespace garlic {


  template<typename T> concept Allocator = requires(T t) {
    { T::needs_free } -> std::convertible_to<bool>;

    { t.allocate(1) } -> std::convertible_to<void*>;
    { t.reallocate(nullptr, 0, 1) } -> std::convertible_to<void*>;
    { t.free(nullptr) };
  };

  class CAllocator {
  public:
    static const bool needs_free = true;

    CAllocator() = default;
    CAllocator(const CAllocator& another) = delete;

    void* allocate(size_t size) {
      if (size > 0) return std::malloc(size);
      else return nullptr;
    }

    void* reallocate(void* original, size_t old_size, size_t new_size) {
      if (new_size != 0) return realloc(original, new_size);
      else {
        std::free(original);
        return nullptr;
      }
    }

    void free(void* ptr) { std::free(ptr); }
  };

}

#endif /* end of include guard: ALLOCATORS_H */