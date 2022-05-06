#pragma once

#include <GLFW/glfw3.h>
#include "bgfx/bgfx.h"
#include "imgui.h"

class ImBgfx {
public:
    static void init(GLFWwindow* window);

    static void reset(uint16_t width, uint16_t height);

    static void events(float dt);

    static void render(ImDrawData* data);

    static void shutdown();

private:
    static bgfx::VertexLayout  _v_layout;
    static bgfx::TextureHandle _font_tex;
    static bgfx::UniformHandle _font_uni;
    static bgfx::ProgramHandle _program;

    static GLFWwindow* _window;
    static GLFWcursor* _cursors[ImGuiMouseCursor_COUNT];

};
