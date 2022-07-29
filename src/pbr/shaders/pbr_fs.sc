$input v_frag_pos  // in view space
$input v_frag_norm // in view space
$input v_texcoord0
$input v_tangent   // in view space

#include <bgfx_shader.sh>

#define MAX_LIGHT_COUNT 8

uniform vec4 u_light_count;
uniform vec4 u_light_dirs[MAX_LIGHT_COUNT];  // in view space
uniform vec4 u_light_colors[MAX_LIGHT_COUNT];
uniform vec4 u_view_pos;

uniform vec4 u_albedo;
uniform vec4 u_metallic_roughness_ao;

float DistributionGGX(vec3 n, vec3 h, float roughness);
float GeometrySchlickGGX(float n_v, float roughness);
float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness);
vec3 fresnelSchlick(float cos, vec3 f0);
vec3 fresnelSchlickRoughness(float cos, vec3 f0, float roughness);

const float PI = 3.14159265359;

void main() {
    vec3 albedo = vec3(u_albedo);
    float roughness = u_metallic_roughness_ao[1];
    float metallic = u_metallic_roughness_ao[0];
    float ao = u_metallic_roughness_ao[2];
    vec3 n = normalize(v_frag_norm);
    vec3 v = normalize(vec3(u_view_pos) - v_frag_pos);
    vec3 r = reflect(-v, n);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    // reflectance equation
    vec3 lo = vec3(0.0);
    int light_num = int(min(u_light_count.x, MAX_LIGHT_COUNT));
    for(int i = 0; i < light_num; ++i) {
        // calculate per-light radiance
        vec3 l = -normalize(vec3(u_light_dirs[i]));
        vec3 h = normalize(v + l);
        vec3 radiance = vec3(u_light_colors[i]);
        
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

    vec3 diffuse      =  albedo;

    vec3 ambient = kd * diffuse * ao;

    // vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + lo;
	
    // TODO: do tone mapping in a separate pass
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));  

    gl_FragColor = vec4(color, 1.0f);
    // gl_FragColor = vec4(n, 1.0f);
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