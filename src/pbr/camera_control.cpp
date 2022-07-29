#include <iostream>
#include "camera_control.h"
#include "math_utils.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

const float translate_sensitivity = 3.0f;
const float rotation_sensitivity = 2.0f;
const float zoom_sensitivity = 0.5f;
const glm::vec3 vertical_up(0.0f, 1.0f, 0.0f);

CameraController::CameraController(rdr::Renderer::Camera camera) {
    _camera = std::move(camera);
}

void CameraController::cursor_move(glm::vec2 pos_wnd) {
    MathUtil::Camera camera = *reinterpret_cast<MathUtil::Camera*>(&_camera);

    glm::vec2 cursor_ndc = MathUtil::wnd2ndc(pos_wnd, camera);
    glm::vec2 move_ndc = (cursor_ndc - _pivot_cursor_ndc);
    glm::vec3 pivot_up = _pivot_camera.up;
    glm::vec3 pivot_front = _pivot_camera.front;
    glm::vec3 pivot_right = glm::normalize(glm::cross(pivot_front, pivot_up));
    if (_camera_act == CameraAction::Translate) {
        glm::vec3 move_world = pivot_right * move_ndc.x +
                               pivot_up * move_ndc.y;
        _camera.eye = _pivot_camera.eye - move_world * translate_sensitivity;
    }

    if (_camera_act == CameraAction::Rotate) {
        glm::quat yaw = glm::angleAxis(move_ndc.x * rotation_sensitivity, vertical_up);
        glm::vec3 camera_right = glm::rotate(yaw, pivot_right);
        // sanitize camera_right, strictly on horizontal plane
        camera_right -= vertical_up * glm::dot(camera_right, vertical_up);
        camera_right = glm::normalize(camera_right);

        glm::quat pitch = glm::conjugate(glm::angleAxis(move_ndc.y * rotation_sensitivity, camera_right));

        _camera.up = glm::normalize(glm::rotate(pitch, glm::rotate(yaw, pivot_up)));
        _camera.front = glm::normalize(glm::rotate(pitch, glm::rotate(yaw, pivot_front)));
    }
}

void CameraController::mouse_action(MouseButton::Enum btn,
                                    MouseAction::Enum act,
                                    glm::vec2 pos_wnd) {
    MathUtil::Camera camera = *reinterpret_cast<MathUtil::Camera*>(&_camera);
    if (act == MouseAction::Release) {
        _camera_act = CameraAction::None;

    } else if (act == MouseAction::Press) {
        // save current camera pose and click position
        _pivot_cursor_ndc = MathUtil::wnd2ndc(pos_wnd, camera);
        _pivot_camera = _camera;

        if (btn == MouseButton::Middle) {
            _camera_act = CameraAction::Translate;
        } else if (btn == MouseButton::Left) {
            // TODO:
        } else if (btn == MouseButton::Right) {
            _camera_act = CameraAction::Rotate;
        } else {}

    } else if (act == MouseAction::ScrollUp || act == MouseAction::ScrollDown) {
        glm::vec3 zoom_direction = glm::normalize(MathUtil::ndc2world(MathUtil::wnd2ndc(pos_wnd, camera), -1.0f, camera) - _camera.eye);
        _camera.eye += zoom_direction * 
                       (act == MouseAction::ScrollUp ? zoom_sensitivity : -zoom_sensitivity);
    }
}
