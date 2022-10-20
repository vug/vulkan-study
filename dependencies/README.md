# Dependencies

One way to add a dependency project is to use Git submodules. See: <https://training.github.com/downloads/submodule-vs-subtree-cheat-sheet/>

Example:

```cmd
git submodule add https://github.com/blend2d/blend2d
```

Submodules can be cloned with the repo via

```cmd
git clone --recurse-submodules https://github.com/vug/graphics-stuff.git
```

or pulled later via

```cmd
git submodule update --init --recursive
```

# VulkanSDK

Vulkan is not a submodule. It needs to be downloaded from LunarG website, https://www.lunarg.com/vulkan-sdk/, and installed with the installer.

# GLFW

* <https://www.glfw.org/>

Added to the project via (from `dependencies/`):

```cmd
git submodule add https://github.com/glfw/glfw.git
```

## Extra knowledge: Building GLFW lib manually (not needed for our projects)

```cmd
cd glfw
mkdir build
cd build
cmake .. -L
cmake .. -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=Off -DGLFW_BUILD_TESTS=OFF -DGLFW_INSTALL=OFF
cmake --build . --config Release
cmake --build . --config Debug
```

GLFW build options

```txt
BUILD_SHARED_LIBS:BOOL=OFF
CMAKE_CONFIGURATION_TYPES:STRING=Debug;Release;MinSizeRel;RelWithDebInfo
CMAKE_INSTALL_PREFIX:PATH=C:/Program Files (x86)/GLFW
GLFW_BUILD_DOCS:BOOL=ON
GLFW_BUILD_EXAMPLES:BOOL=ON
GLFW_BUILD_TESTS:BOOL=ON
GLFW_BUILD_WIN32:BOOL=ON
GLFW_INSTALL:BOOL=ON
GLFW_LIBRARY_TYPE:STRING=
GLFW_USE_HYBRID_HPG:BOOL=OFF
USE_MSVC_RUNTIME_LIBRARY_DLL:BOOL=ON
```

outputs

* PROJECT_ROOT\dependencies\glfw\build\src\Release\glfw3.lib
* PROJECT_ROOT\stuff\dependencies\glfw\build\src\Debug\glfw3.lib

To compile a GLFW based project via Visual Studio compliler from commandline

```cmd
cl /std:c++20 /W4 /external:I"../dependencies" /external:W0 /I"../dependencies/glfw/include" ../dependencies/glfw/build/src/Release/glfw3.lib Opengl32.lib User32.lib Gdi32.lib Shell32.lib /MD /EHsc main.cpp
```

Also choose appropriate optimization flags, or use `/Zi` for generating debug symbols

# vk-bootstrap

* https://github.com/charles-lunarg/vk-bootstrap
* [vk\-bootstrap/getting\_started\.md at master · charles\-lunarg/vk\-bootstrap](https://github.com/charles-lunarg/vk-bootstrap/blob/master/docs/getting_started.md)

```cmd
git submodule add https://github.com/charles-lunarg/vk-bootstrap.git
```