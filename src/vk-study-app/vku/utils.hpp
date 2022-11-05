#pragma once

#include <functional>
#include <filesystem>

namespace vku {
#if !defined( NDEBUG )
  constexpr bool isDebugBuild = true;
#else
  constexpr bool isDebugBuild = false;
#endif

  const std::filesystem::path assetsRootFolder = ASSETS_ROOT_FOLDER;

  template <class T>
  inline void hash_combine(std::size_t& s, const T& v)
  {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
  }
}