#include <iostream>
#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common/application.hpp"
#include "common/file_io.h"
#include "common/pre_computations.h"
#include "common/procedural_shapes.h"
#include "controls.hpp"
#include "camera_control.h"
#include "ray_caster.h"
#include "math_utils.hpp"

#include "common/renderer.hpp"

std::ostream& operator<<(std::ostream& os, const glm::vec4& v4) {
	os << v4.x << ", " << v4.y << ", " << v4.z << ", " << v4.w;
	return os;
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& v3) {
	os << v3.x << ", " << v3.y << ", " << v3.z;
	return os;
}

class ToolApp : public app::Application
{
	// lights
	const static int light_count = 4;
	glm::vec3 light_colors[light_count] = {
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
	};
	glm::vec3 light_dirs[light_count] = {	 // in world space
		{-1.0f, -1.0f,  1.0f},
		{ 1.0f, -1.0f,  1.0f},
		{ 1.0f,  1.0f, -1.0f},
		{-1.0f,  1.0f, -1.0f},
	};
	float light_intensities[light_count] = {
		5.0f, 5.0f, 5.0f, 5.0f
	};

	// camera
	rdr::Renderer::Camera init_camera = {
		.eye 	= glm::vec3(0.0f, 0.0f, 5.0f),
		.front	= glm::vec3(0.0f, 0.0f, -1.0f),
		.up		= glm::vec3(0.0f, 1.0f, 0.0f),
		.width	= float(getWidth()),
		.height	= float(getHeight()),
		.fovy	= glm::radians(60.0f),
		.near	= 0.1f,
		.far	= 100.0f,
	};

	std::shared_ptr<rdr::Renderer> renderer;

	std::shared_ptr<CameraController> camera_control = nullptr;

	std::shared_ptr<RayCaster> ray_caster = nullptr;

	std::vector<float> vb;
	std::vector<uint16_t> ib;

	struct Geometry {
		enum Shape {
			Sphere,
			Cylinder,
		};

		Geometry(Shape s,
				 std::shared_ptr<rdr::Renderer> renderer,
				 std::shared_ptr<RayCaster> ray_caster) {
			std::vector<float> vb;
			if (s == Sphere) {
				ProceduralShapes::gen_ico_sphere(vb, ib, ProceduralShapes::VertexAttrib::POS_NORM_UV_TANGENT,
		 										 0.02f, 2, ProceduralShapes::IndexType::TRIANGLE);
			} else if (s == Cylinder) {
						ProceduralShapes::gen_z_cone(vb, ib, ProceduralShapes::VertexAttrib::POS_NORM_UV_TANGENT,
										 				 0.5f, 0.9, 1.5f, 32, 16, ProceduralShapes::IndexType::TRIANGLE);
			}
			make_streams(vb, vb_pos, vb_normal, vb_uv, vb_tangent);

			rdr::Renderer::Primitive::VertexDesc desc_pos = {
				.data = vb_pos.data(),
				.size = uint32_t(vb_pos.size() * sizeof(float)),
				.attrib = bgfx::Attrib::Position,
				.num = 3,
				.type = bgfx::AttribType::Float,
				.is_dynamic = false,
			};

			rdr::Renderer::Primitive::VertexDesc desc_normal = {
				.data = vb_normal.data(),
				.size = uint32_t(vb_normal.size() * sizeof(float)),
				.attrib = bgfx::Attrib::Normal,
				.num = 3,
				.type = bgfx::AttribType::Float,
				.is_dynamic = false,
			};

			rdr::Renderer::Primitive::VertexDesc desc_uv = {
				.data = vb_uv.data(),
				.size = uint32_t(vb_uv.size() * sizeof(float)),
				.attrib = bgfx::Attrib::TexCoord0,
				.num = 2,
				.type = bgfx::AttribType::Float,
				.is_dynamic = false,
			};

			rdr::Renderer::Primitive::VertexDesc desc_tangent = {
				.data = vb_tangent.data(),
				.size = uint32_t(vb_tangent.size() * sizeof(float)),
				.attrib = bgfx::Attrib::Tangent,
				.num = 3,
				.type = bgfx::AttribType::Float,
				.is_dynamic = false,
			};

			rdr::Renderer::Primitive::IndexDesc desc_idx = {
				.data = ib.data(),
				.size = uint32_t(ib.size() * sizeof(uint16_t)),
				.is_dynamic = false,
			};

			// set primitive
			rdr::Renderer::Primitive prim;
			prim.add_vertex_buffer(desc_pos);
			prim.add_vertex_buffer(desc_normal);
			prim.add_vertex_buffer(desc_uv);
			prim.add_vertex_buffer(desc_tangent);
			prim.set_index_buffer(desc_idx);
			rdr::Renderer::Material material = {
				glm::vec3(0.7f, 0.7f, 0.3f),
				0.8f,
				0.3f,
				1.0f,
				"pbr"
			};
			prim.set_material(material);
			prim.set_transform(glm::mat4(1.0f));
			prim_id = renderer->add_primitive(prim);

			if (ray_caster)  {
				RayCaster::TriMeshDesc prim_rc_desc = {
					.p_desc = {.data = vb_pos.data(),	.count = (uint32_t)vb_pos.size(),},
					.n_desc = {.data = vb_normal.data(),.count = (uint32_t)vb_normal.size(),},
					.i_desc = {.data = ib.data(),		.count = (uint32_t)ib.size(),},
					.transform = glm::mat4(1.0f),
				};
				ray_caster_id = ray_caster->add_tri_mesh(prim_rc_desc);
			}
		}

