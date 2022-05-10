$input v_frag_pos  // in view space
$input v_frag_norm // in view space
$input v_texcoord0
$input v_tangent   // in view space

#include <bgfx_shader.sh>

uniform vec4 u_light_pos[4];  // in view space
uniform vec4 u_light_colors[4];

uniform vec4 u_albedo;
uniform vec4 u_metallic_roughness_ao;

uniform mat4 u_view_inv;

SAMPLER2D(s_albedo, 0);
SAMPLER2D(s_roughness, 1);
SAMPLER2D(s_metallic, 2);
SAMPLER2D(s_normal, 3);
SAMPLER2D(s_ao, 4);
SAMPLERCUBE(s_skybox_irr, 5);
SAMPLERCUBE(s_skybox_prefilter, 6);
SAMPLER2D(s_brdf_lut, 7);

float DistributionGGX(vec3 n, vec3 h, float roughness);
float GeometrySchlickGGX(float n_v, float roughness);
float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness);
vec3 fresnelSchlick(float cos, vec3 f0);
vec3 fresnelSchlickRoughness(float cos, vec3 f0, float roughness);
vec3 compute_normal();

vec3 compute_normal(vec3 norm, vec3 tangent, vec2 coord) {
    vec3 bitangent = cross(norm, tangent);

    // tangent, bitangent, normal coordinates
    vec3 tbn = vec3(texture2D(s_normal, coord)) * 2.0 - 1.0;
    return normalize(tangent * tbn.x + bitangent * tbn.y + norm * tbn.z);
}

// for vectors
vec3 view2world(vec3 v){
    return vec3(u_view_inv * vec4(v, 0.0f));
}

const float PI = 3.14159265359;

void main() {
    vec3 albedo = pow(vec3(texture2D(s_albedo, v_texcoord0)), vec3(2.2f));
    float roughness = texture2D(s_roughness, v_texcoord0).r;
    float metallic = texture2D(s_metallic, v_texcoord0).r;
    float ao = texture2D(s_ao, v_texcoord0).r;
    vec3 n = compute_normal(v_frag_norm, v_tangent, v_texcoord0);
    vec3 v = normalize(vec3(0.0f) - v_frag_pos);
    vec3 r = reflect(-v, n);

    // vec3 albedo = vec3(u_albedo);
    // float roughness = u_metallic_roughness_ao[1];
    // float metallic = u_metallic_roughness_ao[0];
    // float ao = u_metallic_roughness_ao[2];
    // vec3 n = normalize(v_frag_norm);
    // vec3 v = normalize(vec3(0.0f) - v_frag_pos);
    // vec3 r = reflect(-v, n);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    // reflectance equation
    vec3 lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 l = normalize(vec3(u_light_pos[i]) - v_frag_pos);
        vec3 h = normalize(v + l);
        float distance    = length(vec3(u_light_pos[i]) - v_frag_pos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = vec3(u_light_colors[i]) * attenuation;
        
        // cook-torrance brdf
        float ndf = DistributionGGX(n, h, roughness);
        float g   = GeometrySmith(n, v, l, roughness);      
        vec3 f    = fresnelSchlick(max(dot(h, v), 0.0), f0);
        
        vec3 ks = f;
        vec3 kd = vec3(1.0) - ks;
        kd *= 1.0 - metallic;
        
        vec3 numerator    = ndf * g * f;
        float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float n_l = max(dot(n, l), 0.0);
        lo += (kd * albedo / PI + specular) * radiance * n_l; 
    }

    // ambient lighting (we now use IBL as the ambient term)
    vec3 f = fresnelSchlickRoughness(max(dot(n, v), 0.0), f0, roughness);
    
    vec3 ks = f;
    vec3 kd = 1.0 - ks;
    kd *= 1.0 - metallic;	  
    
    vec3 irradiance = textureCube(s_skybox_irr, view2world(n)).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefiltered = textureCubeLod(s_skybox_prefilter, view2world(r),  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture2D(s_brdf_lut, vec2(max(dot(n, v), 0.0), roughness)).rg;
    // TODO: brdf doesnt work well. producing a little circle in the middle. I really should generate it myself
    vec3 specular = prefiltered * (f * brdf.x + brdf.y);

    vec3 ambient = (kd * diffuse + specular) * ao;

    // vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + lo;
	
    // TODO: do tone mapping in a separate pass
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));  

    gl_FragColor = vec4(color, 1.0f);
}

vec3 fresnelSchlick(float cos, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cos, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0);
}   

float DistributionGGX(vec3 n, vec3 h, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float n_h  = max(dot(n, h), 0.0);
    float n_h2 = n_h * n_h;
	
    float num   = a2;
    float denom = (n_h2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float n_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = n_v;
    float denom = n_v * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float n_v = max(dot(n, v), 0.0);
    float n_l = max(dot(n, l), 0.0);
    float ggx2  = GeometrySchlickGGX(n_v, roughness);
    float ggx1  = GeometrySchlickGGX(n_l, roughness);
	
    return ggx1 * ggx2;
}