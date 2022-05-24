struct TriManifold {
    int intersect; // 0: no 1: yes
    // baricentric coordinate of intersection point
    float bari[3];
    float t; // ray parameter
};

struct Ray {
    vec3 o;
    vec3 r;
};

struct PrismManifold {
    int intersect;
    int indices[3]; // indices of the triangle where intersection occurs
    TriManifold man;
};

TriManifold intersect(Ray ray, vec3 tri[3]) {
    TriManifold man;
    vec3 e1 = tri[1] - tri[0];
    vec3 e2 = tri[2] - tri[0];
    vec3 h = cross(ray.r, e2);
    float a = dot(e1, h);
    if (a < 0.0001f && a > -0.0001f) {
        // ray parallel to triangle
        man.intersect = 0;
        return man;
    }
    float f = 1.0f / a;
    vec3 s = ray.o - tri[0];
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) {
        man.intersect = 0;
        return man;
    }
    vec3 q = cross(s, e1);
    float v = f * dot(ray.r, q);
    if (v < 0.0f || u + v > 1.0f) {
        man.intersect = 0;
        return man;
    }

    // compute intersection
    man.t = f * dot(e2, q);
    if (man.t < 0.0001f) {
        // line intersection, not ray intersction
        man.intersect = 0;
        return man;
    }

    man.intersect = 1;
    man.bari[0] = 1.0f - u - v;
    man.bari[1] = u;
    man.bari[2] = v;

    return man;
}

int facing(Ray ray, vec3 tri[3]) {
    vec3 e1 = tri[1] - tri[0];
    vec3 e2 = tri[2] - tri[0];
    float proj = dot(ray.r, cross(e1, e2));
    if (proj < 0.0f) {
        return 1;
    } else {
        return 0;
    }
}

PrismManifold intersect(Ray ray, vec3 prism[6]) {
    PrismManifold prism_man;
    // TriManifold tri_man;
    // traverse all prism faces
    // Triangle index order:
    // 0, 2, 1,
    // 3, 4, 5,
    // 0, 1, 4,
    // 0, 4, 3,
    // 0, 3, 5,
    // 0, 5, 2,
    // 1, 2, 5,
    // 1, 5, 4
    int triangle_indices[24];
    triangle_indices[0] = 0;    
    triangle_indices[1] = 2;
    triangle_indices[2] = 1;

    triangle_indices[3] = 3;
    triangle_indices[4] = 4;
    triangle_indices[5] = 5;

    triangle_indices[6] = 0;
    triangle_indices[7] = 1;
    triangle_indices[8] = 4;

    triangle_indices[9] = 0;
    triangle_indices[10] = 4;
    triangle_indices[11] = 3;

    triangle_indices[12] = 0;
    triangle_indices[13] = 3;
    triangle_indices[14] = 5;

    triangle_indices[15] = 0;
    triangle_indices[16] = 5;
    triangle_indices[17] = 2;

    triangle_indices[18] = 1;
    triangle_indices[19] = 2;
    triangle_indices[20] = 5;

    triangle_indices[21] = 1;
    triangle_indices[22] = 5;
    triangle_indices[23] = 4;

    for (int i = 0; i < 24; i += 3) {
        vec3 tri[3];
        int i0 = triangle_indices[i];
        int i1 = triangle_indices[i + 1];
        int i2 = triangle_indices[i + 2];
        tri[0] = prism[i0];
        tri[1] = prism[i1];
        tri[2] = prism[i2];
        if (facing(ray, tri) == 0) {
            TriManifold tri_man = intersect(ray, tri);
            if (tri_man.intersect == 1) {
                // DEBUG: index not correct
                prism_man.intersect = 1;
                prism_man.indices[0] = triangle_indices[i];
                prism_man.indices[1] = triangle_indices[i + 1];
                prism_man.indices[2] = triangle_indices[i + 2];
                prism_man.man = tri_man;
                return prism_man;
            }
        }
    }

    prism_man.intersect = 0;
    return prism_man;
}
