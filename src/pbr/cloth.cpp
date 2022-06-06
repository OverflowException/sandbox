#include "cloth.h"
#include "geometry.h"
#include "glm/trigonometric.hpp"
#include <cstring>
#include <iostream>

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
            _ortho_edge.push_back({i00, i10, len_h});
            _ortho_edge.push_back({i00, i01, len_v});
            _diag_edge.push_back({i00, i11, len_diag0});
            _diag_edge.push_back({i01, i10, len_diag1});
        }
    }
    // right edge
    i[1] = dims[1] - 1;
    for (i[0] = 0; i[0] < dims[0] - 1; ++i[0]) {
        Idx2 i00 = i;
        Idx2 i01 = {i[0] + 1, i[1]};
        float rest_len_v = glm::length(state.pos[i00] - state.pos[i01]);
        _ortho_edge.push_back({i, i01, rest_len_v});
    }
    // bottom edge
    i[0] = dims[0] - 1;
    for (i[1] = 0; i[1] < dims[1] - 1; ++i[1]) {
        Idx2 i00 = i;
        Idx2 i10 = {i[0], i[1] + 1};
        float rest_len_h = glm::length(state.pos[i00] - state.pos[i10]);
        _ortho_edge.push_back({i, i10, rest_len_h});
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
    // self_collision_p2p_test();
    // self_collision_p2tri_test();
    self_collision_e2e_test();
    apply_constraint();
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
    int p2p_collision_resolved = 0;
    int p2tri_collision_resolved = 0;
    int e2e_collision_resolved = 0;
    for (int iter = 0; iter < iterations; ++iter) {
        // traverse constraints
        // structural constraints
        for (auto&e : _ortho_edge) {
            glm::vec3& p0 = state.pos[e.i0];
            glm::vec3& p1 = state.pos[e.i1];
            glm::vec3 l01 = p1 - p0;
            // TODO: square root is very expensive
            float len = glm::length(l01);
            float diff = (len - e.rest_len) / len;
            p0 += l01 * diff * 0.5f;
            p1 -= l01 * diff * 0.5f;
        }
        for (auto&e : _diag_edge) {
            glm::vec3& p0 = state.pos[e.i0];
            glm::vec3& p1 = state.pos[e.i1];
            glm::vec3 l01 = p1 - p0;
            // TODO: square root is very expensive
            float len = glm::length(l01);
            float diff = (len - e.rest_len) / len;
            p0 += l01 * diff * 0.5f;
            p1 -= l01 * diff * 0.5f;
        }

        // p2p collision resolution
        for (auto&m : _p2p_manifolds) {
            glm::vec3& p0 = state.pos[m.i0];
            glm::vec3& p1 = state.pos[m.i1];
            glm::vec3 e = p1 - p0;
            float proj = glm::dot(e, m.n);
            if (proj < 2 * _p2p_r) {
                ++p2p_collision_resolved;
                float diff = 2 * _p2p_r - proj;
                p0 -= m.n * diff * 0.5f;
                p1 += m.n * diff * 0.5f;
            }
        }
        // p2tri collision resolution
        for (auto& m : _p2tri_manifolds) {
            glm::vec3& p = state.pos[m.ip];
            glm::vec3& p0 = state.pos[m.itri[0]];
            glm::vec3& p1 = state.pos[m.itri[1]];
            glm::vec3& p2 = state.pos[m.itri[2]];
            float proj = glm::dot(p - p0, m.n);
            if (proj < 2 * _p2tri_r) {
                ++p2tri_collision_resolved;
                // TODO: naive collision resolution.
                // Should try out the geometric stiffness method propsed by EA
                float diff = 2 * _p2tri_r - proj;
                // p += m.n * diff;
                // p0 -= m.n * diff * 0.05f;
                // p1 -= m.n * diff * 0.05f;
                // p2 -= m.n * diff * 0.05f;
            }
        }
        // e2e collision resolution
        for (auto& m : _e2e_manifolds) {
            glm::vec3& e0p0 = state.pos[m.e0i0];
            glm::vec3& e0p1 = state.pos[m.e0i1];
            glm::vec3& e1p0 = state.pos[m.e1i0];
            glm::vec3& e1p1 = state.pos[m.e1i1];
            glm::vec3 e0pc = e0p0 + (e0p1 - e0p0) * m.s;
            glm::vec3 e1pc = e1p0 + (e1p1 - e1p0) * m.t;
            float proj = glm::dot(e1pc - e0pc, m.n);
            if (proj < 2 * _e2e_r) {
                ++e2e_collision_resolved;
                float diff = 2 * _e2e_r - proj;
                e0p0 -= m.n * diff * (1.0f - m.s);
                e0p1 -= m.n * diff * m.s;
                e1p0 += m.n * diff * (1.0f - m.t);
                e1p1 += m.n * diff * m.t;
            }
        }

        // maintain kinematics vertices
        for (size_t ii = 0; ii < state.kinematic_ids.size(); ++ii) {
            state.pos[state.kinematic_ids[ii]] = state.kinematic_pos[ii];
        }
    }
    std::cout << "p2p: collision resolved = " << p2p_collision_resolved << std::endl;
    std::cout << "p2tri: collision resolved = " << p2tri_collision_resolved << std::endl;
    std::cout << "e2e: collision resolved = " << e2e_collision_resolved << std::endl;
}

