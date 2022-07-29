#pragma once

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "common/id_allocator.hpp"

class RayCaster {
public:
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    // Sphere-shaped envelope surface
    struct Sphere {
        glm::vec3   center;
        float       radius;
    };

    template<typename T>
    struct BufferDesc {
        const T*     data;
        uint32_t     count;
    };

    struct TriMeshDesc {
        BufferDesc<float>       p_desc;
        BufferDesc<float>       n_desc;
        BufferDesc<uint16_t>    i_desc;
        glm::mat4               transform;
    };

    struct RayTriManifold {
        bool                        intersect   = false;
        size_t                      mesh_id;
        std::array<uint16_t, 3>     triangle_ids;
        float                       t_ray;
        glm::vec3                   bari_triangle;
        glm::vec3                   normal;
    };

    size_t add_tri_mesh(TriMeshDesc desc);

    void remove_tri_mesh(size_t id);

    void update_vertex_position(size_t id, BufferDesc<float> p_desc, BufferDesc<float> n_desc);

    void update_transform(size_t id, glm::mat4 transform);

    // TriMesh& mesh(size_t id) { return _meshes.at(id); }; 
    // const TriMesh mesh(size_t id) const { return _meshes.at(id); };

    RayTriManifold intersect(Ray r);

    std::set<uint16_t> envelope(size_t id, Sphere s); 

private:
    struct AABB {
        glm::vec3   min;
        glm::vec3   max;
    };

    struct TriMesh {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<uint16_t>  indices;
        glm::mat4              transform;

        AABB                   mesh_aabb;
        std::vector<AABB>      triangle_aabbs;
    };

    void recompute_bounding_volumes(TriMesh& mesh);

    bool intersect_aabb(const Ray& r,
                        const AABB& aabb,
                        glm::vec2& t);

    bool intersect_triangle(const Ray& r,
                            glm::vec3 p0,
                            glm::vec3 p1,
                            glm::vec3 p2,
                            float& t,
                            glm::vec3& bari);

    float distance2(glm::vec3 p, AABB aabb);

    std::map<size_t, TriMesh> _meshes;
    IdAllocator               _id_alloc;
};