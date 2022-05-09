$input v_frag_pos // in world space

#include <bgfx_shader.sh>

SAMPLERCUBE(s_env, 0);

void main() {
    // compute tangent space bases, in world space
    vec3 z = normalize(v_frag_pos);
    vec3 y = vec3(0.0f, 1.0f, 0.0f);
    // we are assuming z does not alignes with (0, 1, 0), which is a reasonable assumption when irradiance map's resolution is an even number
    vec3 x = normalize(cross(y, z));
    y = cross(z, x);

    float step = 0.025f;
    int total_steps = 0;
    vec3 irradiance = vec3(0.0f);
    for (float phi = 0.0f; phi < 0.5 * M_PI; phi += step) {
        for (float theta = 0.0f; theta < 2.0 * M_PI; theta += step) {
            // tangent space coordinate
            vec3 t_coord = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
            // sample direction, in world space
            vec3 cube_dir = t_coord.x * x + t_coord.y * y + t_coord.z * z;
            irradiance += textureCube(s_env, cube_dir).xyz * cos(phi) * sin(phi);

            ++total_steps;
        }
    }
    irradiance = M_PI * irradiance / float(total_steps);
    gl_FragColor = vec4(irradiance, 1.0f);
    // gl_FragColor = vec4(textureCube(s_env, z).xyz, 1.0f);
}