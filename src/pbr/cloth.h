#pragma once

#include <array>
#include <map>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat2x2.hpp>

namespace phy {

class Cloth {
public:
    using Idx2 = std::array<size_t, 2>;
    using Idx3 = std::array<size_t, 3>;
    using Idx3x2 = std::array<Idx2, 3>;

    template<typename T>
    class Arr2D : public std::vector<std::vector<T>> {
    public:
        T& operator[](Idx2 i) {
            return (*(this->begin() + i[0]))[i[1]];
        }
    };

    template<typename T>
    using Arr1D = std::vector<T>;

    // TODO: add mass
    // TODO: add physical parameters
    struct State {
        // mandatory for initialization
        Arr2D<glm::vec3>    pos;
        Arr2D<glm::vec2>    tex;
        Arr1D<Idx3x2>       indices;
        Arr1D<Idx2>         kinematic_ids;
        glm::vec3           gravity = glm::vec3(0.0f, -10.0f, 0.0f);

        // re-computed every step
        Arr1D<glm::vec3>    kinematic_pos;
        Arr2D<glm::vec3>    norm;
        Arr2D<glm::vec3>    tangent;
        Arr2D<glm::vec3>    prev_pos;
        Arr2D<glm::vec3>    delta_pos;
    };

    State state;

    void init(const Arr1D<float>& vb,
              const Arr1D<uint16_t>& ib,
              const Arr1D<uint16_t>& kinematic_ids,
              Idx2 dims, // rows vs columns
              size_t vertex_stride,
              size_t pos_offset,
              size_t tex_offset);

    void update_kinematics(const Arr1D<glm::vec3>& k_pos);

    // in seconds
    void update(float dt);

    void copy_back(Arr1D<float>::iterator vb_beg,
                   size_t vertex_stride,
                   size_t pos_offset,
                   size_t norm_offset,
                   size_t tangent_offset,
                   bool inverse_norm,
                   bool inverse_bitangent);

private:
    void compute_tangent_norm();

    void verlet(float dt);

    void apply_constraint();

    void self_collision_p2p_test();

    template<typename T>
    void zero_arr2d(Arr2D<T>& arr) {
        for (auto& row : arr) {
            row.assign(row.size(), T(0));
        }
    }
    
    struct Constraint {
        Idx2 i0;
        Idx2 i1;
        float rest_len;
    };

    struct CollisionP2PManifold {
        Idx2 i0;
        Idx2 i1;
        glm::vec3 n;
    };

    Arr1D<Constraint> _constraints;
    Arr1D<CollisionP2PManifold> _p2p_manifolds;
    // Arr2D<glm::mat2> _uv_inverse_mat;
    Arr2D<glm::vec3> _normal_buf;
    Arr2D<glm::vec3> _tangent_buf;
    Idx2 _dims;

    // TODO: temporary, for point to point collision
    const float _r_sc = 0.24f; // static collision radius, faster the object, larger the static radius
    const float _r = 0.12f;    // particle radius
};

}