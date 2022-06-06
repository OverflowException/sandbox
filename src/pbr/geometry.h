#pragma once

#include <glm/vec3.hpp>

class Geometry {
public:
    constexpr static float epsilon7 = 10E-7f;
    constexpr static float epsilon5 = 10E-5f;
    constexpr static float epsilon3 = 10E-3f;
    constexpr static float epsilon2 = 10E-2f;

    struct LineSeg {
        glm::vec3 p0;
        glm::vec3 p1;
    };

    struct Triangle {
        // p0 -> p1 -> p2 as the positive direction
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
    };

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };

    static float distance(const LineSeg& seg0, const LineSeg& seg1);

    // returns barycentric coordinates
    static glm::vec3 closest(const glm::vec3& p, const Triangle& tri);

    // return line coordinate, not barycentric
    static glm::vec2 closest(const LineSeg& seg0, const LineSeg& seg1);

    static glm::vec2 barycentric(const glm::vec3& p, LineSeg seg);

    static glm::vec3 barycentric(const glm::vec3& p, Triangle tri);

    static AABB aabb(const glm::vec3& p, float r);

    static AABB aabb(const LineSeg& seg, float r);

    static AABB aabb(const Triangle& tri, float r);

    static bool intersect(const AABB& a, const AABB& b);

    static AABB merge(const AABB& a, const AABB& b);
};