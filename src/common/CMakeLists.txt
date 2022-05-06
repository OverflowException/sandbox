add_library(graph_arch STATIC application.cpp)
#target_include_directories(graph_arch PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty)
target_link_libraries(graph_arch PUBLIC imgui_bgfx)

add_library(imgui_bgfx STATIC imgui_bgfx.cpp)
#target_include_directories(imgui_bgfx PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty)
target_link_libraries(imgui_bgfx PUBLIC bx bgfx imgui glfw)

add_library(procedural STATIC procedural_shapes.cpp)
