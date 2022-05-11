#include <vector>

#include "common/application.hpp"
#include "common/file_io.h"

class ScreenQuadApp : public app::Application {
    std::vector<float> vb = {
        -1.0f,  1.0f, 0.0f,
    	-1.0f, -1.0f, 0.0f,
    	 1.0f, -1.0f, 0.0f,
    	 1.0f, -1.0f, 0.0f,
    	 1.0f,  1.0f, 0.0f,
    	-1.0f,  1.0f, 0.0f,
    };

    bgfx::VertexBufferHandle vb_hdl = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle prog = BGFX_INVALID_HANDLE;

    void initialize(int argc, char** argv) {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .end();
        vb_hdl = bgfx::createVertexBuffer(bgfx::makeRef(vb.data(), vb.size() * sizeof(float)), layout);
        prog = io::load_program("shaders/glsl/screen_quad_vs.bin", "shaders/glsl/screen_quad_fs.bin");
    }

    int shutdown() {
        bgfx::destroy(prog);
        bgfx::destroy(vb_hdl);

        return 0;
    }

    void update(float dt) {
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101010ff, 1.0f);
        bgfx::setViewRect(0, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
        bgfx::setState(BGFX_STATE_WRITE_RGB |
                        BGFX_STATE_WRITE_A | 
                        BGFX_STATE_DEPTH_TEST_ALWAYS);
        bgfx::setVertexBuffer(0, vb_hdl);
        bgfx::submit(0, prog);
    }

public:
    ScreenQuadApp() : app::Application("ScreenQuad") {}
};

int main(int argc, char** argv) {
    ScreenQuadApp app;
    return app.run(argc, argv);
}