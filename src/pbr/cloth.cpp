#include "cloth.h"
#include "glm/trigonometric.hpp"
#include <cstring>

namespace phy {

void Cloth::init(const Arr1D<float>& vb,
                 const Arr1D<uint16_t>& ib,
                 Idx2 dims, // rows vs columns
                 size_t vertex_stride,
                 size_t pos_offset,
                 size_t tex_offset) {
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

    // TODO: construct constraints
    

    // construct position buffer
    state.pos.resize(dims[0]);
    for (auto& row : state.pos) {
        row.resize(dims[1], glm::vec3(0.0f));
    }

    Idx2 i;
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

    // TODO: construct kinematic ids
    // TODO: no cross constraints for the moment
}

void Cloth::update(float dt) {
    Idx2 i;
    for (i[0] = 0; i[0] < _dims[0]; ++i[0]) {
        for(i[1] = 0; i[1] < _dims[1]; ++i[1]) {
            float amp = 0.4f * glm::sin(dt + (i[0] * i[0] + i[1] * i[1]) * 0.01f);
            state.pos[i].z = amp;
        }
    }

    compute_tangent_norm();
}

void Cloth::copy_back(Arr1D<float>::iterator vb_beg,
                      size_t vertex_stride,
                      size_t pos_offset,
                      size_t norm_offset,
                      size_t tangent_offset,
                      bool reverse_norm,
                      bool reverse_tangent) {
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
            if (reverse_norm) {
                *(attrib_iter) = -*(attrib_iter);
                *(attrib_iter + 1) = -*(attrib_iter + 1);
                *(attrib_iter + 2) = -*(attrib_iter + 2);
            }

            //tangent
            attrib_iter = vb_beg + vb_offset + tangent_offset;
            memcpy(&*(attrib_iter),
                   &state.tangent[i],
                   sizeof(float) * 3);
            if (reverse_tangent) {
                *(attrib_iter) = -*(attrib_iter);
                *(attrib_iter + 1) = -*(attrib_iter + 1);
                *(attrib_iter + 2) = -*(attrib_iter + 2);
            }

            vb_offset += vertex_stride;
        }
    }
}

void Cloth::compute_tangent_norm() {
    // TODO: judging from renderdoc, values of normal and tangent does not seem right
    clear_arr2d(_normal_buf);
    clear_arr2d(_tangent_buf);
    auto& pos = state.pos;
    auto& tex = state.tex;

    // accumulate normals
    for (Idx3x2& i : state.indices) {
        glm::vec3& pos0 = pos[i[0]];
        glm::vec3& pos1 = pos[i[1]];
        glm::vec3& pos2 = pos[i[2]];

        glm::vec3 e1 = pos1 - pos0;
        glm::vec3 e2 = pos2 - pos1;
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
        glm::vec3 e2 = pos2 - pos1;
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

            state.tangent[i] = t_sum - n_sum * glm::dot(n_sum, t_sum);
            state.tangent[i] = glm::normalize(state.tangent[i]);

            state.norm[i] = glm::normalize(n_sum);
        }
    }


}

}