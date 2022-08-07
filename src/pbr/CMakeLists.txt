set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/pbr)

set(EXEC_NAME pbr)

file(GLOB SRC ./*.cpp)
file(GLOB SHADER_SRC ./shaders/*.sc)

add_executable(${EXEC_NAME} ${SRC} ${SHADER_SRC})

target_link_libraries(${EXEC_NAME} PUBLIC graph_arch procedural pre_computations renderer)

add_shader(shaders/pbr_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/pbr_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_shader(shaders/skybox_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/skybox_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_shader(shaders/shadowmap_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/shadowmap_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_shader(shaders/tonemapping_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/tonemapping_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_custom_command(TARGET ${EXEC_NAME}
                    POST_BUILD
                    COMMAND cp -frv ${CMAKE_CURRENT_SOURCE_DIR}/textures ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
