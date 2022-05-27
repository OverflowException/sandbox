#include "cloth.h"
#include "glm/trigonometric.hpp"
#include <cstring>

namespace phy {

void Cloth::init(const Arr1D<float>& vb,
                 const Arr1D<uint16_t>& ib,
                 const Arr1D<uint16_t>& kinematic_ids,
                 Idx2 dims, // rows vs columns
                 size_t vertex_stride,
                 size_t pos_offset,
                 size_t tex_offset) {
    vertex_stride /= sizeof(float);
    pos_offset /= sizeof(float);
    tex_offset /= sizeof(float);

    // construct position buffer
    Idx2 i;
    state.pos.resize(dims[0]);
    for (auto& row : state.pos) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    for (i[0] = 0; i[0] < dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < dims[1]; ++i[1]) {
            size_t vi = (i[1] + i[0] * dims[1]) * vertex_stride + pos_offset;
            state.pos[i] = glm::vec3(vb[vi], vb[vi + 1], vb[vi + 2]);
        }
    }
    
    // construct uv buffer
    state.tex.resize(dims[0]);
    for (auto& row : state.tex) {
        row.resize(dims[1], glm::vec2(0.0f));
    }

    for (i[0] = 0; i[0] < dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < dims[1]; ++i[1]) {
            size_t vi = (i[1] + i[0] * dims[1]) * vertex_stride + tex_offset;
            state.tex[i] = glm::vec3(vb[vi], vb[vi + 1], vb[vi + 2]);
        }
    }
    
    // construct index buffers
    state.indices.resize(ib.size() / 3);
    for (size_t ii = 0; ii < state.indices.size(); ++ii) {
        uint16_t i0 = ib[ii * 3];
        uint16_t i1 = ib[ii * 3 + 1];
        uint16_t i2 = ib[ii * 3 + 2];

        Idx2 iv0 = {i0 / dims[1], i0 % dims[1]};
        Idx2 iv1 = {i1 / dims[1], i1 % dims[1]};
        Idx2 iv2 = {i2 / dims[1], i2 % dims[1]};

        state.indices[ii] = {iv0, iv1, iv2};
    }

    // construct norm buffer
    state.norm.resize(dims[0]);
    for (auto& row : state.norm) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    // construct tangent buffer
    state.tangent.resize(dims[0]);
    for (auto& row : state.tangent) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    // construct prev_pos buffer
    state.prev_pos.resize(dims[0]);
    for (auto& row : state.prev_pos) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    for (i[0] = 0; i[0] < dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < dims[1]; ++i[1]) {
            size_t vi = (i[1] + i[0] * dims[1]) * vertex_stride + pos_offset;
            state.prev_pos[i] = glm::vec3(vb[vi], vb[vi + 1], vb[vi + 2]);
        }
    }

    _dims = dims;
    // _construct _normal_buf
    _normal_buf.resize(dims[0]);
    for (auto& row : _normal_buf) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    // _construct _tangent_buf
    _tangent_buf.resize(dims[0]);
    for (auto& row : _tangent_buf) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    // assuming order of constraint does not affect simulation
    // populate index buffer
    //
    //             rest_len_h
    //              00 --10
    //  rest_len_v  | \ / |
    //              |  \  |
    //              | / \ |
    //              01 --11
    for (i[0] = 0; i[0] < dims[0] - 1; ++i[0]) {
        for (i[1] = 0; i[1] < dims[1] - 1; ++i[1]) {
            Idx2 i00 = i;
            Idx2 i10 = {i[0]    , i[1] + 1};
            Idx2 i01 = {i[0] + 1, i[1]    };
            Idx2 i11 = {i[0] + 1, i[1] + 1};
            float len_h = glm::length(state.pos[i00] - state.pos[i10]);
            float len_v = glm::length(state.pos[i00] - state.pos[i01]);
            float len_diag0 = glm::length(state.pos[i00] - state.pos[i11]);
            float len_diag1 = glm::length(state.pos[i01] - state.pos[i10]);
            _constraints.push_back({i00, i10, len_h});
            _constraints.push_back({i00, i01, len_v});
            _constraints.push_back({i00, i11, len_diag0});
            _constraints.push_back({i01, i10, len_diag1});
        }
    }
    // right edge
    i[1] = dims[1] - 1;
    for (i[0] = 0; i[0] < dims[0] - 1; ++i[0]) {
        Idx2 i00 = i;
        Idx2 i01 = {i[0] + 1, i[1]};
        float rest_len_v = glm::length(state.pos[i00] - state.pos[i01]);
        _constraints.push_back({i, i01, rest_len_v});
    }
    // bottom edge
    i[0] = dims[0] - 1;
    for (i[1] = 0; i[1] < dims[1] - 1; ++i[1]) {
        Idx2 i00 = i;
        Idx2 i10 = {i[0], i[1] + 1};
        float rest_len_h = glm::length(state.pos[i00] - state.pos[i10]);
        _constraints.push_back({i, i10, rest_len_h});
    }

    // construct kinematic ids
    for (uint16_t iv : kinematic_ids) {
        i = {iv / dims[1], iv % dims[1]};
        state.kinematic_ids.push_back(i);
    }
}

void Cloth::update_kinematics(const Arr1D<glm::vec3>& k_pos) {
    state.kinematic_pos = k_pos;
}

void Cloth::update(float dt) {
    verlet(dt);
    apply_constraint();
    compute_tangent_norm();
}

