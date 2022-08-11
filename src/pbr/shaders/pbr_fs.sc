$input v_frag_pos
$input v_frag_norm
$input v_texcoord0
$input v_tangent

$input v_frag_pos_light_space_ndc_0
$input v_frag_pos_light_space_ndc_1
$input v_frag_pos_light_space_ndc_2
$input v_frag_pos_light_space_ndc_3

#include <bgfx_shader.sh>
#include "shadows.sc"
#include "material.sc"
#include "oit.sc"

#define MAX_LIGHT_COUNT 4

uniform vec4 u_light_dirs[MAX_LIGHT_COUNT];
uniform vec4 u_light_colors[MAX_LIGHT_COUNT];
uniform vec4 u_light_count;

SAMPLER2D(s_shadowmaps_atlas, 0);

uniform vec4 u_view_pos;

uniform vec4 u_albedo;
uniform vec4 u_metallic_roughness_ao;

void main() {
    // compute shadowing factor of each light
    int light_num = int(min(u_light_count.x, MAX_LIGHT_COUNT));
    vec3 frag_pos_light_space_ndc[MAX_LIGHT_COUNT];
    frag_pos_light_space_ndc[0] = v_frag_pos_light_space_ndc_0;
    frag_pos_light_space_ndc[1] = v_frag_pos_light_space_ndc_1;
    frag_pos_light_space_ndc[2] = v_frag_pos_light_space_ndc_2;
    frag_pos_light_space_ndc[3] = v_frag_pos_light_space_ndc_3;
    
    float shadow_visibility[MAX_LIGHT_COUNT];
    for(int i = 0; i < light_num; ++i) {
        shadow_visibility[i] = shadow_visibility_pcf(s_shadowmaps_atlas,
                                                     frag_pos_light_space_ndc[i],
                                                     -normalize(vec3(u_light_dirs[i])),
                                                     v_frag_norm,
                                                     i,
                                                     MAX_LIGHT_COUNT);
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
