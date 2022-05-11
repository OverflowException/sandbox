#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>

#include "pre_computations.h"

#include "glm/matrix.hpp"
#include "bimg/bimg.h"
#include "bx/error.h"
#include "bimg/decode.h"
#include "procedural_shapes.h"
#include "file_io.h"

namespace pcp {

bgfx::TextureHandle convolute_cube_map(bgfx::TextureHandle cube_tex,
										int res,
										bgfx::ProgramHandle prog,
										std::vector<UniformContext> ucs) {
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
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	.end();
	bgfx::VertexBufferHandle vb_hdl = bgfx::createVertexBuffer(bgfx::copy(vb.data(), vb.size() * sizeof(float)), layout);

	bgfx::TextureHandle tex_cube_map = bgfx::createTextureCube(res,
																false,
																1,
																bgfx::TextureFormat::RGBA16F,
																BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_BLIT_DST);
	bgfx::TextureHandle tex_2d_rt = bgfx::createTexture2D(res,
														res,
														false,
														1,
														bgfx::TextureFormat::RGBA16F,
														BGFX_TEXTURE_RT);

	bgfx::FrameBufferHandle fb = bgfx::createFrameBuffer(1, &tex_2d_rt, true);
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1.0f);
	bgfx::UniformHandle s_tex = bgfx::createUniform("s_env", bgfx::UniformType::Sampler);
	uint64_t state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		// | BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_ALWAYS;

	for (int i = 0; i < 6; ++i) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
		bgfx::setViewRect(0, 0, 0, res, res);
		bgfx::setViewFrameBuffer(0, fb);
		bgfx::setViewTransform(0, &views[i], &proj);
		bgfx::setState(state);
		bgfx::setTexture(0, s_tex, cube_tex);
		bgfx::setVertexBuffer(0, vb_hdl);
		for (auto& uc : ucs) {
			bgfx::setUniform(uc.hdl, uc.value, uc.num);
		}
		bgfx::submit(0, prog);
		bgfx::blit(1, tex_cube_map, 0, 0, 0, side_order[i], tex_2d_rt);
		bgfx::frame();
	}

	// todo: destroy stuff
	bgfx::destroy(s_tex);
	bgfx::destroy(fb);
	bgfx::destroy(vb_hdl);

	return tex_cube_map;
}


bgfx::TextureHandle gen_irradiance_map(bgfx::TextureHandle cube_tex,
										int res) {
	bgfx::ProgramHandle prog = io::load_program("common_shaders/glsl/skybox_vs.bin", "common_shaders/glsl/irradiance_convolution_fs.bin");
	bgfx::TextureHandle hdl = convolute_cube_map(cube_tex, res, prog, std::vector<UniformContext>());
	bgfx::destroy(prog);
	return hdl;
}

void blit_cube_map(bgfx::TextureHandle dst,
					int dst_mip,
					bgfx::TextureHandle src,
					int src_mip) {
	uint8_t side_order[6] = {
		BGFX_CUBE_MAP_POSITIVE_X, BGFX_CUBE_MAP_NEGATIVE_X,
		BGFX_CUBE_MAP_POSITIVE_Y, BGFX_CUBE_MAP_NEGATIVE_Y,
		BGFX_CUBE_MAP_POSITIVE_Z, BGFX_CUBE_MAP_NEGATIVE_Z,
	};

	for (int i = 0; i < 6; ++i) {
		bgfx::blit(1, dst, dst_mip, 0, 0, side_order[i], src, src_mip, 0, 0, side_order[i]);
		bgfx::frame();
	}
}

bgfx::TextureHandle gen_prefilter_map(bgfx::TextureHandle cube_tex,
										int res,
										int mip_levels) {
	bgfx::ProgramHandle prog = io::load_program("common_shaders/glsl/skybox_vs.bin", "common_shaders/glsl/prefilter_fs.bin");

	bgfx::UniformHandle u_roughness = bgfx::createUniform("u_roughness", bgfx::UniformType::Vec4);
	std::vector<UniformContext> ucs(1);
	bgfx::TextureHandle hdl = bgfx::createTextureCube(res,
													true,
													1,
													bgfx::TextureFormat::RGBA16F, BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_BLIT_DST);
	for(int m = 0; m < mip_levels; ++m) {
		int mip_res = res * glm::pow(0.5f, m);
		glm::vec4 mip_roughness = glm::vec4((float)m / (float)(mip_levels - 1));
		ucs[0] = {u_roughness, &mip_roughness, 1};
		bgfx::TextureHandle mip_cube_tex = convolute_cube_map(cube_tex, mip_res, prog, ucs);
		blit_cube_map(hdl, m, mip_cube_tex, 0);
		bgfx::destroy(mip_cube_tex);
	}

	bgfx::destroy(u_roughness);
	bgfx::destroy(prog);

	return hdl;
}

bgfx::TextureHandle gen_brdf_lut(int res) {
 	std::vector<float> vb = {
		-1.0f,  1.0f, 0.0f,
    	-1.0f, -1.0f, 0.0f,
    	 1.0f, -1.0f, 0.0f,
    	 1.0f, -1.0f, 0.0f,
    	 1.0f,  1.0f, 0.0f,
    	-1.0f,  1.0f, 0.0f,
	};

	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	.end();

	// use bgfx::copy here insead of bgfx::makeRef.
	// bgfx::frame only kick starts rendering,
	// there is no telling if bgfx could complete rendering a frame before vb is released
	bgfx::VertexBufferHandle vb_hdl = bgfx::createVertexBuffer(bgfx::copy(vb.data(), vb.size() * sizeof(float)), layout);
	bgfx::ProgramHandle prog = io::load_program("common_shaders/glsl/brdf_lut_vs.bin", "common_shaders/glsl/brdf_lut_fs.bin");

	bgfx::TextureHandle tex_lut = bgfx::createTexture2D(res,
														res,
														false,
														1,
														bgfx::TextureFormat::RG16F,
														BGFX_TEXTURE_RT);
	
	bgfx::FrameBufferHandle fb = bgfx::createFrameBuffer(1, &tex_lut, false);

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101010ff, 1.0f);
	bgfx::setViewRect(0, 0, 0, res, res);
	bgfx::setViewFrameBuffer(0, fb);	
	bgfx::setState(BGFX_STATE_WRITE_RGB |
                	BGFX_STATE_WRITE_A | 
                	BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setVertexBuffer(0, vb_hdl);
	bgfx::submit(0, prog);
	bgfx::frame();

	// todo: destroy stuff
	bgfx::destroy(fb);
	bgfx::destroy(prog);
	bgfx::destroy(vb_hdl);

	return tex_lut;
}

}