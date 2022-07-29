set(BGFX_BUILD_TOOLS     ON  CACHE INTERNAL "")
set(BGFX_BUILD_EXAMPLES  OFF CACHE INTERNAL "")
set(BGFX_CUSTOM_TARGETS  OFF CACHE INTERNAL "")
set(BGFX_OPENGL_VERSION  33  CACHE INTERNAL "")
add_subdirectory(bgfx.cmake)

include(tinygltf.cmake)

include(imgui.cmake)