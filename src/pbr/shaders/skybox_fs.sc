$input v_frag_pos

#include <bgfx_shader.sh>

SAMPLERCUBE(s_skybox, 0);

void main() {
    vec3 env_color = textureCube(s_skybox, v_frag_pos).rgb;
    // TODO: hdr in a separate pass
    env_color = env_color / (env_color + vec3(1.0f));
    env_color = pow(env_color, vec3(1.0f / 2.2f));

    gl_FragColor = vec4(env_color, 1.0f);
}