#pragma once

namespace vku {
#if !defined( NDEBUG )
  constexpr bool isDebugBuild = true;
#else
  constexpr bool isDebugBuild = false;
#endif

}