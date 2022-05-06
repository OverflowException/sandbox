$input v_frag_pos

#include <bgfx_shader.sh>

SAMPLERCUBE(s_skybox, 0);

void main() {
    gl_FragColor = textureCube(s_skybox, v_frag_pos);
}