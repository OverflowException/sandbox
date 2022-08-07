$input a_position
$input a_normal      // should be normalized before submitting
$input a_texcoord0
$input a_tangent     // should be normalized before submitting

$output v_frag_pos
$output v_frag_norm
$output v_texcoord0
$output v_tangent

$output v_frag_pos_light_space_ndc_0
$output v_frag_pos_light_space_ndc_1
$output v_frag_pos_light_space_ndc_2
$output v_frag_pos_light_space_ndc_3

#include <bgfx_shader.sh>
#include "shadows.sc"

#define MAX_LIGHT_COUNT 4

uniform mat4 u_model_inv_t;

uniform vec4 u_light_count;
uniform mat4 u_light_space_view_proj[MAX_LIGHT_COUNT];


void main() {
    v_frag_pos = vec3(u_model[0] * vec4(a_position, 1.0f));
    v_frag_norm = vec3(u_model_inv_t * vec4(a_normal, 0.0f));
    v_texcoord0 = a_texcoord0;
    v_tangent = vec3(u_model[0] * vec4(a_tangent, 0.0f));

    gl_Position = u_modelViewProj * vec4(a_position, 1.0);

    // light space computations
    int light_num = int(min(u_light_count.x, MAX_LIGHT_COUNT));
    if (light_num > 0) {
        v_frag_pos_light_space_ndc_0 = light_space_frag_ndc(u_light_space_view_proj[0],
                                                            v_frag_pos);
    }
    if (light_num > 1) {
        v_frag_pos_light_space_ndc_1 = light_space_frag_ndc(u_light_space_view_proj[1],
                                                            v_frag_pos);
    }
    if (light_num > 2) {
        v_frag_pos_light_space_ndc_2 = light_space_frag_ndc(u_light_space_view_proj[2],
                                                            v_frag_pos);
    }
    if (light_num > 3) {
        v_frag_pos_light_space_ndc_3 = light_space_frag_ndc(u_light_space_view_proj[3],
                                                            v_frag_pos);
    }
}