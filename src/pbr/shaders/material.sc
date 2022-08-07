const float PI = 3.14159265359;

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

vec3 reflectance(vec3 albedo, float metallic, float roughness, float ao,
                 vec3 light_color, float shadow,
                 vec3 l, vec3 n, vec3 v) {
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    vec3 h = normalize(v + l);
    // take shadowing factor into account
    vec3 radiance = light_color * vec3(shadow);
    
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
    return (kd * albedo / PI + specular) * radiance * n_l;
}