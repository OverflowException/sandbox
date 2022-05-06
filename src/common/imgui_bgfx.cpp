#include "bx/bx.h"

#include "imgui_bgfx.h"
#include "imgui_bgfx_shaders.h"

bgfx::VertexLayout ImBgfx:: _v_layout;
bgfx::TextureHandle ImBgfx::_font_tex;
bgfx::UniformHandle ImBgfx::_font_uni;
bgfx::ProgramHandle ImBgfx::_program;
GLFWwindow* ImBgfx::_window;
GLFWcursor* ImBgfx::_cursors[ImGuiMouseCursor_COUNT];

void ImBgfx::init(GLFWwindow* window) {
    _window = window;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Setup vertex declaration
	_v_layout
		.begin()
		.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	.end();

	// Create font
	io.Fonts->AddFontDefault();

    unsigned char* data;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
	_font_tex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(data, width * height * 4));
	_font_uni = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	// Create shader program
    assert(bgfx::getRendererType() == bgfx::RendererType::OpenGL);
	bgfx::ShaderHandle vs = bgfx::createShader(bgfx::makeRef(vs_imgui_glsl, vs_imgui_glsl_len));
	bgfx::ShaderHandle fs = bgfx::createShader( bgfx::makeRef(fs_imgui_glsl, fs_imgui_glsl_len));
	_program = bgfx::createProgram(vs, fs, true);

	// Setup back-end capabilities flags
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

	// Key mapping
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_D] = GLFW_KEY_D;
    io.KeyMap[ImGuiKey_S] = GLFW_KEY_S;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_W] = GLFW_KEY_W;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

/* 	io.SetClipboardTextFn = ;
	io.GetClipboardTextFn = ;
	io.ClipboardUserData = _window; */

	_cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	_cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	_cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   // FIXME: GLFW doesn't have this.
	_cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	_cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	_cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
	_cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
	_cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
}

void ImBgfx::reset(uint16_t width, uint16_t height) {
    bgfx::setViewRect(200, 0, 0, width, height);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR, 0x303030ff);
}

void ImBgfx::events(float dt) {
    ImGuiIO& io = ImGui::GetIO();

    int window_w;
    int window_h;
    int frame_w;
    int frame_h;
    glfwGetWindowSize(_window, &window_w, &window_h);
    glfwGetFramebufferSize(_window, &frame_w, &frame_h);

    io.DisplaySize = ImVec2(window_w, window_h);
    io.DisplayFramebufferScale = ImVec2(window_w > 0 ? ((float)frame_w / window_w) : 0,
                                        window_h > 0 ? ((float)frame_h / window_h) : 0);

    io.DeltaTime = dt;

    // Update mouse position
	const ImVec2 mouse_pos_backup = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	const bool focused = glfwGetWindowAttrib(_window, GLFW_FOCUSED) != 0;
	if (focused) {
		if (io.WantSetMousePos) {
			glfwSetCursorPos(_window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
		}
		else {
			double mouse_x, mouse_y;
			glfwGetCursorPos(_window, &mouse_x, &mouse_y );
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
		}
	}

	// Update mouse cursor
	if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) &&
        glfwGetInputMode(_window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
		ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
			// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
			glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else {
			// Show OS mouse cursor
			glfwSetCursor(_window, _cursors[imgui_cursor] ? _cursors[imgui_cursor] : _cursors[ImGuiMouseCursor_Arrow]);
			glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

void ImBgfx::render(ImDrawData* data) {
    for (int ii = 0, num = data->CmdListsCount; ii < num; ++ii) {
		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;

		const ImDrawList* draw_list = data->CmdLists[ii];
		uint32_t v_num = (uint32_t)draw_list->VtxBuffer.size();
		uint32_t i_num  = (uint32_t)draw_list->IdxBuffer.size();

        assert(bgfx::getAvailTransientVertexBuffer(v_num, _v_layout) > 0);
        assert(bgfx::getAvailTransientIndexBuffer(i_num) > 0);

		bgfx::allocTransientVertexBuffer(&tvb, v_num, _v_layout);
		bgfx::allocTransientIndexBuffer(&tib, i_num);

		memcpy(tvb.data, draw_list->VtxBuffer.begin(), v_num * sizeof(ImDrawVert));
		memcpy(tib.data, draw_list->IdxBuffer.begin(), i_num * sizeof(ImDrawIdx));

		uint32_t offset = 0;
        // We could have simply submitted all vertices with one bgfx::submit() call
        // Looping through all commands is to apply individual clipping and user callbacks
		for (const ImDrawCmd* cmd = draw_list->CmdBuffer.begin(); cmd != draw_list->CmdBuffer.end(); ++cmd) {
			if (cmd->UserCallback) {
				cmd->UserCallback(draw_list, cmd);
			}
			else if (0 != cmd->ElemCount) {
				uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
				bgfx::TextureHandle th = _font_tex;
				if (cmd->TextureId) {
					th.idx = uint16_t(uintptr_t(cmd->TextureId));
				}
				state |= BGFX_STATE_BLEND_FUNC( BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA );
				const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f));
				const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f));
 				bgfx::setScissor(xx, yy, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f) - xx), uint16_t(bx::min(cmd->ClipRect.w, 65535.0f) - yy));
				bgfx::setState(state);
				bgfx::setTexture(0, _font_uni, th);
				bgfx::setVertexBuffer(0, &tvb, 0, v_num);
				bgfx::setIndexBuffer(&tib, offset, cmd->ElemCount);
				bgfx::submit(200, _program);
			}

			offset += cmd->ElemCount;
		}
	}
}

void ImBgfx::shutdown() {
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++) {
		glfwDestroyCursor(_cursors[cursor_n]);
		_cursors[cursor_n] = nullptr;
	}

	bgfx::destroy(_font_uni);
	bgfx::destroy(_font_tex);
	bgfx::destroy(_program);
	ImGui::DestroyContext();
}
