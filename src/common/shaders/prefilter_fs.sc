$input v_frag_pos // in world space

#include <bgfx_shader.sh>
#include "sampling.sc"

SAMPLERCUBE(s_env, 0);

uniform vec4 u_roughness;

void main() {
    vec3 n = normalize(v_frag_pos);
    vec3 r = n;
    vec3 v = r;

    const int sample_count = 1024;
    vec3 color = vec3(0.0f);
    float total_weight = 0.0f;

    for (int i = 0; i < sample_count; ++i) {
        vec2 xi = hammersley(i, sample_count);
        vec3 h = importance_sample_ggx(xi, n, u_roughness[0]);
        vec3 l  = normalize(2.0 * dot(v, h) * h - v);

        float n_l = max(dot(n, l), 0.0);
        if(n_l > 0.0) {
            color += textureCube(s_env, l).rgb * n_l;
            total_weight += n_l;
        }
    }
    color = color / total_weight;
    gl_FragColor = vec4(color, 1.0f);
    // gl_FragColor = textureCube(s_env, v_frag_pos);
}