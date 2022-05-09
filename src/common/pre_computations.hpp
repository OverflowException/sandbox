#pragma once

#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "bgfx/bgfx.h"

#include "bimg/bimg.h"
#include "bx/error.h"
#include "bimg/decode.h"
#include "common/procedural_shapes.h"

namespace pcp {

bgfx::TextureHandle gen_irradiance_map(bgfx::TextureHandle cube_tex,
										int resolution) {
	glm::mat4 views[6] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),  // +x
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -x
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),  // +y
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),  // -y
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),  // +z
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))   // -z
	};
	uint8_t side_order[6] = {
		BGFX_CUBE_MAP_POSITIVE_X, BGFX_CUBE_MAP_NEGATIVE_X,
		BGFX_CUBE_MAP_POSITIVE_Y, BGFX_CUBE_MAP_NEGATIVE_Y,
		BGFX_CUBE_MAP_POSITIVE_Z, BGFX_CUBE_MAP_NEGATIVE_Z,
	};

	std::vector<float> vb;
	ProceduralShapes::gen_cube(vb, ProceduralShapes::VertexAttrib::POS, glm::vec3(1.0f, 1.0f, 1.0f), ProceduralShapes::IndexType::TRIANGLE);
	uint32_t v_num = vb.size() / 3;
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	.end();
	bgfx::VertexBufferHandle vb_hdl = bgfx::createVertexBuffer(bgfx::makeRef(vb.data(), vb.size() * sizeof(float)), layout);

	bgfx::TextureHandle tex_radiance_map = bgfx::createTextureCube(resolution,
																	false,
																	1,
																	bgfx::TextureFormat::RGBA16F,
																	BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_BLIT_DST);
	bgfx::TextureHandle tex_radiance_rt = bgfx::createTexture2D(resolution,
																		resolution,
																		false,
																		1,
																		bgfx::TextureFormat::RGBA16F,
																		BGFX_TEXTURE_RT);

	bgfx::FrameBufferHandle fb = bgfx::createFrameBuffer(1, &tex_radiance_rt, true);
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1.0f);
	bgfx::UniformHandle s_tex = bgfx::createUniform("s_env", bgfx::UniformType::Sampler);
	bgfx::ProgramHandle prog = io::load_program("shaders/glsl/skybox_vs.bin", "shaders/glsl/irradiance_convolution_fs.bin");
	uint64_t state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		// | BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_ALWAYS;

	for (int i = 0; i < 6; ++i) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
		bgfx::setViewRect(0, 0, 0, resolution, resolution);
		bgfx::setViewFrameBuffer(0, fb);
		bgfx::setViewTransform(0, &views[i], &proj);
		bgfx::setState(state);
		bgfx::setTexture(0, s_tex, cube_tex);
		bgfx::setVertexBuffer(0, vb_hdl);
		bgfx::submit(0, prog);
		bgfx::blit(1, tex_radiance_map, 0, 0, 0, side_order[i], tex_radiance_rt);//, 0, 0, 0, 0, resolution, resolution);
		bgfx::frame();
	}

	// todo: destroy stuff
	bgfx::destroy(prog);
	bgfx::destroy(s_tex);
	bgfx::destroy(fb);
	bgfx::destroy(vb_hdl);

	return tex_radiance_map;
}
}