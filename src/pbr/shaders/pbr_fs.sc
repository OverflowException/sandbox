$input v_frag_pos
$input v_frag_norm
$input v_texcoord0
$input v_tangent

#define MAX_LIGHT_COUNT     4
#define CASCADE_COUNT       3
#define SHADOW_RESOLUTION   2048

#include <bgfx_shader.sh>
#include "shadows.sc"
#include "material.sc"
#include "oit.sc"

uniform vec4 u_light_dirs[MAX_LIGHT_COUNT];
uniform vec4 u_light_colors[MAX_LIGHT_COUNT];
uniform vec4 u_light_count;
uniform vec4 u_cascade_thresholds_ndc;
uniform mat4 u_light_space_view_proj[MAX_LIGHT_COUNT * CASCADE_COUNT];

SAMPLER2D(s_shadowmaps_atlas, 0);

uniform vec4 u_view_pos;

uniform vec4 u_albedo;
uniform vec4 u_metallic_roughness_ao;

void main() {
    // compute shadowing factor of each light
    vec3 frag_pos_light_space_ndc[MAX_LIGHT_COUNT];
    int light_num = int(min(u_light_count.x, MAX_LIGHT_COUNT));
    int cascade_id = pick_cascade(u_cascade_thresholds_ndc, vec3(gl_FragCoord) * vec3(2.0f) - vec3(1.0f));
    for (int light_id = 0; light_id < light_num; ++light_id) {
        frag_pos_light_space_ndc[light_id] = light_space_frag_ndc(u_light_space_view_proj[cascade_id * light_num + light_id],
                                                           v_frag_pos);
    }

    float shadow_visibility[MAX_LIGHT_COUNT];
    for(int i = 0; i < light_num; ++i) {
        shadow_visibility[i] = shadow_visibility_pcf(s_shadowmaps_atlas,
                                                     frag_pos_light_space_ndc[i],
                                                     -normalize(vec3(u_light_dirs[i])),
                                                     v_frag_norm,
                                                     i,
                                                     cascade_id);
    }

    vec3 albedo = vec3(u_albedo);
    float roughness = u_metallic_roughness_ao[1];
    float metallic = u_metallic_roughness_ao[0];
    float ao = u_metallic_roughness_ao[2];
    vec3 n = normalize(v_frag_norm);
    vec3 v = normalize(vec3(u_view_pos) - v_frag_pos);

    // reflectance equation
    vec3 lo = vec3(0.0f);
    for(int i = 0; i < light_num; ++i) {
        // // calculate per-light radiance
        vec3 l = -normalize(vec3(u_light_dirs[i]));
        lo += reflectance(albedo, metallic, roughness, ao,
                          vec3(u_light_colors[i]), shadow_visibility[i],
                          l, n, v);
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + lo;

    // color cascade, for debugging
    // if (cascade_id == 0) {
    //     color = color * vec3(1.0f, 0.3f, 0.3f);
    // } else if (cascade_id == 1) {
    //     color = color * vec3(0.3f, 1.0f, 0.3f);
    // } else if (cascade_id == 2) {
    //     color = color * vec3(0.3f, 0.3f, 1.0f);
    // } else {
    //     color = color * vec3(1.0f, 1.0f, 1.0f);
    // }

    if (u_albedo.a < 1.0f) {
        // translucent primitive fragment
        float depth = -depth_eye_space(gl_FragCoord, u_invProj);
        float alpha = u_albedo.a;
        float weight = distance_weight(vec4(color, alpha), depth, 2);

        // accumu
        gl_FragData[0] = vec4(color * alpha, alpha) * weight;
        // gl_FragData[0] = vec4(depth, weight, 0.0f, 0.0f);
        // revealage
        gl_FragData[1] = vec4(alpha);
    } else {
        // opaque primitive
        gl_FragData[0] = vec4(color, 1.0f);
    }
}
