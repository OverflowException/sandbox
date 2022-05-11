$input v_frag_pos

#include <bgfx_shader.sh>

void main() {
    gl_FragColor = vec4(v_frag_pos, 1.0f);
}