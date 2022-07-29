#pragma once

#include <queue>
#include <glm/vec3.hpp>
// #include "common/application.hpp"
#include "common/renderer.hpp"

class CameraController {
public:
    struct MouseButton {
        enum Enum {
            Left = 0,
            Right,
            Middle,
            None
        };
    };

    struct MouseAction {
        enum Enum {
            Release = 0,
            Press,
            ScrollUp,
            ScrollDown,
            None
        };
    };

    CameraController(rdr::Renderer::Camera camera);

    const rdr::Renderer::Camera& camera() const { return _camera; };
    rdr::Renderer::Camera& camera() { return _camera; };

    void reset_camera(rdr::Renderer::Camera camera);

    // input window coordinates
    void cursor_move(glm::vec2 pos_wnd);

    // input window coordinates
    void mouse_action(MouseButton::Enum btn,
                      MouseAction::Enum act,
                      glm::vec2 pos_wnd);

private:
    enum CameraAction {
        Translate,
        Rotate,
        None
    };

    rdr::Renderer::Camera                   _camera;
    CameraAction                            _camera_act = CameraAction::None;
    glm::vec2                               _pivot_cursor_ndc  = glm::vec2(0.0f);
    rdr::Renderer::Camera                   _pivot_camera;
};