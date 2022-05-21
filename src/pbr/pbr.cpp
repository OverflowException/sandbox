#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common/application.hpp"
#include "common/file_io.h"
#include "common/pre_computations.h"
#include "common/procedural_shapes.h"
#include "controls.hpp"

class PbrApp : public app::Application
{
	void temp_tri(std::vector<float>& vb, std::vector<uint16_t>& ib) {
		glm::vec3 p[3] = {glm::vec3(0.0f, 0.0f, 0.0f),
					   	  glm::vec3(1.0f, 0.0f, 0.0f),
					   	  glm::vec3(0.0f, 1.0f, 0.0f)};

		glm::vec3 n[3] = {glm::vec3(0.0f, 0.0f, 1.0f),
					   	  glm::vec3(0.0f, 0.0f, 1.0f),
					   	  glm::vec3(0.0f, 0.0f, 1.0f)};

		glm::vec3 uv[3] = {glm::vec3(0.0f, 0.0f, 0.0f),
					   	   glm::vec3(1.0f, 0.0f, 0.0f),
					   	   glm::vec3(0.0f, 1.0f, 0.0f)};
		
		glm::vec3 t[3] = {glm::vec3(1.0f, 0.0f, 0.0f),
			   		   	  glm::vec3(1.0f, 0.0f, 0.0f),
			   		   	  glm::vec3(1.0f, 0.0f, 0.0f)};
		
		float height_ratio = 0.15f;
		glm::vec3 dp[3] = {p[0] + glm::vec3(height_ratio) * n[0],
					   	   p[1] + glm::vec3(height_ratio) * n[1],
					   	   p[2] + glm::vec3(height_ratio) * n[2]};

		glm::vec3 duv[3] = {uv[0] + glm::vec3(0.0f, 0.0f, height_ratio),
							uv[1] + glm::vec3(0.0f, 0.0f, height_ratio),
							uv[2] + glm::vec3(0.0f, 0.0f, height_ratio)};
		
		// original vertices
		for (int i = 0; i < 3; ++i) {
			vb.insert(vb.end(), {p[i].x, p[i].y, p[i].z});
			vb.insert(vb.end(), {n[i].x, n[i].y, n[i].z});
			vb.insert(vb.end(), {uv[i].x, uv[i].y, uv[i].z});
			vb.insert(vb.end(), {t[i].x, t[i].y, t[i].z});

			vb.insert(vb.end(), {p[0].x, p[0].y, p[0].z});
			vb.insert(vb.end(), {p[1].x, p[1].y, p[1].z});
			vb.insert(vb.end(), {p[2].x, p[2].y, p[2].z});

			vb.insert(vb.end(), {dp[0].x, dp[0].y, dp[0].z});
			vb.insert(vb.end(), {dp[1].x, dp[1].y, dp[1].z});
			vb.insert(vb.end(), {dp[2].x, dp[2].y, dp[2].z});

			vb.insert(vb.end(), {uv[0].x, uv[0].y});
			vb.insert(vb.end(), {uv[1].x, uv[1].y});
			vb.insert(vb.end(), {uv[2].x, uv[2].y});
		}
		// displaced vertices
		for (int i = 0; i < 3; ++i) {
			vb.insert(vb.end(), {dp[i].x, dp[i].y, dp[i].z});
			vb.insert(vb.end(), {n[i].x, n[i].y, n[i].z});
			vb.insert(vb.end(), {duv[i].x, duv[i].y, duv[i].z});
			vb.insert(vb.end(), {t[i].x, t[i].y, t[i].z});

			vb.insert(vb.end(), {p[0].x, p[0].y, p[0].z});
			vb.insert(vb.end(), {p[1].x, p[1].y, p[1].z});
			vb.insert(vb.end(), {p[2].x, p[2].y, p[2].z});

			vb.insert(vb.end(), {dp[0].x, dp[0].y, dp[0].z});
			vb.insert(vb.end(), {dp[1].x, dp[1].y, dp[1].z});
			vb.insert(vb.end(), {dp[2].x, dp[2].y, dp[2].z});

			vb.insert(vb.end(), {uv[0].x, uv[0].y});
			vb.insert(vb.end(), {uv[1].x, uv[1].y});
			vb.insert(vb.end(), {uv[2].x, uv[2].y});
		}

		ib.insert(ib.end(), {0, 2, 1,
							 3, 4, 5,
							 0, 1, 4,
							 0, 4, 3,
							 0, 3, 5,
							 0, 5, 2,
							 1, 2, 5,
							 1, 5, 4});
	}

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
	bgfx::VertexBufferHandle sphere_vb;
	bgfx::IndexBufferHandle sphere_ib;
	bgfx::VertexBufferHandle skybox_vb;

	// shader pograms
	bgfx::ProgramHandle pbr_prog;
	bgfx::ProgramHandle skybox_prog;

	// textures
	bgfx::TextureHandle tex_albedo;
	bgfx::TextureHandle tex_roughness;
	bgfx::TextureHandle tex_metallic;
	bgfx::TextureHandle tex_normal;
	bgfx::TextureHandle tex_ao;
	bgfx::TextureHandle tex_height;
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
	bgfx::UniformHandle u_metallic_roughness_ao_scale;

	// samplers
	bgfx::UniformHandle s_albedo;
	bgfx::UniformHandle s_roughness;
	bgfx::UniformHandle s_metallic;
	bgfx::UniformHandle s_normal;
	bgfx::UniformHandle s_ao;
	bgfx::UniformHandle s_height;
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

