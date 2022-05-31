#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common/application.hpp"
#include "common/file_io.h"
#include "common/pre_computations.h"
#include "common/procedural_shapes.h"
#include "controls.hpp"
#include "cloth.h"
#include "timer.hpp"

class PbrApp : public app::Application
{
	// lights
	const static int light_count = 4;
	ImVec4 light_colors[light_count] = {
		ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
		ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
		ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
		ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
	};
	glm::vec3 light_pos[light_count] = {	 // in world space
		{ 5.0f,  5.0f, 0.5f},
		{ 5.0f, -5.0f, 5.0f},
		{-5.0f, -5.0f, 5.0f},
		{-5.0f,  5.0f, 5.0f},
	};
	float light_intensities[light_count] = {
		60.0f, 60.0f, 60.0f, 60.0f
	};

	// buffers
	bgfx::VertexLayout v_layout;
	std::vector<float> vb;
	std::vector<uint16_t> ib;
	bgfx::DynamicVertexBufferHandle vb_hdl;
	bgfx::IndexBufferHandle ib_hdl;
	bgfx::VertexBufferHandle skybox_vb_hdl;

	// shader pograms
	bgfx::ProgramHandle pbr_prog;
	bgfx::ProgramHandle skybox_prog;

	// textures
	bgfx::TextureHandle tex_albedo;
	bgfx::TextureHandle tex_roughness;
	bgfx::TextureHandle tex_metallic;
	bgfx::TextureHandle tex_normal;
	bgfx::TextureHandle tex_ao;
	bgfx::TextureHandle tex_skybox;
	bgfx::TextureHandle tex_skybox_irr;
	bgfx::TextureHandle tex_skybox_prefilter;
	bgfx::TextureHandle tex_brdf_lut;

	// uniforms
	bgfx::UniformHandle u_model_inv_t;
	bgfx::UniformHandle u_view_inv;
	bgfx::UniformHandle u_light_pos;
	bgfx::UniformHandle u_light_colors;
	bgfx::UniformHandle u_albedo;
	bgfx::UniformHandle u_metallic_roughness_ao;

	// samplers
	bgfx::UniformHandle s_albedo;
	bgfx::UniformHandle s_roughness;
	bgfx::UniformHandle s_metallic;
	bgfx::UniformHandle s_normal;
	bgfx::UniformHandle s_ao;
	bgfx::UniformHandle s_skybox;
	bgfx::UniformHandle s_skybox_irr;
	bgfx::UniformHandle s_skybox_prefilter;
	bgfx::UniformHandle s_brdf_lut;

	bgfx::ViewId opaque_id = 0;
	uint64_t opaque_state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW;

	bgfx::ViewId skybox_id = 1;
	uint64_t skybox_state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LEQUAL;

	phy::Cloth cloth;
	Timer timer;
	std::vector<glm::vec3> kinematic_pos;

