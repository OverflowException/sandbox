$input a_position
$input a_normal      // should be normalized before submitting
$input a_texcoord0
$input a_tangent     // should be normalized before submitting

$output v_frag_pos
$output v_frag_norm
$output v_texcoord0
$output v_tangent

#include <bgfx_shader.sh>
#include "shadows.sc"

uniform mat4 u_model_inv_t;

void main() {
    v_frag_pos = vec3(u_model[0] * vec4(a_position, 1.0f));
    v_frag_norm = vec3(u_model_inv_t * vec4(a_normal, 0.0f));
    v_texcoord0 = a_texcoord0;
    v_tangent = vec3(u_model[0] * vec4(a_tangent, 0.0f));

    vec4 pos_clip = u_modelViewProj * vec4(a_position, 1.0);
    gl_Position = pos_clip;
}