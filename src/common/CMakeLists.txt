add_library(graph_arch STATIC application.cpp ${SHADER_SRC})
target_link_libraries(graph_arch PUBLIC imgui_bgfx)

add_library(imgui_bgfx STATIC imgui_bgfx.cpp)
target_link_libraries(imgui_bgfx PUBLIC bx bgfx imgui glfw)

add_library(procedural STATIC procedural_shapes.cpp)

add_library(file_io STATIC file_io.cpp)
target_link_libraries(file_io PUBLIC bx bimg bgfx)

#add_library(gltf_loader STATIC gltf_loader.cpp)
#target_link_libraries(gltf_loader PUBLIC tinygltf)

add_library(renderer STATIC renderer.cpp)
target_link_libraries(renderer PUBLIC bgfx bimg file_io)

file(GLOB SHADER_SRC ./shaders/*.sc)
add_library(pre_computations STATIC pre_computations.cpp ${SHADER_SRC})
target_link_libraries(pre_computations PUBLIC bx bimg bgfx file_io)

#shaders for pre_computations
add_shader(shaders/skybox_vs.sc VERTEX OUTPUT ${RUNTIME_OUTPUT_DIRECTORY}/common_shaders GLSL 330)
add_shader(shaders/irradiance_convolution_fs.sc FRAGMENT OUTPUT ${RUNTIME_OUTPUT_DIRECTORY}/common_shaders GLSL 330)
add_shader(shaders/prefilter_fs.sc FRAGMENT OUTPUT ${RUNTIME_OUTPUT_DIRECTORY}/common_shaders GLSL 330)
add_shader(shaders/brdf_lut_vs.sc VERTEX OUTPUT ${RUNTIME_OUTPUT_DIRECTORY}/common_shaders GLSL 330)
add_shader(shaders/brdf_lut_fs.sc FRAGMENT OUTPUT ${RUNTIME_OUTPUT_DIRECTORY}/common_shaders GLSL 330)