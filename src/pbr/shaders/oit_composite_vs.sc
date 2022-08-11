$input a_position

$output v_frag_pos

#include <bgfx_shader.sh>

void main() {
    v_frag_pos = a_position;
    gl_Position = vec4(a_position.xy, 1.0f, 1.0f);
}