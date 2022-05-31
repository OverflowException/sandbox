#pragma once

#include <glm/vec3.hpp>

class Geometry {
public:
    constexpr static float epsilon5 = 10E-5f;
    constexpr static float epsilon3 = 10E-3f;
    constexpr static float epsilon2 = 10E-2f;

    struct LineSeg {
        glm::vec3 p0;
        glm::vec3 p1;
    };

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };

    static float distance(const LineSeg& seg0, const LineSeg& seg1);

    static float distance_parallel(const LineSeg& seg0, const LineSeg& seg1);

    static float distance(const glm::vec3& p, const LineSeg& seg);

    static glm::vec2 barycentric(const glm::vec3& p, LineSeg seg);

    static AABB aabb(const LineSeg& seg, float r);

    static bool intersect(const AABB& a, const AABB& b);
};