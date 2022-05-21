$input a_position
$input a_normal      // should be normalized before submitting
$input a_texcoord0
$input a_tangent     // should be normalized before submitting

// packed prism vertices
$input a_color0 
$input a_color1 
$input a_color2 
$input a_color3 
$input a_indices
$input a_weight
$input a_texcoord1
$input a_texcoord2
$input a_texcoord3

$output v_frag_pos   // in view space
$output v_frag_norm  // in view space
$output v_texcoord0
$output v_tangent    // in view space
$output v_prism_pos0 
$output v_prism_pos1 
$output v_prism_pos2 
$output v_prism_pos3 
$output v_prism_pos4
$output v_prism_pos5
$output v_prism_uv0
$output v_prism_uv1
$output v_prism_uv2

#include <bgfx_shader.sh>

uniform mat4 u_model_inv_t;

void main() {
    v_frag_pos = vec3(u_modelView * vec4(a_position, 1.0f));
    v_frag_norm = vec3(u_view * u_model_inv_t * vec4(a_normal, 0.0f));
    v_texcoord0 = a_texcoord0;
    v_tangent = vec3(u_modelView * vec4(a_tangent, 0.0f));

    // pass prism to fragment shader, view space
    v_prism_pos0 = vec3(u_modelView * vec4(a_color0, 1.0f));
    v_prism_pos1 = vec3(u_modelView * vec4(a_color1, 1.0f));
    v_prism_pos2 = vec3(u_modelView * vec4(a_color2, 1.0f));
    v_prism_pos3 = vec3(u_modelView * vec4(a_color3, 1.0f));
    v_prism_pos4 = vec3(u_modelView * vec4(a_indices, 1.0f));
    v_prism_pos5 = vec3(u_modelView * vec4(a_weight, 1.0f));
    v_prism_uv0 = a_texcoord1;
    v_prism_uv1 = a_texcoord2;
    v_prism_uv2 = a_texcoord3;


    gl_Position = u_modelViewProj * vec4(a_position, 1.0);
}