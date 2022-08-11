#include <sstream>
#include <string.h>

#include "glm/gtc/matrix_transform.hpp"
#include "procedural_shapes.h"
#include "renderer.hpp"
#include "file_io.h"

const std::string shader_base_path = "shaders/glsl/";
const std::vector<std::string> shader_names = {
    "pbr",
    "skybox",
    "shadowmap",
    "tonemapping",
    "oit_composite"
};

const int max_light_count = 4;
const uint16_t shadow_res = 1024;
const glm::vec3 shadow_dims(10.0f, 10.0f, 15.0f); // width, height, depth of shadow ortho projection

// 1 shadow pass for each light
const bgfx::ViewId directional_shadow_id = 0;
const uint64_t shadow_state = 0
	| BGFX_STATE_WRITE_Z
	| BGFX_STATE_DEPTH_TEST_LESS
	| BGFX_STATE_CULL_CCW;

// opaque primitive pass
const bgfx::ViewId opaque_id = directional_shadow_id + max_light_count;
const uint64_t opaque_state = 0
    | BGFX_STATE_WRITE_RGB
	| BGFX_STATE_WRITE_A
	| BGFX_STATE_WRITE_Z
	| BGFX_STATE_DEPTH_TEST_LESS
	| BGFX_STATE_CULL_CW;

// 2 passes of order independent transparency
const bgfx::ViewId oit_accum_id = opaque_id + 1;
const uint64_t oit_accum_state = 0
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_DEPTH_TEST_LESS
    | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) // RT_0 : O = S + D
    | BGFX_STATE_BLEND_INDEPENDENT; // indicates there's another render target
const uint32_t oit_accum_factor = 0
    | BGFX_STATE_BLEND_FUNC_RT_1(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_INV_SRC_ALPHA); // RT_1 = (1 - alpha_s) * D

const bgfx::ViewId oit_composite_id = oit_accum_id + 1;
const uint64_t oit_composite_state = 0
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_DEPTH_TEST_ALWAYS
    | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

// tonemapping id
const bgfx::ViewId tonemapping_id = oit_composite_id + 1;
const uint64_t tonemapping_state = 0
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_DEPTH_TEST_ALWAYS;

