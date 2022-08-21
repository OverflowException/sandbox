#pragma once

#include <memory>
#include <map>
#include <vector>
#include <list>
#include <limits>
#include <string>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

#include "bgfx/bgfx.h"
#include "id_allocator.hpp"
#include "geometry_utils.hpp"

namespace rdr {

class Renderer {
public:
    Renderer();

    ~Renderer();

    struct Material {
        glm::vec4       albedo      = glm::vec4(1.0f);
        float           metallic    = 0.1f;
        float           roughness   = 0.1f;
        float           ao          = 0.1f;
        std::string     shader_name = "";
    };

    struct Primitive {
        struct VertexBufferHandle {
            ~VertexBufferHandle();

            void bind(uint8_t stream);

            bgfx::DynamicVertexBufferHandle dyn_hdl = BGFX_INVALID_HANDLE;
            bgfx::VertexBufferHandle        sta_hdl = BGFX_INVALID_HANDLE;

            bgfx::Attrib::Enum              attrib;
        };

        struct IndexBufferHandle {
            ~IndexBufferHandle();

            void bind();

            bgfx::DynamicIndexBufferHandle  dyn_hdl = BGFX_INVALID_HANDLE;
            bgfx::IndexBufferHandle         sta_hdl = BGFX_INVALID_HANDLE;
        };

        struct VertexDesc {
            const void*             data;
            uint32_t                size;
            bgfx::Attrib::Enum      attrib;
            uint8_t                 num;
            bgfx::AttribType::Enum  type;
            bool                    is_dynamic = false;
        };

        struct IndexDesc {
            const void*             data;
            uint32_t                size;
            bool                    is_dynamic;
        };


        std::shared_ptr<VertexBufferHandle> add_vertex_buffer(VertexDesc desc);

        std::shared_ptr<IndexBufferHandle> set_index_buffer(IndexDesc desc);

        void set_transform(glm::mat4 transform);

        void set_material(Material material);

        std::vector<std::shared_ptr<VertexBufferHandle>>         vbs;
        std::shared_ptr<IndexBufferHandle>                       ib;

        glm::mat4                               transform;
        Material                                material;
        GeometryUtil::AABB                      aabb;
    };

    struct ViewFrustumCtx {
        GeometryUtil::Frustum               view_world;         // entirety of view frustum, world space
        std::vector<GeometryUtil::Sphere>   slices_bs_world;    // slices' bounding sphere, world space
        std::vector<GeometryUtil::Frustum>  slices_ndc_z;       // frustum slices in ndc, only z values
    };

    struct Camera {
        glm::vec3 eye;
        glm::vec3 front;
        glm::vec3 up;
        
        float width;
        float height;
        float fovy;
        float near;
        float far;

        // will get recomputed by renderer every render
        glm::mat4 proj = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 inv_proj = glm::mat4(1.0f);
        glm::mat4 inv_view = glm::mat4(1.0f);
        ViewFrustumCtx frust; // in world space
    };

    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
        float     intensity;

        // will get recomputed by renderer every render
        glm::mat4               view = glm::mat4(1.0f);
        std::vector<glm::mat4>  orthos;
    };
    
    // primitive operations
    size_t add_primitive(Primitive prim);

    void remove_primitive(size_t id);

    Primitive& primitive(size_t id) { return _primitives.at(id); }
    const Primitive& primitive(size_t id) const { return _primitives.at(id); }

    // light operations
    size_t add_light(DirectionalLight light);

    void remove_light(size_t id);

    DirectionalLight& light(size_t id) { return _lights.at(id); };
    const DirectionalLight& light(size_t id) const { return _lights.at(id); };

    Camera& camera() { return _camera; };
    const Camera& camera() const { return _camera; };

    void set_uniform(const std::string& name,
                     const void* value,
                     bgfx::UniformType::Enum type,
                     uint16_t num = (uint16_t)1);

    void recompute_camera();

    void recompute_lights();

    void render();

    void render_shadowmaps();

    void blit_shadowmap_atlas(bgfx::ViewId target_view_id);

    void render_opaque();

    bool render_translucent();

    void composite_opaque_translucent();

    void render_tonemapping();

    void submit_lighting(const Primitive& prim,
                         bgfx::ViewId view_id,
                         uint64_t state,
                         uint32_t factor = 0);

    void compute_view_frust_slices(int n);

    void compute_cascaded_light_properties(size_t light_id);

    void compute_scene_aabb();

    void reset(uint16_t width, uint16_t height);

private:
    Camera                                        _camera;
    std::map<size_t, DirectionalLight>            _lights;
    std::map<size_t, Primitive>                   _primitives;
    GeometryUtil::AABB                            _scene_aabb;

    std::map<std::string, bgfx::ProgramHandle>    _shaders;
    std::map<std::string, bgfx::UniformHandle>    _uniforms;
    std::map<std::string, bgfx::TextureHandle>    _textures;
    std::map<std::string, bgfx::FrameBufferHandle> _fbos;

    IdAllocator                                   _id_alloc;
};

}