#include <algorithm>
#include <cstring>
#include <iostream>
#include "ray_caster.h"

size_t RayCaster::add_tri_mesh(TriMeshDesc desc) {
    size_t id = _id_alloc.allocate();

    TriMesh mesh;
    mesh.positions.resize(desc.p_desc.count / 3);
    std::memcpy(mesh.positions.data(),
                desc.p_desc.data,
                desc.p_desc.count * sizeof(float));
    mesh.normals.resize(desc.n_desc.count / 3);
    std::memcpy(mesh.normals.data(),
                desc.n_desc.data,
                desc.n_desc.count * sizeof(float));
    mesh.indices.resize(desc.i_desc.count);
    std::memcpy(mesh.indices.data(),
                desc.i_desc.data,
                desc.i_desc.count * sizeof(uint16_t));
    mesh.transform = desc.transform;
    recompute_bounding_volumes(mesh);
    _meshes[id] = std::move(mesh);
    
    return id;
}

void RayCaster::recompute_bounding_volumes(TriMesh& mesh) {
    glm::vec3 triangle_min(std::numeric_limits<float>::max());
    glm::vec3 triangle_max(std::numeric_limits<float>::lowest());
    glm::vec3 mesh_min = triangle_min;
    glm::vec3 mesh_max = triangle_max;
    for (size_t i = 0; i < mesh.indices.size();) {
        uint16_t i0 = mesh.indices[i++];
        uint16_t i1 = mesh.indices[i++];
        uint16_t i2 = mesh.indices[i++];
        glm::vec3& p0 = mesh.positions[i0];
        glm::vec3& p1 = mesh.positions[i1];
        glm::vec3& p2 = mesh.positions[i2];

        triangle_min = glm::min(p0, glm::min(p1, p2));
        triangle_max = glm::max(p0, glm::max(p1, p2));
        mesh.triangle_aabbs.push_back({triangle_min, triangle_max});

        mesh_min = glm::min(triangle_min, mesh_min);
        mesh_max = glm::max(triangle_max, mesh_max);
    }

    mesh.mesh_aabb = {mesh_min, mesh_max};
}

void RayCaster::update_transform(size_t id, glm::mat4 transform) {
    _meshes[id].transform = std::move(transform);
}

void RayCaster::remove_tri_mesh(size_t id) {
    _meshes.erase(id);
    _id_alloc.deallocate(id);
}

RayCaster::RayTriManifold RayCaster::intersect(Ray r) {
    std::vector<RayTriManifold> mans;
    for (auto& ele : _meshes) {
        size_t mesh_id = ele.first;
        TriMesh& mesh = ele.second;

        // transform ray into mesh's local space
        glm::mat4 transform_inv = glm::inverse(mesh.transform);
        Ray r_local = {
            .origin = transform_inv * glm::vec4(r.origin, 1.0f),
            .direction = transform_inv * glm::vec4(r.direction, 0.0f),
        };

        glm::vec2 t_ray_aabb(0.0f);
        // try mesh aabb
        if (!intersect_aabb(r_local, mesh.mesh_aabb, t_ray_aabb)) {
            continue;
        }
        for (size_t i = 0; i < mesh.triangle_aabbs.size(); ++i) {
            // try triangle aabb
            if (!intersect_aabb(r_local, mesh.triangle_aabbs[i], t_ray_aabb)) {
                continue;
            }
            glm::vec3 tri_bari(0.0f);
            uint16_t i0 = mesh.indices[i * 3];
            uint16_t i1 = mesh.indices[i * 3 + 1];
            uint16_t i2 = mesh.indices[i * 3 + 2];
            glm::vec3& p0 = mesh.positions[i0];
            glm::vec3& p1 = mesh.positions[i1];
            glm::vec3& p2 = mesh.positions[i2];
            float t_ray_tri;
            // try triangle
            if (!intersect_triangle(r_local, p0, p1, p2, t_ray_tri, tri_bari)) {
                continue;
            }
            RayTriManifold man = {
                .intersect = true,
                .mesh_id = mesh_id,
                .triangle_ids = {i0, i1, i2},
                .t_ray = t_ray_tri,
                .bari_triangle = tri_bari,
                .normal = glm::mat3(glm::inverse(glm::transpose(mesh.transform))) *
                          (mesh.normals[i0] * tri_bari[0] +
                           mesh.normals[i1] * tri_bari[1] +
                           mesh.normals[i2] * tri_bari[2]),
            };
            mans.push_back(man);
        }
    }

    if (mans.empty()) {
        // no intersetion
        return RayTriManifold{.intersect = false,};
    } else {
        // return nearest
        return *std::min_element(mans.begin(), mans.end(),
                                 [&](RayTriManifold& m0, RayTriManifold& m1){
                                     return m0.t_ray < m1.t_ray;
                                 });
    }
}

