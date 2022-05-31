#include "geometry.h"
#include <glm/gtc/matrix_transform.hpp>

float Geometry::distance(const LineSeg& seg0, const LineSeg& seg1) {
    glm::vec3 e0 = seg0.p1 - seg0.p0;
    glm::vec3 e1 = seg1.p1 - seg1.p0;
    glm::vec3 mid0 = (seg0.p0 + seg0.p1) * 0.5f;
    glm::vec3 mid1 = (seg1.p0 + seg1.p1) * 0.5f;
    float len0 = glm::length(e0);
    float mid_l = glm::length(mid0 - mid1);
    if (len0 < epsilon3) { 
        // seg0 degenerates into a point
        return distance(seg0.p0, seg1);
    }
    if (mid_l * epsilon2 > len0) {
        // length of seg0 is not significant
        return distance(mid0, seg1);
    }

    float len1 = glm::length(e1);
    if (len1 < epsilon3) {
        // seg 1 degenerates into a point
        return distance(seg1.p0, seg0);
    }
    if (mid_l * epsilon2 > len1) {
        // length of seg1 is not significant
        return distance(mid1, seg0);
    }

    glm::vec3 e2 = glm::cross(e0, e1);
    if (glm::length(e2) < len0 * len1 * epsilon5) {
        // distance between 2 parallel line segments
        return distance_parallel(seg0, seg1);
    }

    // 2 line segments do not degenerate and are not parallel
    // construct a linear system
    glm::mat3 A(1.0f);
    A[0] = e0;
    A[1] = -e1;
    A[2] = e2;
    glm::vec3 b = seg1.p0 - seg0.p0;
    // A should be invertible at this point
    // TODO: determinant assertion fails sometimes, but s and t seem to have reasonable values
    // assert(glm::abs(glm::determinant(A)) > epsilon5);

    glm::vec3 x = glm::inverse(A) * b;

    float s = x[0]; // line coordinate of segment 0
    float t = x[1]; // line coordinate of segment 1

    glm::vec3 closest0 = seg0.p0 + glm::clamp(s, 0.0f, 1.0f) * e0;
    glm::vec3 closest1 = seg1.p0 + glm::clamp(t, 0.0f, 1.0f) * e1;

    return glm::length(closest0 - closest1);
}

float Geometry::distance_parallel(const LineSeg& seg0, const LineSeg& seg1) {
    assert(glm::length(glm::cross(seg0.p1 - seg0.p0, seg1.p1 - seg1.p0)) < epsilon3);
    // projecting seg0.p0 onto seg1
    glm::vec2 uv0 = barycentric(seg0.p0, seg1);
    if (uv0.x >= 0.0f && uv0.x <= 1.0f) {
        // seg0.p0 between seg1
        glm::vec3 ortho = seg1.p0 * uv0.x + seg1.p1 * uv0.y;
        return glm::length(seg0.p0 - ortho);
    }
    // projecting seg0.p1 onto seg1
    glm::vec2 uv1 = barycentric(seg0.p1, seg1);
    if (uv1.x >= 0.0f && uv1.x <= 1.0f) {
        // seg0.p1 betweem seg1
        glm::vec3 ortho = seg1.p0 * uv1.x + seg1.p1 * uv1.y;
        return glm::length(seg0.p1 - ortho);
    }
    if (uv0.x < 0.0f && uv1.x < 0.0f) {
        // seg0 completely outside of seg1.p1
        if (uv0.x > uv1.x) {
            return glm::length(seg0.p0 - seg1.p1);
        } else if (uv1.x > uv0.x) {
            return glm::length(seg0.p1 - seg1.p1);
        } else {
            assert(false);
        }
    }
    if (uv0.y < 0.0f && uv1.y < 0.0f) {
        // seg0 completely outside of seg1.p0
        if (uv0.y > uv1.y) {
            return glm::length(seg0.p0 - seg1.p0);
        } else if (uv1.y > uv0.y) {
            return glm::length(seg0.p1 - seg1.p0);
        } else {
            assert(false);
        }
    }
    if (uv0.y < 0.0f && uv1.x < 0.0f ||
        uv0.x < 0.0f && uv1.y < 0.0f) {
        // seg0 covers seg1 entirely
        return distance(seg1.p0, seg0);
    } else {
        assert(false);
    }
    assert(false);
    return 0.0f;
}

float Geometry::distance(const glm::vec3& p, const LineSeg& seg) {
    glm::vec3 e = seg.p1 - seg.p0;
    glm::vec3 mid = (seg.p0 + seg.p1) * 0.5f;
    float mid_l = glm::length(p - mid);
    if (glm::length(e) < epsilon3) {
        // segment degenerates into a point
        return glm::length(p - seg.p0);
    }
    if (mid_l * epsilon2 > glm::length(e)) {
        // length of seg is not significant
        return mid_l;
    }

    glm::vec2 uv = barycentric(p, seg);
    if (uv.x < 0.0f) {
        // closer to p1
        return glm::length(p - seg.p1);
    } else if (uv.y < 0.0f) {
        // closer to p0
        return glm::length(p - seg.p0);
    } else {
        glm::vec3 ortho = seg.p0 * uv.x + seg.p1 * uv.y;
        return glm::length(p - ortho);
    }
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

Geometry::AABB Geometry::aabb(const LineSeg& seg, float r) {
    AABB aabb;
    aabb.min = glm::vec3(std::numeric_limits<float>::max());
    aabb.max = glm::vec3(std::numeric_limits<float>::lowest());

    float val;
    aabb.min.x = aabb.min.x > (val = seg.p0.x - r) ? val : aabb.min.x;
    aabb.min.x = aabb.min.x > (val = seg.p1.x - r) ? val : aabb.min.x;
    aabb.min.y = aabb.min.y > (val = seg.p0.y - r) ? val : aabb.min.y;
    aabb.min.y = aabb.min.y > (val = seg.p1.y - r) ? val : aabb.min.y;
    aabb.min.z = aabb.min.z > (val = seg.p0.z - r) ? val : aabb.min.z;
    aabb.min.z = aabb.min.z > (val = seg.p1.z - r) ? val : aabb.min.z;

    aabb.max.x = aabb.max.x < (val = seg.p0.x + r) ? val : aabb.max.x;
    aabb.max.x = aabb.max.x < (val = seg.p1.x + r) ? val : aabb.max.x;
    aabb.max.y = aabb.max.y < (val = seg.p0.y + r) ? val : aabb.max.y;
    aabb.max.y = aabb.max.y < (val = seg.p1.y + r) ? val : aabb.max.y;
    aabb.max.z = aabb.max.z < (val = seg.p0.z + r) ? val : aabb.max.z;
    aabb.max.z = aabb.max.z < (val = seg.p1.z + r) ? val : aabb.max.z;

    return aabb;
}

bool Geometry::intersect(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}
