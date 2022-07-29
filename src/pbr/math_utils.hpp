#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

class MathUtil {
public:
    struct Camera {
        glm::vec3 eye;
        glm::vec3 front;
        glm::vec3 up;
        
        float width;
        float height;
        float fovy;
        float near;
        float far;
    };

    // depth -- depth in eye space, conventionally a negative value
    static glm::vec3 ndc2world(glm::vec2 ndc, float depth, const Camera& camera) {
        float w_clip = -depth;
        float x_clip = ndc.x * w_clip;
        float y_clip = ndc.y * w_clip;

        float f = camera.far;
        float n = camera.near;
        float inv = 1.0 / (f - n);
        float z_clip = -(f + n) * inv * depth - 2.0f * f * n * inv;

        glm::mat4 proj = glm::perspective(camera.fovy,
                                          camera.width / camera.height,
                                          camera.near,
                                          camera.far);
        glm::mat4 view = glm::lookAt(camera.eye,
                                     camera.eye + camera.front,
                                     camera.up);

        return glm::vec3(glm::inverse(proj * view) * glm::vec4(x_clip, y_clip, z_clip, w_clip));
    }

    static glm::vec2 wnd2ndc(glm::vec2 wnd, const Camera& camera) {
        return glm::vec2(wnd.x / camera.width * 2.0f - 1.0f,
                         (1.0f - wnd.y / camera.height) * 2.0f - 1.0f);
    }

    static glm::quat shortest_rotation(const Camera& start, const Camera& end) {
        glm::mat3 start_frame(start.front,
                              start.up,
                              glm::cross(start.front, start.up));

        glm::mat3 end_frame(end.front,
                            end.up,
                            glm::cross(end.front, end.up));

        return glm::toQuat(end_frame) * glm::conjugate(glm::toQuat(start_frame));
    }
};