namespace rdr {

Renderer::Renderer() {
    // load all shaders
    for (auto& name : shader_names) {
        std::string vs_path = shader_base_path + name + "_vs.bin";
        std::string fs_path = shader_base_path + name + "_fs.bin";
        _shaders[name] = io::load_program(vs_path.c_str(), fs_path.c_str());
    }

    // create shadowmap atlas texture
    _textures["shadowmaps_atlas"] = bgfx::createTexture2D(shadow_res * max_light_count,
                                                          shadow_res,
                                                          false,
                                                          1,
                                                          bgfx::TextureFormat::D16F,
                                                          BGFX_TEXTURE_BLIT_DST |
                                                          BGFX_SAMPLER_UVW_CLAMP);
}

Renderer::~Renderer() {
    for (auto& ele : _shaders) {
        bgfx::destroy(ele.second);
    }

    for (auto& ele : _uniforms) {
        bgfx::destroy(ele.second);
    }

    for (auto& ele : _textures) {
        bgfx::destroy(ele.second);
    }

    for (auto& ele : _fbos) {
        bgfx::destroy(ele.second);
    }
}

inline std::string make_key(const std::string& prefix, size_t id) {
    std::ostringstream oss;
    oss << prefix << id;
    return oss.str();
}

void Renderer::render_shadowmaps() {
    uint16_t atlas_id = 0;
    for (auto light_iter = _lights.begin(); light_iter != _lights.end(); ++light_iter, ++atlas_id) {
        // atlas id and light id are 2 different id systems
        bgfx::ViewId view_id = directional_shadow_id + atlas_id;
        size_t light_id = light_iter->first;
        DirectionalLight& light = light_iter->second;
        bgfx::FrameBufferHandle fbo = _fbos[make_key("shadowmap_", light_id)];

        bgfx::setViewTransform(view_id, &light.view[0][0], &light.ortho[0][0]);
        bgfx::setViewClear(view_id, BGFX_CLEAR_DEPTH);
        bgfx::setViewRect(view_id, 0, 0, shadow_res, shadow_res);
        bgfx::setViewFrameBuffer(view_id, fbo);
        for (auto& ele : _primitives) {
            Primitive& primitive = ele.second;
            // submit position buffer only
            primitive.vbs[0]->bind((uint8_t)0);
            primitive.ib->bind();
            bgfx::setTransform(&primitive.transform[0][0]);
            bgfx::setState(shadow_state);
            bgfx::submit(view_id, _shaders["shadowmap"]);
        }
    }
}

void Renderer::blit_shadowmap_atlas(bgfx::ViewId target_view_id) {
    int atlas_id = 0;
    for (auto& ele : _lights) {
        size_t light_id = ele.first;
        bgfx::blit(target_view_id,
                   _textures["shadowmaps_atlas"],
                   shadow_res * atlas_id,
                   0,
                   bgfx::getTexture(_fbos[make_key("shadowmap_", light_id)], 0));
        ++atlas_id;
    }
}

void Renderer::render_opaque() {
    // set view transform once per view
    bgfx::setViewTransform(opaque_id, &_camera.view[0][0], &_camera.proj[0][0]);
    bgfx::setViewFrameBuffer(opaque_id, _fbos["main"]);

    for (auto& ele : _primitives) {
        Primitive& primitive = ele.second;
        if (primitive.material.albedo.a >= 1.0f) {
            // opqaue
            submit_lighting(primitive, opaque_id, opaque_state);
        }
    }
}

bool Renderer::render_translucent() {

    // set view transform once per view
    bgfx::setViewTransform(oit_accum_id, &_camera.view[0][0], &_camera.proj[0][0]);
    bgfx::setViewFrameBuffer(oit_accum_id, _fbos["oit"]);

    bool has_translucent_prim = false;
    for (auto& ele : _primitives) {
        Primitive& primitive = ele.second;
        if (primitive.material.albedo.a < 1.0f) {
            // translucent
            submit_lighting(primitive, oit_accum_id, oit_accum_state, oit_accum_factor);
            has_translucent_prim = true;
        }
    }

    return has_translucent_prim;
}

void bind_screen_quad() {
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::VertexLayout v_layout;
    v_layout.begin().add(bgfx::Attrib::Enum::Position, 3, bgfx::AttribType::Enum::Float).end();

    assert(bgfx::getAvailTransientVertexBuffer(4, v_layout));
    assert(bgfx::getAvailTransientIndexBuffer(6));
    bgfx::allocTransientVertexBuffer(&tvb, 4, v_layout);
    bgfx::allocTransientIndexBuffer(&tib, 6);

    std::vector<float> vb;
    std::vector<uint16_t> ib;
    ProceduralShapes::gen_z_quad(vb, ib, glm::vec2(1.0f, 1.0f));
    memcpy(tvb.data, vb.data(), sizeof(float) * 3 * 4);
    memcpy(tib.data, ib.data(), sizeof(uint16_t) * 6);

    bgfx::setVertexBuffer(0, &tvb, 0, 4);
    bgfx::setIndexBuffer(&tib, 0, 6);
}

void Renderer::composite_opaque_translucent() {
    bind_screen_quad();
    bgfx::TextureHandle accumu_tex = bgfx::getTexture(_fbos["oit"], 0);
    bgfx::TextureHandle revealage_tex = bgfx::getTexture(_fbos["oit"], 1);
    set_uniform("s_accumu", &accumu_tex, bgfx::UniformType::Sampler, 0);
    set_uniform("s_revealage", &revealage_tex, bgfx::UniformType::Sampler, 1);
    bgfx::setViewFrameBuffer(oit_composite_id, _fbos["main"]);
    bgfx::setState(oit_composite_state);
    bgfx::submit(oit_composite_id, _shaders["oit_composite"]);
}

void Renderer::render_tonemapping() {
    bind_screen_quad();
    bgfx::TextureHandle main_color_tex = bgfx::getTexture(_fbos["main"], 0);
    set_uniform("s_rt", &main_color_tex, bgfx::UniformType::Sampler, 0);
    bgfx::setViewFrameBuffer(tonemapping_id, BGFX_INVALID_HANDLE);
    bgfx::setState(tonemapping_state);
    bgfx::submit(tonemapping_id, _shaders["tonemapping"]);
}

void Renderer::render() {
    recompute_camera();
    recompute_lights();

    render_shadowmaps();

    // blit shadowmaps atlas
    // since bgfx claims: In Views, all draw commands are executed **after** blit and compute commands
    // blitting is done in opaque view, after shadowmaps are generated
    blit_shadowmap_atlas(opaque_id);

    render_opaque();

    if (render_translucent()) {
        composite_opaque_translucent();
    }

    render_tonemapping();
}

void Renderer::submit_lighting(const Primitive& prim,
                               bgfx::ViewId view_id,
                               uint64_t state,
                               uint32_t factor) {
    for (int stream = 0; stream < prim.vbs.size(); ++stream) {
        prim.vbs[stream]->bind((uint8_t)stream);
    }
    prim.ib->bind();

    // model matrix, once per submit
    bgfx::setTransform(&prim.transform[0][0]);
    glm::mat4 model_inv_t = glm::transpose(glm::inverse(prim.transform));
    set_uniform("u_model_inv_t", &model_inv_t, bgfx::UniformType::Mat4);

    // lights
    uint16_t light_count = _lights.size();
    glm::vec4 vec4_light_count = glm::vec4((float)light_count);
    set_uniform("u_light_count", &vec4_light_count, bgfx::UniformType::Vec4);
    std::vector<glm::vec4> arr_light_pos;
    std::vector<glm::vec4> arr_light_color;
    for (auto& ele : _lights) {
        DirectionalLight& light = ele.second;
        arr_light_pos.emplace_back(glm::vec4(light.direction, 0.0f));
        arr_light_color.emplace_back(light.color * light.intensity, 1.0f);
    }
    set_uniform("u_light_dirs", arr_light_pos.data(), bgfx::UniformType::Vec4, light_count);
    set_uniform("u_light_colors", arr_light_color.data(), bgfx::UniformType::Vec4, light_count);
    glm::vec4 vec4_view_pos = glm::vec4(_camera.eye, 1.0f);
    set_uniform("u_view_pos", &vec4_view_pos, bgfx::UniformType::Vec4);

    // shadowmaps
    std::vector<glm::mat4> arr_light_view_proj;
    for (auto& ele : _lights) {
        // TODO: this gets computed multiple times
        DirectionalLight& light = ele.second;
        arr_light_view_proj.push_back(light.ortho * light.view);
    }
    set_uniform("u_light_space_view_proj", arr_light_view_proj.data(), bgfx::UniformType::Mat4, light_count);
    set_uniform("s_shadowmaps_atlas", &_textures["shadowmaps_atlas"], bgfx::UniformType::Sampler, 0);

    // materials
    const Renderer::Material& material = prim.material;
    set_uniform("u_albedo", &material.albedo, bgfx::UniformType::Vec4);
    glm::vec4 vec4_mra = glm::vec4(material.metallic, material.roughness, material.ao, 1.0f);
    set_uniform("u_metallic_roughness_ao", &vec4_mra, bgfx::UniformType::Vec4);

    bgfx::setState(state, factor);
    bgfx::submit(view_id, _shaders[material.shader_name]);
}

void Renderer::reset(uint16_t width, uint16_t height) {
    // reset opaque view
    bgfx::setViewClear(opaque_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x2f2f2fff);
    bgfx::setViewRect(opaque_id, 0, 0, width, height);

    // reset tonemapping view
    bgfx::setViewClear(tonemapping_id, BGFX_CLEAR_COLOR, 0x00);
    bgfx::setViewRect(tonemapping_id, 0, 0, width, height);

    // reset oit accumulation view
    bgfx::setPaletteColor(0, 0.0f, 0.0f, 0.0f, 0.0f);
    bgfx::setPaletteColor(1, 1.0f, 1.0f, 1.0f, 1.0f);
    bgfx::setViewClear(oit_accum_id, BGFX_CLEAR_COLOR, 0.0f, 0, 0, 1);
    bgfx::setViewRect(oit_accum_id, 0, 0, width, height);
    // reset oit composition view
    // bgfx::setViewClear(oit_composite_id, BGFX_CLEAR_COLOR, 0.0f, 0, 1);
    bgfx::setViewRect(oit_composite_id, 0, 0, width, height);


    // create or reset main render target
    if (_fbos.find("main") != _fbos.end()) {
        bgfx::destroy(_fbos["main"]);
    }
    bgfx::TextureHandle main_attachments[2];
    main_attachments[0] = bgfx::createTexture2D(width, height,  // color
                                                false,
                                                1,
                                                bgfx::TextureFormat::RGBA16F,
                                                BGFX_TEXTURE_RT |
                                                BGFX_SAMPLER_UVW_CLAMP);
    main_attachments[1] = bgfx::createTexture2D(width, height,  // depth
                                                false,
                                                1,
                                                bgfx::TextureFormat::D16F,
                                                BGFX_TEXTURE_RT |
                                                BGFX_SAMPLER_UVW_CLAMP);
    _fbos["main"] = bgfx::createFrameBuffer(2, main_attachments, true);

    // create or reset oit target
    if (_fbos.find("oit") != _fbos.end()) {
        bgfx::destroy(_fbos["oit"]);
    }
    bgfx::TextureHandle oit_attachments[3];
    oit_attachments[0] = bgfx::createTexture2D(width, height,   // accumulation
                                               false,
                                               1,
                                               bgfx::TextureFormat::RGBA16F,
                                               BGFX_TEXTURE_RT |
                                               BGFX_SAMPLER_UVW_CLAMP);
    oit_attachments[1] = bgfx::createTexture2D(width, height,   // revealage
                                               false,
                                               1,
                                               bgfx::TextureFormat::R16F,
                                               BGFX_TEXTURE_RT |
                                               BGFX_SAMPLER_UVW_CLAMP);
    oit_attachments[2] = main_attachments[1];   // depth
    _fbos["oit"] = bgfx::createFrameBuffer(3, oit_attachments, true);
}

void Renderer::set_uniform(const std::string& name,
                           const void* value,
                           bgfx::UniformType::Enum type,
                           uint16_t num) {
    auto iter = _uniforms.find(name);
    bgfx::UniformHandle u_hdl;

    if (iter != _uniforms.end()) {
        u_hdl = iter->second;
    } else {
        // make a new one
        if (type == bgfx::UniformType::Sampler) {
            u_hdl = bgfx::createUniform(name.c_str(), type);
            _uniforms[name] = u_hdl;
        } else {
            u_hdl = bgfx::createUniform(name.c_str(), type, num);
            _uniforms[name] = u_hdl;
        }
    }

    if (type == bgfx::UniformType::Sampler) {
        bgfx::setTexture(num, u_hdl, *static_cast<const bgfx::TextureHandle*>(value));
    } else {
        bgfx::setUniform(u_hdl, value, num);
    }
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

size_t Renderer::add_primitive(Primitive prim) {
    size_t id = _id_alloc.allocate();
    _primitives[id] = std::move(prim);
    return id;
}

void Renderer::remove_primitive(size_t id) {
    _primitives.erase(id);
    _id_alloc.deallocate(id);
}

size_t Renderer::add_light(DirectionalLight light) {
    size_t id = _id_alloc.allocate();
    _lights[id] = std::move(light);

    bgfx::TextureHandle depth_attatchment = bgfx::createTexture2D(shadow_res,
                                                                  shadow_res,
                                                                  false,
                                                                  1,
                                                                  bgfx::TextureFormat::D16F,
                                                                  BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP);
    _fbos[make_key("shadowmap_", id)] = bgfx::createFrameBuffer(1, &depth_attatchment, true);

    return id;
}

void Renderer::remove_light(size_t id) {
    _lights.erase(id);
    _id_alloc.deallocate(id); 

    std::string key = make_key("shadowmap_", id);
    bgfx::destroy(_fbos[key]);
    _fbos.erase(key);
}

void Renderer::recompute_camera() {
    _camera.proj = glm::perspective(_camera.fovy, _camera.width / _camera.height, _camera.near, _camera.far);
	_camera.view = glm::lookAt(_camera.eye, _camera.eye + _camera.front, _camera.up);
}

void Renderer::recompute_lights() {
    for (auto& ele : _lights) {
        DirectionalLight& light = ele.second;
        float right = shadow_dims.x / 2.0f;
        float top = shadow_dims.y / 2.0f;
        float far = shadow_dims.z;
        light.ortho = glm::ortho(-right, right, -top, top, 0.0f, far);
        light.view = glm::lookAt(glm::vec3(0.0) - light.direction * 3.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }   
}

}