		void make_streams(const std::vector<float>& vb,
						  std::vector<float>& pos,
						  std::vector<float>& normal,
						  std::vector<float>& uv,
						  std::vector<float>& tangent) {
			size_t vertex_count = vb.size() / 11;
			for (int i = 0; i < vertex_count; ++i) {
				int j = i * 11;
				pos		.insert(pos.end(),		{vb[j],		vb[j + 1], vb[j + 2]});
				normal	.insert(normal.end(),	{vb[j + 3], vb[j + 4], vb[j + 5]});
				uv		.insert(uv.end(),		{vb[j + 6], vb[j + 7]});
				tangent	.insert(tangent.end(),	{vb[j + 8], vb[j + 9], vb[j + 10]});
			}
		}

		std::vector<float> vb_pos;
		std::vector<float> vb_normal;
		std::vector<float> vb_uv;
		std::vector<float> vb_tangent;
		std::vector<uint16_t> ib;
		size_t prim_id = -1;
		size_t ray_caster_id = -1;
	};

	std::shared_ptr<Geometry> target;

	std::shared_ptr<Geometry> doodad;

	std::vector<std::shared_ptr<Geometry>> markers;

	void initialize(int argc, char** argv) {
		bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

		// initialize renderer
		renderer.reset(new rdr::Renderer);

		// initialize camera controller
		camera_control.reset(new CameraController(init_camera));

		// initilize ray caster
		ray_caster.reset(new RayCaster);

		// set camera
		renderer->camera() = init_camera;

		// set lighting
		for (int i = 0; i < light_count; ++i) {
			renderer->add_light({
				light_dirs[i],
			 	light_colors[i],
			 	light_intensities[i]});
		}

		// set target primitive
		target.reset(new Geometry(Geometry::Shape::Cylinder,
								  renderer,
								  ray_caster));
		glm::mat4 target_transform = glm::rotate(glm::mat4(1.0f), float(M_PI_2), glm::vec3(-1.0f, 0.0f, 0.0f));
		renderer->primitive(target->prim_id).transform = target_transform;
		ray_caster->update_transform(target->ray_caster_id, target_transform);

		// random doodad
		doodad.reset(new Geometry(Geometry::Shape::Sphere,
								  renderer,
								  ray_caster));
		glm::mat4 doodad_transform = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, -1.0f)) *
									 glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));
		renderer->primitive(doodad->prim_id).transform = doodad_transform;
		ray_caster->update_transform(doodad->ray_caster_id, doodad_transform);

		// set marker primitive
		markers.resize(target->vb_pos.size() / 3);
		for (int i = 0; i < markers.size(); ++i) {
			markers[i].reset(new Geometry(Geometry::Shape::Sphere,
					   		 			  renderer,
							 			  ray_caster));
			std::shared_ptr<Geometry> marker_ptr = markers[i];
			renderer->primitive(marker_ptr->prim_id).set_material({
				.albedo = glm::vec3(1.0f, 1.0f, 1.0f),
				.metallic = 0.3f,
				.roughness = 0.6f,
				.ao = 1.0f,
				.shader_name = "pbr",
			});

			glm::vec3 marker_pos = target_transform * glm::vec4(glm::make_vec3(&target->vb_pos[i * 3]) + glm::make_vec3(&target->vb_normal[i * 3]) * 0.1f, 1.0f);
			renderer->primitive(marker_ptr->prim_id).set_transform(glm::translate(glm::mat4(1.0f), marker_pos));
		}
		// renderer->primitive(target->prim_id).set_transform();

		renderer->reset(uint16_t(getWidth()), uint16_t(getHeight()));
	}

	int shutdown() {
		ray_caster = nullptr;
		camera_control = nullptr;
		renderer = nullptr;
		return 0;
	}

	void onReset() {
		if (renderer) {
			init_camera.width = getWidth();
			init_camera.height = getHeight();
			camera_control->camera() = init_camera;
			renderer->camera() = init_camera;
			renderer->reset(uint16_t(getWidth()), uint16_t(getHeight()));
		}
	}

	void update(float dt) {
		renderer->camera() = camera_control->camera();
		renderer->render();

		// do not call bgfx::frame() here. Or imgui would flash
		// bgfx::frame();
	}

	void onScroll(double xoffset, double yoffset) {
		double xpos = 0.0f;
		double ypos = 0.0f;
		glfwGetCursorPos(mWindow, &xpos, &ypos);
		std::cout << "onScroll, xoffset = " << xoffset << ", yoffset = " << yoffset << std::endl;
		std::cout << ", x = " << xpos << ", y = " << ypos << std::endl;

		CameraController::MouseButton::Enum btn = CameraController::MouseButton::Middle;
		CameraController::MouseAction::Enum act = CameraController::MouseAction::None;
		if (yoffset < 0) {
			act = CameraController::MouseAction::ScrollDown;
		} else if (yoffset > 0) {
			act = CameraController::MouseAction::ScrollUp;
		} else {}
		camera_control->mouse_action(btn, act, {(float)xpos, (float)ypos});
	}

	void onCursorPos(double xpos, double ypos) {
		camera_control->cursor_move({(float)xpos, (float)ypos});

		// temp: mouse picking
		bool left_button_pressed = (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
		if (!left_button_pressed) {
			return;
		}

		rdr::Renderer::Camera cur_camera = camera_control->camera();
		MathUtil::Camera math_camera = *reinterpret_cast<MathUtil::Camera*>(&cur_camera);
		RayCaster::Ray ray = {
			.origin = cur_camera.eye,
			.direction = glm::normalize(MathUtil::ndc2world(MathUtil::wnd2ndc({xpos, ypos}, math_camera), -1.0f, math_camera) - math_camera.eye),
		};
		std::cout << "cast origin = " << ray.origin << std::endl;
		std::cout << "cast direction = " << ray.direction << std::endl;
		RayCaster::RayTriManifold cast_result =  std::move(ray_caster->intersect(ray));
		if (cast_result.intersect) {
			glm::vec3 p_itsc = ray.origin + ray.direction * cast_result.t_ray;
			// brush
			RayCaster::Sphere brush = {
				.center = p_itsc,
				.radius = 0.15f,
			};

			auto enveloped = std::move(ray_caster->envelope(cast_result.mesh_id, brush));
			for (uint16_t id : enveloped) {
				renderer->primitive(markers[id]->prim_id).material.albedo = glm::vec3(1.0f, 0.0f, 1.0f);
			}
		} else {
			std::cout << "cast no intersection" << std::endl;
		}
	}

	void onMouseButton(int button, int action, int mods) {
		double xpos = 0.0f;
		double ypos = 0.0f;
		glfwGetCursorPos(mWindow, &xpos, &ypos);
		// std::cout << "onMouseButton, button = " << button << ", action = " << action << ", mods = " << mods;
		// std::cout << ", x = " << xpos << ", y = " << ypos << std::endl;

		CameraController::MouseButton::Enum btn = (CameraController::MouseButton::Enum)button;
		CameraController::MouseAction::Enum act = (CameraController::MouseAction::Enum)action;

		camera_control->mouse_action(btn, act, {(float)xpos, (float)ypos});
	}

public:
	ToolApp() : app::Application("Cloth Tool") {}
};

int main(int argc, char** argv) {
	ToolApp app;
	return app.run(argc, argv);
}