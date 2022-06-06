#include "geometry.h"
#include <glm/gtc/matrix_transform.hpp>

float Geometry::distance(const LineSeg& seg0, const LineSeg& seg1) {
    // assuming these line segments are not parallel
    glm::vec2 line_coords = closest(seg0, seg1);
    float s = line_coords.x;
    float t = line_coords.y;
    glm::vec3 e0 = seg0.p1 - seg0.p0;
    glm::vec3 e1 = seg1.p1 - seg1.p0;

    glm::vec3 closest0 = seg0.p0 + s * e0;
    glm::vec3 closest1 = seg1.p0 + t * e1;

    return glm::length(closest0 - closest1);
}

glm::vec2 Geometry::closest(const LineSeg& seg0, const LineSeg& seg1) {
    glm::vec3 e0 = seg0.p1 - seg0.p0;
    glm::vec3 e1 = seg1.p1 - seg1.p0;
    glm::vec3 e2 = glm::cross(e0, e1);

    // construct a linear system
    glm::mat3 A(1.0f);
    A[0] = e0;
    A[1] = -e1;
    A[2] = e2;
    glm::vec3 b = seg1.p0 - seg0.p0;

    glm::vec3 x = glm::inverse(A) * b;

    float s = glm::clamp(x[0], 0.0f, 1.0f); // line coordinate of segment 0
    float t = glm::clamp(x[1], 0.0f, 1.0f); // line coordinate of segment 1

    return glm::vec2(s, t);
}

glm::vec3 Geometry::closest(const glm::vec3& p, const Geometry::Triangle& tri) {
    // TODO: assuming tri is guaranteed to be a triangle
    // line segment barycentrics: seg01_uv seg12_uv seg20_uv
    // triangle barycentrics: tri_uvw
    const glm::vec3& p0 = tri.p0;
    const glm::vec3& p1 = tri.p1;
    const glm::vec3& p2 = tri.p2;

    // test vertex regions
    glm::vec2 seg01_uv = barycentric(p, LineSeg{p0, p1});
    glm::vec2 seg20_uv = barycentric(p, LineSeg{p2, p0});
    if (seg01_uv.y <= 0.0f && seg20_uv.x <= 0.0f) {
        // vertex 0
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec2 seg12_uv = barycentric(p, LineSeg{p1, p2});
    if (seg12_uv.y <= 0.0f && seg01_uv.x <= 0.0f) {
        // vertex 1
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (seg20_uv.y <= 0.0f && seg12_uv.x <= 0.0f) {
        // vertex 2
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // test interior region
    glm::vec3 tri_uvw = barycentric(p, tri);
    if (tri_uvw.x >= 0.0f && tri_uvw.y >= 0.0f && tri_uvw.z >= 0.0f) {
        return tri_uvw;
    }

    // test edge regions
    if (seg01_uv.x >= 0.0f && seg01_uv.y >= 0.0f && tri_uvw.z <= 0.0f) {
        // edge 01 region
        return glm::vec3(seg01_uv, 0.0f);
    }

    if (seg12_uv.x >= 0.0f && seg12_uv.y >= 0.0f && tri_uvw.x <= 0.0f) {
        // edge 12 region
        return glm::vec3(0.0f, seg12_uv);
    }

    if (seg20_uv.x >= 0.0f && seg20_uv.y >= 0.0f && tri_uvw.y <= 0.0f) {
        // edge 20 region
        return glm::vec3(seg20_uv.y, 0.0f, seg20_uv.x);
    }

    assert(false);
    return glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec2 Geometry::barycentric(const glm::vec3& p, LineSeg seg) {
    glm::vec3 e = seg.p1 - seg.p0;
    float len = glm::length(e);
    assert(len > epsilon3);
    float inv_len2 = 1.0f / (len * len);

    glm::vec3 e0 = p - seg.p0;

    glm::vec2 uv(0.0f);
    uv.y = glm::dot(e0, e) * inv_len2;
    uv.x = 1.0f - uv.y;

    return uv;
}

glm::vec3 Geometry::barycentric(const glm::vec3& p, Triangle tri) {
    glm::vec3 e01 = tri.p1 - tri.p0;
    glm::vec3 e02 = tri.p2 - tri.p0;

    // pseudo inverse
    glm::mat2x3 B(e01, e02);
    glm::mat3x2 BT = glm::transpose(B);
    glm::mat2 BTB = BT * B;
    // to verify tri is indeed a triangle
    assert(abs(glm::determinant(BTB)) > epsilon7);
    glm::mat3x2 B_pseudo_inv = glm::inverse(BTB) * BT;
    glm::vec2 x_b = B_pseudo_inv * (p - tri.p0);
    return glm::vec3(1.0f - x_b.x - x_b.y, x_b.x, x_b.y);
}

Geometry::AABB Geometry::aabb(const glm::vec3& p, float r) {
    return AABB{{p - glm::vec3(r)}, {p + glm::vec3(r)}};
}

Geometry::AABB Geometry::aabb(const LineSeg& seg, float r) {
    return merge(aabb(seg.p0, r), aabb(seg.p1, r));
}

Geometry::AABB Geometry::aabb(const Triangle& tri, float r) {
    return merge(merge(aabb(tri.p0, r), aabb(tri.p1, r)), aabb(tri.p2, r));
}

Geometry::AABB Geometry::merge(const AABB& a, const AABB& b) {
    return AABB{glm::min(a.min, b.min), glm::max(a.max, b.max)};
}

bool Geometry::intersect(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}