void Cloth::self_collision_p2p_test() {
    _p2p_manifolds.clear();

    size_t total_collision_checks = 0;
    size_t broad_phase_intersect = 0;
    size_t narrow_phase_collide = 0;

    Idx2 i;
    // traverse all vertices
    for (i[0] = 0; i[0] < _dims[0]; ++i[0]) {
        for (i[1] = 0; i[1] < _dims[1]; ++i[1]) {
            // traverse all vertices after current vertex
            size_t j = i[0] * _dims[1] + i[1] + 1;
            for (; j < _dims[0] * _dims[1]; ++j) {
                Idx2 k = {j / _dims[1], j % _dims[1]};
                ++total_collision_checks;
                // predictive collision
                Geometry::LineSeg seg0 = {state.prev_pos[i], state.pos[i]};
                Geometry::LineSeg seg1 = {state.prev_pos[k], state.pos[k]};
                Geometry::AABB aabb0 = Geometry::aabb(seg0, _p2p_r + _p2p_r_sc);
                Geometry::AABB aabb1 = Geometry::aabb(seg1, _p2p_r + _p2p_r_sc);
                // TODO: mock broad phase
                if (!Geometry::intersect(aabb0, aabb1)) {
                    continue;
                }
                ++broad_phase_intersect;
                float d = Geometry::distance(seg0, seg1);
                if (d <= 2 * _p2p_r + _p2p_r_sc) {
                    ++narrow_phase_collide;
                    _p2p_manifolds.push_back({i, k, glm::normalize(state.prev_pos[k] - state.prev_pos[i])});
                }
            }
        }
    }

    std::cout << "p2p: total collision checks = " << total_collision_checks << std::endl;
    std::cout << "p2p: broad phase intersect = " << broad_phase_intersect << std::endl;
    std::cout << "p2p: narrow phase collide = " << narrow_phase_collide << std::endl;
}

void Cloth::self_collision_e2e_test() {
    _e2e_manifolds.clear();

    size_t total_collision_checks = 0;
    size_t broad_phase_intersect = 0;
    size_t narrow_phase_collide = 0;

    // Traverse orthogonal edges
    for (size_t i = 0; i < _ortho_edge.size(); ++i) {
        Idx2 e0i0 = _ortho_edge[i].i0;
        Idx2 e0i1 = _ortho_edge[i].i1;
        // traverse all orthogonal edges after current edge
        for (size_t j = i + 1; j < _ortho_edge.size(); ++j) {
            Idx2 e1i0 = _ortho_edge[j].i0;
            Idx2 e1i1 = _ortho_edge[j].i1;

            // skip interconnected ones
            if (e1i0 == e0i0 ||
                e1i0 == e0i1 ||
                e1i1 == e0i0 ||
                e1i1 == e0i1) {
                continue;
            }
            ++total_collision_checks;
            // predictive collision
            Geometry::LineSeg e0_prev = {state.prev_pos[e0i0], state.prev_pos[e0i1]};
            Geometry::LineSeg e0_cur = {state.pos[e0i0], state.pos[e0i1]};
            Geometry::AABB aabb_e0 = Geometry::merge(Geometry::aabb(e0_prev, _e2e_r + _e2e_r_sc),
                                                     Geometry::aabb(e0_cur, _e2e_r + _e2e_r_sc));

            Geometry::LineSeg e1_prev = {state.prev_pos[e1i0], state.prev_pos[e1i1]};
            Geometry::LineSeg e1_cur = {state.pos[e1i0], state.pos[e1i1]};
            Geometry::AABB aabb_e1 = Geometry::merge(Geometry::aabb(e1_prev, _e2e_r + _e2e_r_sc),
                                                     Geometry::aabb(e1_cur, _e2e_r + _e2e_r_sc));
            
            // TODO: mock broad phase
            if (!Geometry::intersect(aabb_e0, aabb_e1)) {
                continue;
            }
            ++broad_phase_intersect;
            // closest coordinates, on both lines
            glm::vec2 cc = Geometry::closest(e0_prev, e1_prev);
            glm::vec3 e0_prev_pc = e0_prev.p0 + (e0_prev.p1 - e0_prev.p0) * cc.x;
            glm::vec3 e1_prev_pc = e1_prev.p0 + (e1_prev.p1 - e1_prev.p0) * cc.y;
            // seperating direction
            glm::vec3 sep_dir = glm::normalize(e1_prev_pc - e0_prev_pc);
            glm::vec3 e0_cur_pc = e0_cur.p0 + (e0_cur.p1 - e0_cur.p0) * cc.x;
            glm::vec3 e1_cur_pc = e1_cur.p0 + (e1_cur.p1 - e1_cur.p0) * cc.y;
            // predicts collision
            if (dot(e1_cur_pc - e0_cur_pc, sep_dir) < 2 * _e2e_r + _e2e_r_sc) {
                ++narrow_phase_collide;
                _e2e_manifolds.push_back({e0i0, e0i1, e1i0, e1i1, cc.x, cc.y, sep_dir});
            }
        }
    }
    std::cout << "e2e: total collision checks = " << total_collision_checks << std::endl;
    std::cout << "e2e: broad phase intersect = " << broad_phase_intersect << std::endl;
    std::cout << "e2e: narrow phase collide = " << narrow_phase_collide << std::endl;
}

