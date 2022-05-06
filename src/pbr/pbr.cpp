#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "common/application.hpp"
#include "common/file_io.hpp"
#include "common/procedural_shapes.h"
#include "controls.hpp"

float skyboxVertices[] = {
    	// positions          
    	-1.0f,  1.0f, -1.0f,
    	-1.0f, -1.0f, -1.0f,
    	 1.0f, -1.0f, -1.0f,
    	 1.0f, -1.0f, -1.0f,
    	 1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,

    	-1.0f, -1.0f,  1.0f,
    	-1.0f, -1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f,  1.0f,
    	-1.0f, -1.0f,  1.0f,

     	1.0f, -1.0f, -1.0f,
     	1.0f, -1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f, -1.0f,
     	1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
    	-1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f, -1.0f,  1.0f,
    	-1.0f, -1.0f,  1.0f,

    	-1.0f,  1.0f, -1.0f,
     	1.0f,  1.0f, -1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
    	-1.0f,  1.0f,  1.0f,
    	-1.0f,  1.0f, -1.0f,

    	-1.0f, -1.0f, -1.0f,
    	-1.0f, -1.0f,  1.0f,
     	1.0f, -1.0f, -1.0f,
     	1.0f, -1.0f, -1.0f,
    	-1.0f, -1.0f,  1.0f,
     	1.0f, -1.0f,  1.0f
};

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
	bgfx::VertexBufferHandle pbr_vb;
	bgfx::IndexBufferHandle pbr_ib;
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
	bgfx::TextureHandle tex_skybox;

	// uniforms
	bgfx::UniformHandle u_model_inv_t;
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

	
	uint64_t opaque_state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW;

	uint64_t skybox_state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LEQUAL;

	void initialize(int argc, char** argv) {
		std::vector<float> vb;
		std::vector<uint16_t> ib;

		ProceduralShapes::gen_ico_sphere(vb, ib, ProceduralShapes::VertexAttrib::POS_NORM_UV_TANGENT,
											1.5f, 3, ProceduralShapes::IndexType::TRIANGLE);

		// bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS | BGFX_DEBUG_WIREFRAME);

		// model vertices
		bgfx::VertexLayout v_layout;
		v_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
		.end();
		pbr_vb = bgfx::createVertexBuffer(bgfx::copy(vb.data(), vb.size() * sizeof(float)), v_layout);
		pbr_ib = bgfx::createIndexBuffer(bgfx::copy(ib.data(), ib.size() * sizeof(uint16_t)));

		// skybox vertices
		bgfx::VertexLayout skybox_layout;
		skybox_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.end();
		int test = sizeof(skyboxVertices);
		skybox_vb = bgfx::createVertexBuffer(bgfx::copy(skyboxVertices, sizeof(skyboxVertices)), skybox_layout);

		pbr_prog = io::load_program("shaders/glsl/pbr_vs.bin", "shaders/glsl/pbr_fs.bin");
		assert(bgfx::isValid(pbr_prog));
		skybox_prog = io::load_program("shaders/glsl/skybox_vs.bin", "shaders/glsl/skybox_fs.bin");
		assert(bgfx::isValid(skybox_prog));

		// textures
		tex_albedo = io::load_texture_2d("textures/metal_chipped_paint/albedo.png");
		tex_roughness = io::load_texture_2d("textures/metal_chipped_paint/roughness.png");
		tex_metallic = io::load_texture_2d("textures/metal_chipped_paint/metallic.png");
		tex_normal = io::load_texture_2d("textures/metal_chipped_paint/normal.png");
		tex_ao = io::load_texture_2d("textures/metal_chipped_paint/ao.png");
		tex_skybox = io::load_texture_cube({"textures/skybox/right.jpg",
		 									"textures/skybox/left.jpg",
		 									"textures/skybox/top.jpg",
		 									"textures/skybox/bottom.jpg",
		 									"textures/skybox/front.jpg",
		 									"textures/skybox/back.jpg",});

		// tex_skybox = io::load_ktx_cube_map("textures/skybox/texture-cubemap-test.ktx");
		// tex_skybox = io::load_texture_cube_immutable_ktx("textures/skybox/warehouse.ktx");

		// uniforms
		u_model_inv_t = bgfx::createUniform("u_model_inv_t", bgfx::UniformType::Mat4);
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
	}

	int shutdown() {
		bgfx::destroy(pbr_vb);
		bgfx::destroy(pbr_ib);
		bgfx::destroy(skybox_vb);
		bgfx::destroy(pbr_prog);
		bgfx::destroy(skybox_prog);
		bgfx::destroy(tex_albedo);
		bgfx::destroy(tex_roughness);
		bgfx::destroy(tex_metallic);
		bgfx::destroy(tex_normal);
		bgfx::destroy(tex_ao);
		bgfx::destroy(tex_skybox);
		bgfx::destroy(u_model_inv_t);
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

		return 0;
	}

	void onReset() {
		bgfx::setViewClear(0,
							BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
							0x0f0f0fff,
							1.0f,
							0);
		bgfx::setViewRect(0, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
	}

	int counter = 0;
	void update(float dt) {
		Ctrl::camera_control();
		glm::mat4 proj = glm::perspective(glm::radians(60.0f), float(getWidth()) / getHeight(), 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(Ctrl::eye,
									Ctrl::eye + Ctrl::front, Ctrl::up);
		bgfx::setViewTransform(0, &view[0][0], &proj[0][0]);
		bgfx::setViewRect(0, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));

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
		// bgfx::setTexture(5, s_skybox, tex_skybox);
		bgfx::setVertexBuffer(0, pbr_vb);
		bgfx::setIndexBuffer(pbr_ib);
		bgfx::setState(opaque_state);
		bgfx::submit(0, pbr_prog);

		// skybox
		view = glm::mat4(glm::mat3(view));
		bgfx::setViewTransform(1, &view[0][0], &proj[0][0]);
		bgfx::setViewRect(1, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));
		bgfx::setTexture(0, s_skybox, tex_skybox);
		bgfx::setVertexBuffer(0, skybox_vb);
		bgfx::setState(skybox_state);
		bgfx::submit(1, skybox_prog);

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