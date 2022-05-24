#include "intersect.sc"

vec2 parallax_mapping(vec3 p,
                      vec3 v,
                      vec3 tangent,
                      vec3 norm,
                      vec3 coord,
                      vec3 prism_pos[6],
                      vec2 prism_uv[6]) {
    vec2 coord_end;
    Ray ray;
    ray.o = p;
    ray.r = v;
    PrismManifold prism_man = intersect(ray, prism_pos);
    if(prism_man.intersect == 1) {
        // intersection with prism
        coord_end = vec2(prism_man.man.bari[0]) * prism_uv[prism_man.indices[0]] +
                    vec2(prism_man.man.bari[1]) * prism_uv[prism_man.indices[1]] +
                    vec2(prism_man.man.bari[2]) * prism_uv[prism_man.indices[2]];
        // DEBUG:
        // showing ray depth
        // return vec2(prism_man.man.t * -v.z);
        // showing UV map
        // return coord_end;
    } else {
        // no intersection with prism, which is an abnormality
        discard;
    }

    vec3 bitangent = cross(norm, tangent);
    mat3 tbn = mat3(tangent, bitangent, norm);
    // view vector's coordinate in tangent space
    vec3 v_tbn = transpose(tbn) * v;

    // dynamic linear search step number, to eliminate stepping artifacts at grazing angle
    const float max_step = 40.0f;
    const float min_step = 10.0f;

    float r = clamp(abs(dot(norm, v)), 0.0f, 1.0f);
    int step_count = int(min_step + (1.0f - r) * (max_step - min_step));
    float inv_step_count = 1.0f / float(step_count);

    // linear search
    float v_cur = coord.z;
    float v_step = v_tbn.z * prism_man.man.t * inv_step_count;
    vec2 coord_cur = coord.xy;
    vec2 coord_step = (coord_end - coord_cur) * vec2(inv_step_count);
    for (int i = 0; i < step_count; ++i) {
        float v_next = v_cur + v_step;
        vec2 coord_next = coord_cur + coord_step;
        // TODO: why height ratio?
        float height_ratio = 0.3f;
        float h_cur = texture2D(s_height, coord_cur).r * height_ratio;
        float h_next = texture2D(s_height, coord_next).r * height_ratio;
        if (h_cur < v_cur &&
            h_next > v_next) {
            float ratio = (v_cur - h_cur) / (h_next - v_next);
            ratio = ratio / (1.0f + ratio);
            return coord_next * vec2(ratio) + coord_cur * vec2(1.0 - ratio);
        } else if (h_cur == v_cur) {
            return coord_cur;
        } else if (h_next == v_next) {
            return coord_next;
        }

        v_cur += v_step;
        coord_cur += coord_step;
    }

    // no intersection between ray and height map
    discard;
}