void Cloth::self_collision_p2tri_test() {
    _p2tri_manifolds.clear();

    size_t total_collision_checks = 0;
    size_t broad_phase_intersect = 0;
    size_t narrow_phase_collide = 0;

    Idx2 vi;
    // traverse all vertices
    for (vi[0] = 0; vi[0] < _dims[0]; ++vi[0]) {
        for (vi[1] = 0; vi[1] < _dims[1]; ++vi[1]) {
            // traverse all triangles except the one that this vertex itself is in
            for (auto& idx : state.indices) {
                if (vi == idx[0] || vi == idx[1] || vi == idx[2]) {
                    continue;
                }
                ++total_collision_checks;
                glm::vec3& p_cur = state.pos[vi];
                glm::vec3& p_prev = state.prev_pos[vi];
                Geometry::LineSeg seg = {p_prev, p_cur};
                Geometry::Triangle tri_cur = {state.pos[idx[0]],
                                              state.pos[idx[1]],
                                              state.pos[idx[2]]};
                Geometry::Triangle tri_prev = {state.prev_pos[idx[0]],
                                               state.prev_pos[idx[1]],
                                               state.prev_pos[idx[2]]};
                // TODO: mock broad phase
                Geometry::AABB aabb_seg = Geometry::aabb(seg, _p2tri_r + _p2tri_r_sc);
                Geometry::AABB aabb_prism = Geometry::merge(Geometry::aabb(tri_prev, _p2tri_r + _p2tri_r_sc),
                                                            Geometry::aabb(tri_cur,  _p2tri_r + _p2tri_r_sc));
                if (!Geometry::intersect(aabb_seg, aabb_prism)) {
                    continue;
                }
                ++broad_phase_intersect;
                // narrow phase collision detection
                // closest point on triangle
                glm::vec3 pc_bary = Geometry::closest(p_prev, tri_prev);
                glm::vec3 pc_prev = tri_prev.p0 * pc_bary.x +
                                    tri_prev.p1 * pc_bary.y +
                                    tri_prev.p2 * pc_bary.z;
                glm::vec3 pc_cur = tri_cur.p0 * pc_bary.x +
                                   tri_cur.p1 * pc_bary.y +
                                   tri_cur.p2 * pc_bary.z;
                
                // separating direction
                glm::vec3 sep_dir = glm::normalize(pc_prev - p_prev);
                if (dot(pc_cur - p_cur, sep_dir) < 2 * _p2tri_r + _p2tri_r_sc) {
                    ++narrow_phase_collide;
                    glm::vec3 positive_dir = glm::cross(tri_prev.p1 - tri_prev.p0,
                                                        tri_prev.p2 - tri_prev.p0);
                    bool on_positive_side = glm::dot(positive_dir, p_prev - tri_prev.p0) > 0.0f;
                    _p2tri_manifolds.push_back({vi, idx,
                                                on_positive_side ?  glm::normalize(positive_dir) :
                                                                   -glm::normalize(positive_dir)});
                }
            }
        }
    }

    std::cout << "p2tri: total collision checks = " << total_collision_checks << std::endl;
    std::cout << "p2tri: broad phase intersect = " << broad_phase_intersect << std::endl;
    std::cout << "p2tri: narrow phase collide = " << narrow_phase_collide << std::endl;
 }

void Cloth::copy_back(Arr1D<float>::iterator vb_beg,
                      size_t vertex_stride,
                      size_t pos_offset,
                      size_t norm_offset,
                      size_t tangent_offset,
                      bool inverse_norm,
                      bool inverse_bitangent) {
    compute_tangent_norm();

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