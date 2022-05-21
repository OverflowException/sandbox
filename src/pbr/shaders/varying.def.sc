vec3 v_frag_pos  : POSITION = vec3(0.0f, 0.0f, 0.0f);
vec3 v_frag_norm : NORMAL;
vec3 v_texcoord0 : TEXCOORD0;
vec3 v_tangent   : TANGENT;

vec3 v_prism_pos0 : COLOR0;
vec3 v_prism_pos1 : COLOR1;
vec3 v_prism_pos2 : COLOR2;
vec3 v_prism_pos3 : COLOR3;
vec3 v_prism_pos4 : INDICES;
vec3 v_prism_pos5 : WEIGHT;
vec2 v_prism_uv0 : TEXCOORD1;
vec2 v_prism_uv1 : TEXCOORD2;
vec2 v_prism_uv2 : TEXCOORD3;




vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec3 a_texcoord0 : TEXCOORD0;
vec3 a_tangent   : TANGENT;

vec3 a_color0 : COLOR0;
vec3 a_color1 : COLOR1;
vec3 a_color2 : COLOR2;
vec3 a_color3 : COLOR3;
vec3 a_indices : INDICES;
vec3 a_weight : WEIGHT;
vec2 a_texcoord1 : TEXCOORD1;
vec2 a_texcoord2 : TEXCOORD2;
vec2 a_texcoord3 : TEXCOORD3;