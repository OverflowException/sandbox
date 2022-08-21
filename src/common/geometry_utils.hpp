#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

class GeometryUtil {
public:
    struct Frustum {
        // x --- half width
        // y --- half height
        // z --- depth
        glm::vec3 near  = glm::vec3(0.0f);
        glm::vec3 far   = glm::vec3(0.0f);
        glm::mat4 trans = glm::mat4(1.0f);

        std::vector<glm::vec3> corners() {
            glm::vec3 n = near;
            glm::vec3 f = far;
            return std::vector<glm::vec3> {
                glm::vec3(trans * glm::vec4(-n.x, -n.y, n.z, 1.0f)),
                glm::vec3(trans * glm::vec4(-n.x,  n.y, n.z, 1.0f)),
                glm::vec3(trans * glm::vec4( n.x, -n.y, n.z, 1.0f)),
                glm::vec3(trans * glm::vec4( n.x,  n.y, n.z, 1.0f)),
                glm::vec3(trans * glm::vec4(-f.x, -f.y, f.z, 1.0f)),
                glm::vec3(trans * glm::vec4(-f.x,  f.y, f.z, 1.0f)),
                glm::vec3(trans * glm::vec4( f.x, -f.y, f.z, 1.0f)),
                glm::vec3(trans * glm::vec4( f.x,  f.y, f.z, 1.0f)),
            };
        }
    };

    struct Sphere {
        glm::vec3 center;
        float     radius;
    };

    struct AABB {
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        void add(glm::vec3 p) {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        std::vector<glm::vec3> corners() const {
            return std::vector<glm::vec3> {
                glm::vec3(min.x, min.y, min.z),
                glm::vec3(max.x, min.y, min.z),
                glm::vec3(min.x, max.y, min.z),
                glm::vec3(max.x, max.y, min.z),
                glm::vec3(min.x, min.y, max.z),
                glm::vec3(max.x, min.y, max.z),
                glm::vec3(min.x, max.y, max.z),
                glm::vec3(max.x, max.y, max.z),
            };
        }

        AABB transformed_bound(const glm::mat4& transform) const {
            AABB result;
            std::vector<glm::vec3> cs = std::move(corners());
            for (glm::vec3& c : cs) {
                result.add(glm::vec3(transform * glm::vec4(c, 1.0f)));
            }
            return result;
        }

        void merge(const AABB& aabb) {
            min = glm::min(min, aabb.min);
            max = glm::max(max, aabb.max);
        }

        Sphere bounding_sphere() {
            Sphere s;
            s.center = (min + max) * glm::vec3(0.5f);
            s.radius = glm::distance(s.center, min);
            return s;
        }
    };
};