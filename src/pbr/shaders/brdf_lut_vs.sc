$input a_position
$output v_frag_pos

#include <bgfx_shader.sh>

void main() {
    v_frag_pos = (a_position + vec3(1.0f)) * vec3(0.5f);
    gl_Position = vec4(a_position.x, a_position.y, 1.0f, 1.0f);
}