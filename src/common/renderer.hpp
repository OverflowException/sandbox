#pragma once

#include <memory>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

#include "bgfx/bgfx.h"
#include "id_allocator.hpp"

namespace rdr {

class Renderer {
public:
    Renderer();

    ~Renderer();

    struct Material {
        glm::vec3       albedo      = glm::vec3(1.0f);
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
    };

    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
        float     intensity;
    };
    
    // primitive operations
    size_t add_primitive(Primitive prim) {
        size_t id = _id_alloc.allocate();
        _primitives[id] = std::move(prim);
        return id;
    };

    void remove_primitive(size_t id) {
        _primitives.erase(id);
        _id_alloc.deallocate(id);
    };

    Primitive& primitive(size_t id) { return _primitives.at(id); };
    const Primitive& primitive(size_t id) const { return _primitives.at(id); };

    // std::vector<Primitive>& primitives() { return _primitives; };
    // const std::vector<Primitive>& primitives() const { return _primitives; };

    std::vector<PointLight>& lights() { return _lights; };
    const std::vector<PointLight>& lights() const { return _lights; };

    Camera& camera() { return _camera; };
    const Camera& camera() const { return _camera; };

    void set_uniform(const std::string& name,
                     void* value,
                     bgfx::UniformType::Enum type,
                     uint16_t num = (uint16_t)1);

    void render();

    void reset(uint16_t width, uint16_t height);

private:

    Camera                                        _camera;
    std::vector<PointLight>                       _lights;
    // std::vector<Primitive>                        _primitives;
    std::map<size_t, Primitive>                   _primitives;
    std::map<std::string, bgfx::ProgramHandle>    _shaders;
    std::map<std::string, bgfx::UniformHandle>    _uniforms;
    IdAllocator                                   _id_alloc;
};

}