	void initialize(int argc, char** argv) {
		// bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

		std::vector<float> vb;
		std::vector<uint16_t> ib;

		// sphere vertices
		// ProceduralShapes::BufData* inter_data = new ProceduralShapes::BufData; // intermediate date
		// ProceduralShapes::gen_ico_sphere(vb, ib, ProceduralShapes::VertexAttrib::POS_NORM_UV_TANGENT,
		// 								1.5f, 0, ProceduralShapes::IndexType::TRIANGLE, inter_data);
		// ProceduralShapes::displace_prism(vb, ib, *inter_data);

		temp_tri(vb, ib);

		bgfx::VertexLayout v_layout;
		v_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)

			// prism vertices' positions, top slab + bottom slab
			.add(bgfx::Attrib::Color0,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color1,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color2,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color3,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Indices, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Weight,  3, bgfx::AttribType::Float)
			// bottom slab's uv coorinates
			.add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord2, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float)
		.end();
		sphere_vb = bgfx::createVertexBuffer(bgfx::copy(vb.data(), vb.size() * sizeof(float)), v_layout);
		sphere_ib = bgfx::createIndexBuffer(bgfx::copy(ib.data(), ib.size() * sizeof(uint16_t)));

		// skybox vertices
		ProceduralShapes::gen_cube(vb, ProceduralShapes::VertexAttrib::POS, glm::vec3(1.0f, 1.0f, 1.0f), ProceduralShapes::TRIANGLE);
		bgfx::VertexLayout skybox_layout;
		skybox_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.end();
		skybox_vb = bgfx::createVertexBuffer(bgfx::copy(vb.data(), vb.size() * sizeof(float)), skybox_layout);

		pbr_prog = io::load_program("shaders/glsl/pbr_vs.bin", "shaders/glsl/pbr_fs.bin");
		assert(bgfx::isValid(pbr_prog));
		skybox_prog = io::load_program("shaders/glsl/skybox_vs.bin", "shaders/glsl/skybox_fs.bin");
		assert(bgfx::isValid(skybox_prog));

		// textures
		std::string model_name = "textures/rough_rock";
		tex_albedo 		= io::load_texture_2d(model_name + "/albedo.png");
		tex_roughness 	= io::load_texture_2d(model_name + "/roughness.png");
		tex_metallic 	= io::load_texture_2d(model_name + "/metallic.png");
		tex_normal 		= io::load_texture_2d(model_name + "/normal.png");
		tex_ao 			= io::load_texture_2d(model_name + "/ao.png");
		tex_height		= io::load_texture_2d(model_name + "/height.png");
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
		u_metallic_roughness_ao_scale = bgfx::createUniform("u_metallic_roughness_ao_scale", bgfx::UniformType::Vec4);

		// samplers
		s_albedo = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
		s_roughness = bgfx::createUniform("s_roughness", bgfx::UniformType::Sampler);
		s_metallic = bgfx::createUniform("s_metallic", bgfx::UniformType::Sampler);
		s_normal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
		s_ao = bgfx::createUniform("s_ao", bgfx::UniformType::Sampler);
		s_height = bgfx::createUniform("s_height", bgfx::UniformType::Sampler);
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
	}

	int shutdown() {
		bgfx::destroy(sphere_vb);
		bgfx::destroy(sphere_ib);
		bgfx::destroy(skybox_vb);
		bgfx::destroy(pbr_prog);
		bgfx::destroy(skybox_prog);
		bgfx::destroy(tex_albedo);
		bgfx::destroy(tex_roughness);
		bgfx::destroy(tex_metallic);
		bgfx::destroy(tex_normal);
		bgfx::destroy(tex_ao);
		bgfx::destroy(tex_height);
		bgfx::destroy(tex_skybox);
		bgfx::destroy(tex_skybox_irr);
		bgfx::destroy(tex_skybox_prefilter);
		bgfx::destroy(tex_brdf_lut);
		bgfx::destroy(u_model_inv_t);
		bgfx::destroy(u_view_inv);
		bgfx::destroy(u_light_pos);
		bgfx::destroy(u_light_colors);
		bgfx::destroy(u_albedo);
		bgfx::destroy(u_metallic_roughness_ao_scale);
		bgfx::destroy(s_albedo);
		bgfx::destroy(s_roughness);
		bgfx::destroy(s_metallic);
		bgfx::destroy(s_normal);
		bgfx::destroy(s_ao);
		bgfx::destroy(s_height);
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

		// mterial & height map control
		Ctrl::material_control();
		Ctrl::height_map_control();
		bgfx::setUniform(u_albedo, &Ctrl::albedo);
		float mra[4] = {Ctrl::metallic, Ctrl::roughness, Ctrl::ao, Ctrl::height_map_scale};
		bgfx::setUniform(u_metallic_roughness_ao_scale, mra);

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
		bgfx::setTexture(5, s_height, tex_height);
		bgfx::setTexture(6, s_skybox_irr, tex_skybox_irr);
		bgfx::setTexture(7, s_skybox_prefilter, tex_skybox_prefilter);
		bgfx::setTexture(8, s_brdf_lut, tex_brdf_lut);
		bgfx::setVertexBuffer(0, sphere_vb);
		bgfx::setIndexBuffer(sphere_ib);
		bgfx::setState(opaque_state);
		bgfx::submit(opaque_id, pbr_prog);

		// skybox has to be in a separate drawcall since uniform changed
 		view = glm::mat4(glm::mat3(view));
		bgfx::setViewTransform(skybox_id, &view[0][0], &proj[0][0]);
		bgfx::setViewRect(skybox_id, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
		// bgfx::setTexture(0, s_skybox, tex_skybox);
		// bgfx::setTexture(0, s_skybox, tex_skybox_irr);
		bgfx::setTexture(0, s_skybox, tex_skybox);
		bgfx::setVertexBuffer(0, skybox_vb);
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