void Cloth::verlet(float dt) {
    // TODO: only gravity at this moment, add other accelerations
    Idx2 i;
    for (i[0] = 0; i[0] < _dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < _dims[1]; ++i[1]) {
            glm::vec3 cur_pos = state.pos[i];
            state.pos[i] += state.pos[i] - state.prev_pos[i] + state.gravity * dt * dt;
            state.prev_pos[i] = cur_pos;
        }
    }
}

void Cloth::apply_constraint() {
    const int iterations = 10;
    for (int iter = 0; iter < iterations; ++iter) {
        // traverse constraints
        for (auto&c : _constraints) {
            glm::vec3& p0 = state.pos[c.i0];
            glm::vec3& p1 = state.pos[c.i1];
            glm::vec3 l01 = p1 - p0;
            // TODO: square root is very expensive
            float len = glm::length(l01);
            float diff = (len - c.rest_len) / len;
            p0 += l01 * diff * 0.5f;
            p1 -= l01 * diff * 0.5f;
        }


        // maintain kinematics vertices
        for (size_t ii = 0; ii < state.kinematic_ids.size(); ++ii) {
            state.pos[state.kinematic_ids[ii]] = state.kinematic_pos[ii];
        }
        // TODO: anchor upper-left and upper-right points, for the moment.
        // Add kinematic points later
        // Idx2 i_ul = {0, 0};
        // Idx2 i_ur = {0, _dims[1] - 1};
        // state.pos[i_ul] = glm::vec3(-2.0f, 2.0f, 0.0f);
        // state.pos[i_ur] = glm::vec3( 2.0f, 2.0f, 0.0f);
    }
}

void Cloth::copy_back(Arr1D<float>::iterator vb_beg,
                      size_t vertex_stride,
                      size_t pos_offset,
                      size_t norm_offset,
                      size_t tangent_offset,
                      bool inverse_norm,
                      bool inverse_bitangent) {
    vertex_stride /= sizeof(float);
    pos_offset /= sizeof(float);
    norm_offset /= sizeof(float);
    tangent_offset /= sizeof(float);

    // Copy attributes
    Idx2 i;
    size_t vb_offset = 0;
    auto attrib_iter = vb_beg;
    for (i[0] = 0; i[0] < _dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < _dims[1]; ++i[1]) {
            // position
            attrib_iter = vb_beg + vb_offset + pos_offset;
            memcpy(&*(attrib_iter),
                   &state.pos[i],
                   sizeof(float) * 3);

            // normal
            attrib_iter = vb_beg + vb_offset + norm_offset;
            memcpy(&*(attrib_iter),
                   &state.norm[i],
                   sizeof(float) * 3);
            if (inverse_norm) {
                *(attrib_iter) = -*(attrib_iter);
                *(attrib_iter + 1) = -*(attrib_iter + 1);
                *(attrib_iter + 2) = -*(attrib_iter + 2);
            }

            //tangent
            attrib_iter = vb_beg + vb_offset + tangent_offset;
            memcpy(&*(attrib_iter),
                   &state.tangent[i],
                   sizeof(float) * 3);
            *(attrib_iter + 3) = inverse_bitangent ? -1.0f : 1.0f;

            vb_offset += vertex_stride;
        }
    }
}

void Cloth::compute_tangent_norm() {
    // TODO: judging from renderdoc, values of normal and tangent does not seem right
    zero_arr2d(_normal_buf);
    zero_arr2d(_tangent_buf);
    auto& pos = state.pos;
    auto& tex = state.tex;

    // accumulate normals
    for (Idx3x2& i : state.indices) {
        glm::vec3& pos0 = pos[i[0]];
        glm::vec3& pos1 = pos[i[1]];
        glm::vec3& pos2 = pos[i[2]];

        glm::vec3 e1 = pos1 - pos0;
        glm::vec3 e2 = pos2 - pos0;
        glm::vec3 norm = glm::cross(e1, e2);

        _normal_buf[i[0]] += norm;
        _normal_buf[i[1]] += norm;
        _normal_buf[i[2]] += norm;
    }

    // accumulate tangents
    for (Idx3x2& i : state.indices) {
        glm::vec3& pos0 = pos[i[0]];
        glm::vec3& pos1 = pos[i[1]];
        glm::vec3& pos2 = pos[i[2]];

        glm::vec2& tex0 = tex[i[0]];
        glm::vec2& tex1 = tex[i[1]];
        glm::vec2& tex2 = tex[i[2]];

        glm::vec3 e1 = pos1 - pos0;
        glm::vec3 e2 = pos2 - pos0;
        glm::vec2 uv1 = tex1 - tex0;
        glm::vec2 uv2 = tex2 - tex0;

        // TODO: does not have to compute inverse uv every frame.
        // optimization opportunity here
        glm::vec3 tangent = (glm::mat2x3(e1, e2) *
                             glm::inverse(glm::mat2(uv1, uv2)))[0];
        
        _tangent_buf[i[0]] += tangent;
        _tangent_buf[i[1]] += tangent;
        _tangent_buf[i[2]] += tangent;
    }

    // normalize tangents and normals
    Idx2 i;
    for (i[0] = 0; i[0] < _dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < _dims[1]; ++i[1]) {
            glm::vec3& n_sum = _normal_buf[i];
            glm::vec3& t_sum = _tangent_buf[i];

            state.norm[i] = glm::normalize(n_sum);
            state.tangent[i] = t_sum - state.norm[i] * glm::dot(state.norm[i], t_sum);
            state.tangent[i] = glm::normalize(state.tangent[i]);
        }
    }


}

}