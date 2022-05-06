set(CUR_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CUR_BIN_DIR}/pbr)

set(EXEC_NAME pbr)

file(GLOB SRC ./*.cpp)

add_executable(${EXEC_NAME} ${SRC} shaders/pbr_vs.sc shaders/pbr_fs.sc
                                    shaders/skybox_vs.sc shaders/skybox_fs.sc)

target_link_libraries(${EXEC_NAME} PUBLIC graph_arch procedural bimg)

add_shader(shaders/pbr_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/pbr_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_shader(shaders/skybox_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/skybox_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

add_custom_command(TARGET ${EXEC_NAME}
                    POST_BUILD
                    COMMAND cp -frv ${CMAKE_CURRENT_SOURCE_DIR}/textures ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
