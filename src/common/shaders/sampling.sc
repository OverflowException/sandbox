float van_der_corput(int n, int base) {
    float inv_base = 1.0 / float(base);
    float denom   = 1.0;
    float result  = 0.0;

    for(int i = 0; i < 32; ++i) {
        if(n > 0) {
            denom   = mod(float(n), 2.0);
            result += denom * inv_base;
            inv_base = inv_base / 2.0;
            n       = int(float(n) / 2.0);
        }
    }

    return result;
}

vec2 hammersley(int i, int n) {
    return vec2(float(i)/float(n), van_der_corput(i, 2));
}

vec3 importance_sample_ggx(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
	
	float theta = 2.0 * M_PI * xi.x;
	float cos_phi = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	float sin_phi = sqrt(1.0 - cos_phi * cos_phi);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 h;
	h.x = cos(theta) * sin_phi;
	h.y = sin(theta) * sin_phi;
	h.z = cos_phi;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, n));
	vec3 bitangent = cross(n, tangent);
	
	vec3 sample_vec = tangent * h.x + bitangent * h.y + n * h.z;
	return normalize(sample_vec);
}