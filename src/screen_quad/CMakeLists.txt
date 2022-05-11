set(CUR_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CUR_BIN_DIR}/screen_quad)

set(EXEC_NAME screen_quad)

file(GLOB SRC ./*.cpp)
file(GLOB SHADER_SRC ./shaders/*.sc)

add_executable(${EXEC_NAME} ${SRC} ${SHADER_SRC})

add_shader(shaders/screen_quad_vs.sc VERTEX OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)
add_shader(shaders/screen_quad_fs.sc FRAGMENT OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders GLSL 330)

target_link_libraries(${EXEC_NAME} PUBLIC graph_arch procedural bimg file_io)

