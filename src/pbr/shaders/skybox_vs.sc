$input a_position

$output v_frag_pos

#include <bgfx_shader.sh>

void main() {
    v_frag_pos = a_position;
    vec4 pos = u_viewProj * vec4(a_position, 1.0);
    gl_Position = pos.xyww;
}