#include <sstream>
#include <string.h>

#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
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
const uint16_t shadow_res = 2048;    // corresponding parameter in shader
const glm::vec3 shadow_dims(10.0f, 10.0f, 15.0f); // width, height, depth of shadow ortho projection
const int cascade_num = 3;
const float cascade_correction = 0.85;

// 1 shadow pass for each light
const bgfx::ViewId directional_shadow_id = 0;
const uint64_t shadow_state = 0
	| BGFX_STATE_WRITE_Z
	| BGFX_STATE_DEPTH_TEST_LESS
	| BGFX_STATE_CULL_CCW;

// opaque primitive pass
const bgfx::ViewId opaque_id = directional_shadow_id + max_light_count * cascade_num;
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
                                                          shadow_res * cascade_num,
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

inline std::string make_key(const std::string& prefix, size_t id0, size_t id1) {
    std::ostringstream oss;
    oss << prefix << id0 << "_" << id1;
    return oss.str();
}

void Renderer::render_shadowmaps() {
    uint16_t atlas_id = 0;
    for (auto light_iter = _lights.begin(); light_iter != _lights.end(); ++light_iter, ++atlas_id) {
        // atlas id and light id are 2 different id systems
        size_t light_id = light_iter->first;
        DirectionalLight& light = light_iter->second;

        for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
            bgfx::ViewId view_id = directional_shadow_id + atlas_id * cascade_num + cascade_id;
            bgfx::FrameBufferHandle fbo = _fbos[make_key("shadowmap_", light_id, cascade_id)];
            bgfx::setViewTransform(view_id, &light.view[0][0], &light.orthos[cascade_id][0][0]);
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
}

void Renderer::blit_shadowmap_atlas(bgfx::ViewId target_view_id) {
    int atlas_id = 0;
    for (auto& ele : _lights) {
        size_t light_id = ele.first;
        for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
            bgfx::blit(target_view_id,
                       _textures["shadowmaps_atlas"],
                       shadow_res * atlas_id,
                       shadow_res * cascade_id,
                       bgfx::getTexture(_fbos[make_key("shadowmap_", light_id, cascade_id)], 0));
        }
        ++atlas_id;
    }
}

void Renderer::render_opaque() {
    // set view transform once per view
    bgfx::setViewTransform(opaque_id, &_camera.view[0][0], &_camera.proj[0][0]);
    bgfx::setViewFrameBuffer(opaque_id, _fbos["main"]);

    // touch in case there is no opaque primitives
    bgfx::touch(opaque_id);
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

    // cascaded shadowmaps
    std::vector<glm::mat4> arr_light_view_proj;
    for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
        for (auto& ele : _lights) {
            DirectionalLight& light = ele.second;
            arr_light_view_proj.push_back(light.orthos[cascade_id] * light.view);
        }
    }
    glm::vec4 cascade_thresholds(1.0f);
    for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
        assert(cascade_id <= 3);
        cascade_thresholds[cascade_id] = _camera.frust.slices_ndc_z[cascade_id].near.z;
    }
    set_uniform("u_cascade_thresholds_ndc", &cascade_thresholds, bgfx::UniformType::Vec4);
    set_uniform("u_light_space_view_proj", arr_light_view_proj.data(), bgfx::UniformType::Mat4, light_count * cascade_num);
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
    if (desc.attrib == bgfx::Attrib::Enum::Position) {
        // construct aabb based on position
        assert(desc.type == bgfx::AttribType::Float && desc.num == 3);
        const glm::vec3* start = (const glm::vec3*)desc.data;
        const glm::vec3* end = (const glm::vec3*)((const char*)desc.data + desc.size);
        
        for (const glm::vec3* ptr = start; ptr < end; ++ptr) {
            aabb.add(*ptr);
        }
    }

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

    for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
        bgfx::TextureHandle depth_attatchment = bgfx::createTexture2D(shadow_res,
                                                                      shadow_res,
                                                                      false,
                                                                      1,
                                                                      bgfx::TextureFormat::D16F,
                                                                      BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP);
        _fbos[make_key("shadowmap_", id, cascade_id)] = bgfx::createFrameBuffer(1, &depth_attatchment, true);   
    }

    return id;
}

void Renderer::remove_light(size_t id) {
    _lights.erase(id);
    _id_alloc.deallocate(id); 

    for (int cascade_id = 0; cascade_id < cascade_num; ++cascade_id) {
        std::string key = make_key("shadowmap_", id, cascade_id);
        bgfx::destroy(_fbos[key]);
        _fbos.erase(key);
    }
}

void Renderer::recompute_camera() {
    _camera.proj = glm::perspective(_camera.fovy, _camera.width / _camera.height, _camera.near, _camera.far);
	_camera.view = glm::lookAt(_camera.eye, _camera.eye + _camera.front, _camera.up);
    _camera.inv_proj = glm::inverse(_camera.proj);
    _camera.inv_view = glm::inverse(_camera.view);
    compute_view_frust_slices(cascade_num);
}

void Renderer::recompute_lights() {
    compute_scene_aabb();
    for (auto& ele : _lights) {
        size_t light_id = ele.first;
        compute_cascaded_light_properties(light_id);
    }
}

void Renderer::compute_scene_aabb() {
    for (auto& ele : _primitives) {
        const Primitive& prim = ele.second;
        _scene_aabb.merge(prim.aabb.transformed_bound(prim.transform));
    }
}

