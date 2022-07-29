#include "renderer.hpp"
#include "file_io.h"
#include "glm/gtc/matrix_transform.hpp"

const std::string shader_base_path = "shaders/glsl/";
const std::vector<std::string> shader_names = {
    "pbr",
    "skybox"
};
const bgfx::ViewId opaque_id = 0;
const uint64_t opaque_state = 0
    | BGFX_STATE_WRITE_RGB
	| BGFX_STATE_WRITE_A
	| BGFX_STATE_WRITE_Z
	| BGFX_STATE_DEPTH_TEST_LESS
	| BGFX_STATE_CULL_CW;

namespace rdr {

Renderer::Renderer() {
    // load all shaders
    for (auto& name : shader_names) {
        std::string vs_path = shader_base_path + name + "_vs.bin";
        std::string fs_path = shader_base_path + name + "_fs.bin";
        _shaders[name] = io::load_program(vs_path.c_str(), fs_path.c_str());
    }

}

Renderer::~Renderer() {
    for (auto& ele : _shaders) {
        bgfx::destroy(ele.second);
    }

    for (auto& ele : _uniforms) {
        bgfx::destroy(ele.second);
    }
}

void Renderer::render() {
    glm::mat4 proj = glm::perspective(_camera.fovy, _camera.width / _camera.height, _camera.near, _camera.far);
	glm::mat4 view = glm::lookAt(_camera.eye, _camera.eye + _camera.front, _camera.up);

    // set view transform once per view
    bgfx::setViewTransform(opaque_id, &view[0][0], &proj[0][0]);
    bgfx::setViewRect(opaque_id, 0, 0, uint16_t(_camera.width), uint16_t(_camera.height));

    for (auto& ele : _primitives) {
        Primitive& primitive = ele.second;
        for (int stream = 0; stream < primitive.vbs.size(); ++stream) {
            primitive.vbs[stream]->bind((uint8_t)stream);
        }
        primitive.ib->bind();

        // model matrix, once per submit
        bgfx::setTransform(&primitive.transform[0][0]);
        glm::mat4 model_inv_t = glm::transpose(glm::inverse(primitive.transform));
        set_uniform("u_model_inv_t", &model_inv_t, bgfx::UniformType::Mat4);

        // lighting
        uint16_t light_count = _lights.size();
        glm::vec4 vec4_light_count = glm::vec4((float)light_count);
        set_uniform("u_light_count", &vec4_light_count, bgfx::UniformType::Vec4);
        std::vector<glm::vec4> arr_light_pos;
        std::vector<glm::vec4> arr_light_color;
        for (auto& light : _lights) {
            arr_light_pos.emplace_back(glm::vec4(light.direction, 0.0f));
            arr_light_color.emplace_back(light.color * light.intensity, 1.0f);
        }
        set_uniform("u_light_dirs", arr_light_pos.data(), bgfx::UniformType::Vec4, light_count);
        set_uniform("u_light_colors", arr_light_color.data(), bgfx::UniformType::Vec4, light_count);
        glm::vec4 vec4_view_pos = glm::vec4(_camera.eye, 1.0f);
        set_uniform("u_view_pos", &vec4_view_pos, bgfx::UniformType::Vec4);
        
        // materials
        Renderer::Material& material = primitive.material;
        glm::vec4 vec4_albedo = glm::vec4(material.albedo, 1.0f);
        set_uniform("u_albedo", &vec4_albedo, bgfx::UniformType::Vec4);
        glm::vec4 vec4_mra = glm::vec4(material.metallic, material.roughness, material.ao, 1.0f);
        set_uniform("u_metallic_roughness_ao", &vec4_mra, bgfx::UniformType::Vec4);

        bgfx::setState(opaque_state);
        bgfx::submit(opaque_id, _shaders[material.shader_name]);
    }
}

void Renderer::reset(uint16_t width, uint16_t height) {
    bgfx::setViewClear(opaque_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0f0f0fff);
    bgfx::setViewRect(opaque_id, 0, 0, width, height);
    // bgfx::setViewFrameBuffer(opaque_id, BGFX_INVALID_HANDLE);
}

void Renderer::set_uniform(const std::string& name,
                           void* value,
                           bgfx::UniformType::Enum type,
                           uint16_t num) {
    auto iter = _uniforms.find(name);
    bgfx::UniformHandle u_hdl;

    if (iter != _uniforms.end()) {
        u_hdl = iter->second;
    } else {
        // make a new one
        u_hdl = bgfx::createUniform(name.c_str(), type, num);
        _uniforms[name] = u_hdl;
    }

    bgfx::setUniform(u_hdl, value, num);
}

Renderer::Primitive::IndexBufferHandle::~IndexBufferHandle() {
    if (bgfx::isValid(dyn_hdl)) {
        bgfx::destroy(dyn_hdl);
    }
    if (bgfx::isValid(sta_hdl)) {
        bgfx::destroy(sta_hdl);
    }
}

void Renderer::Primitive::IndexBufferHandle::bind() {
    if (bgfx::isValid(dyn_hdl)) {
        bgfx::setIndexBuffer(dyn_hdl);
    }
    if (bgfx::isValid(sta_hdl)) {
        bgfx::setIndexBuffer(sta_hdl);
    }
}

Renderer::Primitive::VertexBufferHandle::~VertexBufferHandle() {
    if (bgfx::isValid(dyn_hdl)) {
        bgfx::destroy(dyn_hdl);
    }
    if (bgfx::isValid(sta_hdl)) {
        bgfx::destroy(sta_hdl);
    }
}

void Renderer::Primitive::VertexBufferHandle::bind(uint8_t stream) {
    if (bgfx::isValid(dyn_hdl)) {
        bgfx::setVertexBuffer(stream, dyn_hdl);
    }
    if (bgfx::isValid(sta_hdl)) {
        bgfx::setVertexBuffer(stream, sta_hdl);
    }
}

std::shared_ptr<Renderer::Primitive::VertexBufferHandle>
Renderer::Primitive::add_vertex_buffer(VertexDesc desc) {
    bgfx::VertexLayout layout;
    layout.begin().add(desc.attrib, desc.num, desc.type).end();

    vbs.emplace_back(new VertexBufferHandle);
    auto& vb = vbs.back();
    if (desc.is_dynamic) {
        vb->dyn_hdl = bgfx::createDynamicVertexBuffer(bgfx::copy(desc.data, desc.size), layout);
    } else {
        vb->sta_hdl = bgfx::createVertexBuffer(bgfx::copy(desc.data, desc.size), layout);
    }

    vb->attrib = desc.attrib;

    return vb;
}

std::shared_ptr<Renderer::Primitive::IndexBufferHandle>
Renderer::Primitive::set_index_buffer(IndexDesc desc) {
    ib.reset(new IndexBufferHandle);
    if (desc.is_dynamic) {
        ib->dyn_hdl = bgfx::createDynamicIndexBuffer(bgfx::copy(desc.data, desc.size));
    } else {
        ib->sta_hdl = bgfx::createIndexBuffer(bgfx::copy(desc.data, desc.size));
    }

    return ib;
}

void Renderer::Primitive::set_transform(glm::mat4 transform) {
    this->transform = std::move(transform);
}


void Renderer::Primitive::set_material(Material material) {
    this->material = std::move(material);
}

}