std::set<uint16_t> RayCaster::envelope(size_t id, Sphere s) {
    std::set<uint16_t> result_ids;
    TriMesh& mesh = _meshes[id];

    // transform sphere center into mesh's local space
    glm::mat4 transform_inv = glm::inverse(mesh.transform);
    Sphere s_local = {
        .center = transform_inv * glm::vec4(s.center, 1.0f),
        .radius = s.radius,
    };

    // for (uint16_t i : mesh.indices) {
    //     if (glm::distance(mesh.positions[i], s_local.center) <= s_local.radius) {
    //         result_ids.insert(i);
    //     }
    // }

    for (size_t i = 0; i < mesh.triangle_aabbs.size(); ++i) {
        // try triangle aabb
        if (distance2(s_local.center, mesh.triangle_aabbs[i]) > s_local.radius) {
            continue;
        }
        uint16_t i0 = mesh.indices[i * 3];
        uint16_t i1 = mesh.indices[i * 3 + 1];
        uint16_t i2 = mesh.indices[i * 3 + 2];
        glm::vec3& p0 = mesh.positions[i0];
        glm::vec3& p1 = mesh.positions[i1];
        glm::vec3& p2 = mesh.positions[i2];
        // try triangle
        if (glm::distance(p0, s_local.center) <= s_local.radius) {
            result_ids.insert(i0);
        }
        if (glm::distance(p1, s_local.center) <= s_local.radius) {
            result_ids.insert(i1);
        }
        if (glm::distance(p2, s_local.center) <= s_local.radius) {
            result_ids.insert(i2);
        }
    }

    return result_ids;
}


bool RayCaster::intersect_aabb(const Ray& r, const AABB& aabb, glm::vec2& t) {
    // TODO: edge conditions of ray direction
    // assert(r.direction.x != 0.0f);
    // assert(r.direction.y != 0.0f);
    // assert(r.direction.z != 0.0f);
    glm::vec3 t_min = (aabb.min - r.origin) / r.direction;
    glm::vec3 t_max = (aabb.max - r.origin) / r.direction;
    glm::vec3 t1 = glm::min(t_min, t_max);
    glm::vec3 t2 = glm::max(t_min, t_max);
    float t_near = glm::max(glm::max(t1.x, t1.y), t1.z);
    float t_far = glm::min(glm::min(t2.x, t2.y), t2.z);
    t = {t_near, t_far};
    return t_near <= t_far && (t_near >= 0.0f || t_far >= 0.0f);
}

bool RayCaster::intersect_triangle(const Ray& r,
                                   glm::vec3 p0,
                                   glm::vec3 p1,
                                   glm::vec3 p2,
                                   float& t,
                                   glm::vec3& bari) {
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;
    glm::vec3 h = cross(r.direction, e2);
    float a = dot(e1, h);
    if (a < 0.0001f && a > -0.0001f) {
        // ray parallel to triangle
        return false;
    }
    float f = 1.0f / a;
    glm::vec3 s = r.origin - p0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    glm::vec3 q = cross(s, e1);
    float v = f * dot(r.direction, q);
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // compute intersection
    t = f * dot(e2, q);
    if (t < 0.0f) {
        // line intersection, not ray intersction
        return false;
    }

    bari[0] = 1.0f - u - v;
    bari[1] = u;
    bari[2] = v;

    return true;
}

float RayCaster::distance2(glm::vec3 p, AABB aabb) {
    float dist2 = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float v = p[i];
        if (v < aabb.min[i]) {
            dist2 += (v - aabb.min[i]) * (v- aabb.min[i]);
        }
        if (v > aabb.max[i]) {
            dist2 += (v - aabb.max[i]) * (v - aabb.max[i]);
        }
    }
    return dist2;
}