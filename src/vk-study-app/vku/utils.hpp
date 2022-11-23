#pragma once

#include <filesystem>
#include <functional>

namespace vku {
#if !defined(NDEBUG)
constexpr bool isDebugBuild = true;
#else
constexpr bool isDebugBuild = false;
#endif

// TODO: get assetsRootFolder via env variable of command-line argument instead of preprocessor definition
const std::filesystem::path assetsRootFolder = ASSETS_ROOT_FOLDER;

template <class T>
inline void hash_combine(std::size_t& s, const T& v) {
  std::hash<T> h;
  s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}
}  // namespace vku