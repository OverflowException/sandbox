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
    
    using Constraints = std::pair<size_t, size_t>;

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
        Arr2D<glm::vec3>    norm;
        Arr2D<glm::vec3>    tangent;
        Arr2D<glm::vec3>    prev_pos;
    };

    State state;

    void init(const Arr1D<float>& vb,
              const Arr1D<uint16_t>& ib,
              Idx2 dims, // rows vs columns
              size_t vertex_stride,
              size_t pos_offset,
              size_t tex_offset);

    // in milliseconds
    void update(float dt);

    void copy_back(Arr1D<float>::iterator vb_beg,
                   size_t vertex_stride,
                   size_t pos_offset,
                   size_t norm_offset,
                   size_t tangent_offset,
                   bool reverse_norm = false,
                   bool reverse_tangent = false);

private:
    void compute_tangent_norm();

    template<typename T>
    void clear_arr2d(Arr2D<T>& arr) {
        for (auto& row : arr) {
            row.resize(row.size(), T(0.0f));
        }
    }
    
    struct Particle {
        glm::vec3 cur_pos;
        glm::vec3 prev_pos;
        glm::vec3 acc;
    };

    Arr1D<Constraints> _constraints;
    // Arr2D<glm::mat2> _uv_inverse_mat;
    Arr2D<glm::vec3> _normal_buf;
    Arr2D<glm::vec3> _tangent_buf;
    Idx2 _dims;
};

}