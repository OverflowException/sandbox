$input v_frag_pos

#include <bgfx_shader.sh>

SAMPLER2D(s_rt, 0);

void main() {
    vec2 uv = v_frag_pos.xy * vec2(0.5f) + vec2(0.5f);
    vec3 color = texture2D(s_rt, uv).xyz;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    gl_FragColor = vec4(color, 1.0f);
}