	void initialize(int argc, char** argv) {
		// bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

		// sphere vertices
		uint16_t x_res = 16;
		uint16_t y_res = 16;
		float x_half_dim = 2.0f;
		float y_half_dim = 2.0f;
		ProceduralShapes::gen_quad_mesh(vb, ib, ProceduralShapes::VertexAttrib::POS_NORM_UV_TANGENT,
										ProceduralShapes::u16vec2(x_res, y_res), glm::vec2(x_half_dim, y_half_dim));
		
		v_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
		.end();
		vb_hdl = bgfx::createDynamicVertexBuffer(bgfx::makeRef(vb.data(), vb.size() * sizeof(float)), v_layout);
		ib_hdl = bgfx::createIndexBuffer(bgfx::makeRef(ib.data(), ib.size() * sizeof(uint16_t)));

		// set physics cloth
		// take +z side only
		std::vector<float> phy_vb(vb.begin(), vb.begin() + vb.size() / 2);
		std::vector<uint16_t> phy_ib(ib.begin(), ib.begin() + ib.size() / 2);
		// set first row to kinematics
		std::vector<uint16_t> kinematic_ids;
		for (uint16_t col = 0; col < x_res + 1; ++col) {
			kinematic_ids.push_back(col);
			glm::vec3 pos(-x_half_dim + 2.0f * x_half_dim / x_res * col,
						  y_half_dim,
						  0.0f);
			kinematic_pos.push_back(pos);
		}
		cloth.init(phy_vb,
				   phy_ib,
				   kinematic_ids,
				   phy::Cloth::Idx2({y_res + 1u, x_res + 1u}),
				   v_layout.getStride(),
				   v_layout.getOffset(bgfx::Attrib::Position),
				   v_layout.getOffset(bgfx::Attrib::TexCoord0));

		// skybox vertices
		std::vector<float> skybox_vb;
		ProceduralShapes::gen_cube(skybox_vb, ProceduralShapes::VertexAttrib::POS, glm::vec3(1.0f, 1.0f, 1.0f), ProceduralShapes::TRIANGLE);
		bgfx::VertexLayout skybox_layout;
		skybox_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.end();
		skybox_vb_hdl = bgfx::createVertexBuffer(bgfx::copy(skybox_vb.data(), skybox_vb.size() * sizeof(float)), skybox_layout);

		pbr_prog = io::load_program("shaders/glsl/pbr_vs.bin", "shaders/glsl/pbr_fs.bin");
		assert(bgfx::isValid(pbr_prog));
		skybox_prog = io::load_program("shaders/glsl/skybox_vs.bin", "shaders/glsl/skybox_fs.bin");
		assert(bgfx::isValid(skybox_prog));

		// textures
		std::string model_name = "textures/metal_grid";
		tex_albedo = io::load_texture_2d(model_name + "/albedo.png");
		tex_roughness = io::load_texture_2d(model_name + "/roughness.png");
		tex_metallic = io::load_texture_2d(model_name + "/metallic.png");
		tex_normal = io::load_texture_2d(model_name + "/normal.png");
		tex_ao = io::load_texture_2d(model_name + "/ao.png");
		tex_skybox = io::load_texture_cube({"textures/skybox/right.jpg",
		 									"textures/skybox/left.jpg",
		 									"textures/skybox/top.jpg",
		 									"textures/skybox/bottom.jpg",
		 									"textures/skybox/front.jpg",
		 									"textures/skybox/back.jpg",});
		// tex_skybox = io::load_ktx_cube_map("textures/skybox/texture-cubemap-test.ktx");
		// tex_skybox = io::load_texture_cube_immutable_ktx("textures/skybox/warehouse.ktx");
		tex_skybox_irr = pcp::gen_irradiance_map(tex_skybox, 32);
		tex_skybox_prefilter = pcp::gen_prefilter_map(tex_skybox, 256, 5);
		// tex_brdf_lut = io::load_texture_2d("textures/skybox/brdf_lut_v_flipped.png");
		tex_brdf_lut = pcp::gen_brdf_lut(512);

		// uniforms
		u_model_inv_t = bgfx::createUniform("u_model_inv_t", bgfx::UniformType::Mat4);
		u_view_inv = bgfx::createUniform("u_view_inv", bgfx::UniformType::Mat4);
		u_light_pos = bgfx::createUniform("u_light_pos", bgfx::UniformType::Vec4, light_count);
		u_light_colors = bgfx::createUniform("u_light_colors", bgfx::UniformType::Vec4, light_count);

		u_albedo = bgfx::createUniform("u_albedo", bgfx::UniformType::Vec4);
		u_metallic_roughness_ao = bgfx::createUniform("u_metallic_roughness_ao", bgfx::UniformType::Vec4);

		// samplers
		s_albedo = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
		s_roughness = bgfx::createUniform("s_roughness", bgfx::UniformType::Sampler);
		s_metallic = bgfx::createUniform("s_metallic", bgfx::UniformType::Sampler);
		s_normal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
		s_ao = bgfx::createUniform("s_ao", bgfx::UniformType::Sampler);
		s_skybox = bgfx::createUniform("s_skybox", bgfx::UniformType::Sampler);
		s_skybox_irr = bgfx::createUniform("s_skybox_irr", bgfx::UniformType::Sampler);
		s_skybox_prefilter = bgfx::createUniform("s_skybox_prefilter", bgfx::UniformType::Sampler);
		s_brdf_lut = bgfx::createUniform("s_brdf_lut", bgfx::UniformType::Sampler);

		bgfx::setViewClear(opaque_id,
							BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
							0x0f0f0fff,
							1.0f,
							0);
		bgfx::setViewRect(opaque_id, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
		bgfx::setViewFrameBuffer(opaque_id, BGFX_INVALID_HANDLE); // set default back buffer. To counteract irradiance map's framebuffer settings
		timer.start();
	}

	int shutdown() {
		bgfx::destroy(vb_hdl);
		bgfx::destroy(ib_hdl);
		bgfx::destroy(skybox_vb_hdl);
		bgfx::destroy(pbr_prog);
		bgfx::destroy(skybox_prog);
		bgfx::destroy(tex_albedo);
		bgfx::destroy(tex_roughness);
		bgfx::destroy(tex_metallic);
		bgfx::destroy(tex_normal);
		bgfx::destroy(tex_ao);
		bgfx::destroy(tex_skybox);
		bgfx::destroy(tex_skybox_irr);
		bgfx::destroy(tex_skybox_prefilter);
		bgfx::destroy(tex_brdf_lut);
		bgfx::destroy(u_model_inv_t);
		bgfx::destroy(u_view_inv);
		bgfx::destroy(u_light_pos);
		bgfx::destroy(u_light_colors);
		bgfx::destroy(u_albedo);
		bgfx::destroy(u_metallic_roughness_ao);
		bgfx::destroy(s_albedo);
		bgfx::destroy(s_roughness);
		bgfx::destroy(s_metallic);
		bgfx::destroy(s_normal);
		bgfx::destroy(s_ao);
		bgfx::destroy(s_skybox);
		bgfx::destroy(s_skybox_irr);
		bgfx::destroy(s_skybox_prefilter);
		bgfx::destroy(s_brdf_lut);

		return 0;
	}

	void onReset() {
		bgfx::setViewClear(opaque_id,
							BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
							0x0f0f0fff,
							1.0f,
							0);
		bgfx::setViewRect(opaque_id, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
	}

	void update(float dt) {
		Ctrl::camera_control();
		glm::mat4 proj = glm::perspective(glm::radians(60.0f), float(getWidth()) / getHeight(), 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(Ctrl::eye,
									Ctrl::eye + Ctrl::front, Ctrl::up);
		glm::mat4 view_inv = glm::inverse(view);
		bgfx::setViewTransform(opaque_id, &view[0][0], &proj[0][0]);
		bgfx::setViewRect(opaque_id, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
		bgfx::setUniform(u_view_inv, &view_inv);

		Ctrl::model_control();
		// rotation order: z-y-x
		glm::mat4 model(1.0f);
		model = glm::rotate(model, float(Ctrl::model_euler.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::rotate(model, float(Ctrl::model_euler.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, float(Ctrl::model_euler.x), glm::vec3(1.0f, 0.0f, 0.0f));
		bgfx::setTransform(&model[0][0]);
		glm::mat4 model_inv_t = glm::transpose(glm::inverse(model));
		bgfx::setUniform(u_model_inv_t, &model_inv_t);

		Ctrl::material_control();
		bgfx::setUniform(u_albedo, &Ctrl::albedo);
		float mra[3] = {Ctrl::metallic, Ctrl::roughness, Ctrl::ao};
		bgfx::setUniform(u_metallic_roughness_ao, mra);

		Ctrl::kinematics_control();
		// rotation order: z-y-x
		glm::mat4 k_mat(1.0f);
		k_mat = glm::rotate(k_mat, float(Ctrl::kinematics_euler.z), glm::vec3(0.0f, 0.0f, 1.0f));
		k_mat = glm::rotate(k_mat, float(Ctrl::kinematics_euler.y), glm::vec3(0.0f, 1.0f, 0.0f));
		k_mat = glm::rotate(k_mat, float(Ctrl::kinematics_euler.x), glm::vec3(1.0f, 0.0f, 0.0f));
		std::vector<glm::vec3> cur_kinematic_pos = kinematic_pos;
		for (auto& p : cur_kinematic_pos) {
			p = glm::vec3(k_mat * glm::vec4(p, 1.0f));
		}

		// light control
		// TODO: lighting position computation is not correct
		glm::vec4 view_light_pos[light_count];
		for (int i = 0; i < light_count; ++i) {
			view_light_pos[i] = view * glm::vec4(light_pos[i], 1.0f);
		}
		bgfx::setUniform(u_light_pos, view_light_pos, light_count);
		{
			ImGui::Begin("lights");
			std::string color_name = "light color  ";
			std::string intensity_name = "light intensity  ";
			for (int i = 0; i < light_count; ++i) {
				color_name.back() = '0' + i;
				intensity_name.back() = '0' + i;
				ImGui::SliderFloat(intensity_name.c_str(), &light_intensities[i], 0.0f, 100.0f);
				ImGui::ColorPicker3(color_name.c_str(), (float*)&(light_colors[i]));
			}
			ImGui::End();
		}
		glm::vec4 light_color_intensities[4];
		for (int i = 0; i < light_count; ++i) {
			light_color_intensities[i] = glm::vec4(light_colors[i].x * light_intensities[i],
													light_colors[i].y * light_intensities[i],
													light_colors[i].z * light_intensities[i],
													light_colors[i].w * light_intensities[i]);
		}
		bgfx::setUniform(u_light_colors, light_color_intensities, light_count);
		
  		bgfx::setTexture(0, s_albedo, tex_albedo);
		bgfx::setTexture(1, s_roughness, tex_roughness);
		bgfx::setTexture(2, s_metallic, tex_metallic);
		bgfx::setTexture(3, s_normal, tex_normal);
		bgfx::setTexture(4, s_ao, tex_ao);
		bgfx::setTexture(5, s_skybox_irr, tex_skybox_irr);
		bgfx::setTexture(6, s_skybox_prefilter, tex_skybox_prefilter);
		bgfx::setTexture(7, s_brdf_lut, tex_brdf_lut);

		// update physics
		cloth.update_kinematics(cur_kinematic_pos);
		cloth.update(0.008);
		// copy both sides
		cloth.copy_back(vb.begin(),
						v_layout.getStride(),
						v_layout.getOffset(bgfx::Attrib::Position),
						v_layout.getOffset(bgfx::Attrib::Normal),
						v_layout.getOffset(bgfx::Attrib::Tangent),
						false, true);
		cloth.copy_back(vb.begin() + vb.size() / 2,
						v_layout.getStride(),
						v_layout.getOffset(bgfx::Attrib::Position),
						v_layout.getOffset(bgfx::Attrib::Normal),
						v_layout.getOffset(bgfx::Attrib::Tangent),
						true, false);

		bgfx::update(vb_hdl, 0, bgfx::makeRef(vb.data(), vb.size() * sizeof(float)));
		bgfx::setVertexBuffer(0, vb_hdl);
		bgfx::setIndexBuffer(ib_hdl);
		bgfx::setState(opaque_state);
		bgfx::submit(opaque_id, pbr_prog);

		// skybox has to be in a separate drawcall since uniform changed
 		view = glm::mat4(glm::mat3(view));
		bgfx::setViewTransform(skybox_id, &view[0][0], &proj[0][0]);
		bgfx::setViewRect(skybox_id, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
		// bgfx::setTexture(0, s_skybox, tex_skybox);
		// bgfx::setTexture(0, s_skybox, tex_skybox_irr);
		bgfx::setTexture(0, s_skybox, tex_skybox);
		bgfx::setVertexBuffer(0, skybox_vb_hdl);
		bgfx::setState(skybox_state);
		bgfx::submit(skybox_id, skybox_prog);

		// do not call bgfx::frame() here. Or imgui would flash
		// bgfx::frame();
	}
public:
	PbrApp() : app::Application("PBR") {}
};

int main(int argc, char** argv) {
	PbrApp app;
	return app.run(argc, argv);
}