void Renderer::compute_cascaded_light_properties(size_t id) {
    glm::vec3 dir = _lights[id].direction;
    glm::mat4& light_view = _lights[id].view;
    std::vector<glm::mat4>& orthos = _lights[id].orthos;

    light_view = glm::lookAt(glm::vec3(0.0),
                             glm::vec3(0.0f) + dir,
                             glm::vec3(0.0f, 1.0f, 0.0f)); // TODO: what if light shines straight up

    // determine near and far plane of orthogonal transform, so that
    // 1. the entire view frustum is in sight
    // 2. the entire scene is in sight
    float min_proj = std::numeric_limits<float>::max();
    float max_proj = std::numeric_limits<float>::min();
    // view frustum
    std::vector<glm::vec3> view_corners = std::move(_camera.frust.view_world.corners());
    for (auto& c : view_corners) {
        float proj = glm::dot(dir, c);
        min_proj = glm::min(min_proj, proj);
        max_proj = glm::max(max_proj, proj);
    }
    // scene
    std::vector<glm::vec3> scene_corners = std::move(_scene_aabb.corners());
    for (auto& c : scene_corners) {
        float proj = glm::dot(dir, c);
        min_proj = glm::min(min_proj, proj);
        max_proj = glm::max(max_proj, proj);
    }
    min_proj /= glm::length(dir);
    max_proj /= glm::length(dir);
    
    // determin left/right/up/bottom boundaries of orthogonal transform, so that
    // view frustum bounding sphere is bound inside a light space cubic aabb
    orthos.clear();
    for (auto bs : _camera.frust.slices_bs_world) {
        //GeometryUtil::Sphere slice_
        bs.center = glm::vec3(light_view * glm::vec4(bs.center, 1.0f));
        
        // texel snapping on xy
        float length_per_texel = bs.radius / (shadow_res * 0.5f);
        glm::mat2 snap(1.0f / length_per_texel);
        glm::mat2 inv_snap(length_per_texel);
        glm::vec2 center_xy = inv_snap * glm::floor(snap * glm::vec2(bs.center));
        bs.center = glm::vec3(center_xy.x, center_xy.y, bs.center.z);

        orthos.push_back(glm::ortho(bs.center.x - bs.radius,  bs.center.x + bs.radius,
                                    bs.center.y - bs.radius,  bs.center.y + bs.radius,
                                    min_proj,                 max_proj));
    }

}

void Renderer::compute_view_frust_slices(int num) {
    // view frustum in world space
    // near - top - right
    glm::vec4 ntr = _camera.inv_proj * glm::vec4(1.0f, 1.0f, -1.0f, 1.0f);
    ntr /= glm::vec4(ntr.w);
    _camera.frust.view_world.near = glm::vec3(ntr.x, ntr.y, ntr.z);
    // far - top - right
    glm::vec4 ftr = _camera.inv_proj * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ftr /= glm::vec4(ftr.w);
    _camera.frust.view_world.far = glm::vec3(ftr.x, ftr.y, ftr.z);
    _camera.frust.view_world.trans = _camera.inv_view;

    // slices
    std::vector<float> split_z;
    float n = -_camera.near;
    float f = -_camera.far;
    split_z.push_back(n);
    for (int i = 1; i < num; ++i) {
        // TODO: see Nvidia cascaded shadow maps for details
        float lambda = 0.85f;
        split_z.push_back(lambda * n * glm::pow(f / n, float(i) / float(num)) + 
                         (1 - lambda) * (n + (float(i) / float(num)) * (f - n)));
    }
    split_z.push_back(f);

    // set slices' bounding sphere in world space
    glm::vec3 frust_near = _camera.frust.view_world.near;
    glm::vec3 frust_far = _camera.frust.view_world.far;
    std::vector<glm::vec3> slices_view;
    for (float z : split_z) {
        glm::vec3 slice = frust_near +
                          (frust_far - frust_near) *
                          (z - n) / (f - n);
        slices_view.push_back(slice);
    }
    _camera.frust.slices_bs_world.clear();
    for (size_t i = 0; i < num; ++i) {
        GeometryUtil::Frustum slice_frust = {slices_view[i], slices_view[i + 1]};
        std::vector<glm::vec3> slice_corners = std::move(slice_frust.corners());
        GeometryUtil::AABB aabb;
        for (auto& c : slice_corners) {
            aabb.add(glm::vec3(glm::vec4(c, 1.0f)));
        }
        GeometryUtil::Sphere bs = std::move(aabb.bounding_sphere());
        bs.center = glm::vec3(_camera.inv_view * glm::vec4(bs.center, 1.0f));
        _camera.frust.slices_bs_world.push_back(bs);
    }

    // set slices in ndc
    std::vector<glm::vec3> slices_ndc;
    for (glm::vec3 slice : slices_view) {
        // only z matters
        glm::vec4 slice_ndc = _camera.proj * glm::vec4(0.0f, 0.0f, slice.z, 1.0f);
        slice_ndc /= slice_ndc.w;
        slices_ndc.push_back(glm::vec3(slice_ndc));
    }
    _camera.frust.slices_ndc_z.clear();
    for (size_t i = 0; i < num; ++i) {
        _camera.frust.slices_ndc_z.push_back({slices_ndc[i], slices_ndc[i + 1]});
    }
}

}