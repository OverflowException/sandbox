#include "procedural_shapes.h"

#include <algorithm>
#include <map>
#include <cmath>
#include <cstring>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <set>
#include <cstdio>

void ProceduralShapes::gen_ico_sphere(std::vector<float>& vb,
                                      std::vector<uint16_t>& ib,
                                      VertexAttrib attrib,
                                      float r,
                                      int lod,
                                      IndexType i_type) {
    ShapeData data;
    auto& vec3_pb  = data.vec3_pb;
    auto& vec3_nb  = data.vec3_nb;
    auto& vec2_uvb = data.vec2_uvb;
    auto& vec3_tb  = data.vec3_tb;
    auto& vec3_ib  = data.vec3_ib;
    
    //generate position buffer and index buffer
    init_icosphere(r, vec3_pb, vec3_ib);
    for(int i = 0; i < lod; ++i) {
        add_icosphere_lod(r, vec3_pb, vec3_ib);
    }

    if (attrib & VertexAttrib::NORM) {
        //generate normal buffer
        for(const glm::vec3& p : vec3_pb) {
            vec3_nb.push_back(glm::normalize(p));
        }
    }

    if (attrib & VertexAttrib::UV) {
        // generate uv buffer
        for (const glm::vec3& p : vec3_pb) {
            float u = std::atan2(p.y, p.x) * M_1_PI * 0.5 + 0.5;
            float v = std::asin(p.z / r) * M_1_PI + 0.5;
            vec2_uvb.push_back(glm::vec2(u, v));
        }

        // eliminate seam
        // TODO: seam points redundant
        for (glm::u16vec3& i : vec3_ib) {
            // check AABB of individual triangle
            float y_min = std::numeric_limits<float>::max();
            float y_max = -std::numeric_limits<float>::max();
            uint16_t* i_ptr = (uint16_t*)&i;
            for (int offset = 0; offset < 3; ++offset, ++i_ptr) {
                glm::vec3& p = vec3_pb[*i_ptr];
                y_min = glm::min(p.y, y_min);
                y_max = glm::max(p.y, y_max);
            }

            if (y_min < 0.0f && y_max > 0.0f) {
                // this triangle intersects y = 0 plane
                float x_mean = (vec3_nb[i.x].x + vec3_nb[i.y].x + vec3_nb[i.z].x) / 3;
                if (x_mean < 0.0f) {
                    // this triangle is facing -x direction. So crossing the seam
                    i_ptr = (uint16_t*)&i;
                    for (int offset = 0; offset < 3; ++offset, ++i_ptr) {
                        glm::vec3& p = vec3_pb[*i_ptr];
                        glm::vec3& n = vec3_nb[*i_ptr];
                        glm::vec2& uv = vec2_uvb[*i_ptr];
                        if (p.y < 0.0f && p.z != r && p.z != -r) {
                            // leave out the top and bottom point
                            vec3_pb.push_back(p);
                            vec3_nb.push_back(n);
                            vec2_uvb.emplace_back(uv.x + 1.0f, uv.y);
                            *i_ptr = vec3_pb.size() - 1;
                        }
                    }
                }
            }

            if (y_max == 0.0f) {
                float x_mean = (vec3_nb[i.x].x + vec3_nb[i.y].x + vec3_nb[i.z].x) / 3;
                if (x_mean < 0.0f) {
                    i_ptr = (uint16_t*)&i;
                    for (int offset = 0; offset < 3; ++offset, ++i_ptr) {
                        glm::vec3& p = vec3_pb[*i_ptr];
                        glm::vec3& n = vec3_nb[*i_ptr];
                        glm::vec2& uv = vec2_uvb[*i_ptr];
                        if (p.y == 0.0f && p.z != r && p.z != -r) {
                            // leave out the top and bottom point
                            vec3_pb.push_back(p);
                            vec3_nb.push_back(n);
                            vec2_uvb.emplace_back(uv.x - 1.0f, uv.y);
                            *i_ptr = vec3_pb.size() - 1;
                        }
                    }
                }
            }
        }

        // rearrange top and bottom point
        for (glm::u16vec3& i : vec3_ib) {
            glm::vec3& p0 = vec3_pb[i.x];
            glm::vec3& p1 = vec3_pb[i.y];
            glm::vec3& p2 = vec3_pb[i.z];
            glm::vec2& uv0 = vec2_uvb[i.x];
            glm::vec2& uv1 = vec2_uvb[i.y];
            glm::vec2& uv2 = vec2_uvb[i.z];

            if (p0.z == r || p0.z == -r) {
                vec3_pb.push_back(p0);
                vec3_nb.push_back(vec3_nb[i.x]);
                vec2_uvb.emplace_back((uv1.x + uv2.x) / 2, uv0.y);
                i.x = vec3_pb.size() - 1;
            } else if (p1.z == r || p1.z == -r) {
                vec3_pb.push_back(p1);
                vec3_nb.push_back(vec3_nb[i.y]);
                vec2_uvb.emplace_back((uv0.x + uv2.x) / 2, uv1.y);
                i.y = vec3_pb.size() - 1;
            } else if (p2.z == r || p2.z == -r) {
                vec3_pb.push_back(p2);
                vec3_nb.push_back(vec3_nb[i.z]);
                vec2_uvb.emplace_back((uv0.x + uv1.x) / 2, uv2.y);
                i.z = vec3_pb.size() - 1;
            }
        }
    }

    if (attrib & VertexAttrib::TANGENT) {
        for (int i = 0; i < vec3_pb.size(); ++i) {
            const glm::vec2& uv = vec2_uvb[i];
            glm::vec3 t0(0.0f, -1.0f, 0.0f);
            glm::quat t_rot = glm::angleAxis((float)uv.x * glm::two_pi<float>(),
                                            glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec3 t = glm::rotate(t_rot, t0);
            vec3_tb.push_back(t);
        }
    }

    // TODO: This is not a good way to compute tangent and bitangent for sphere.
    // But you should test and document this
/*      if (attrib & VertexAttrib::TANGENT) {
        vec3_tb.resize(vec3_pb.size());
        std::vector<bool> computed(vec3_pb.size(), false);
        for (glm::u16vec3& i : vec3_ib) {
            if (computed[i.x] && computed[i.y] && computed[i.z]) {
                continue;
            }

            glm::vec3 p01 = vec3_pb[i.y] - vec3_pb[i.x];
            glm::vec3 p02 = vec3_pb[i.z] - vec3_pb[i.x];
            glm::vec2 uv01 = vec2_uv[i.y] - vec2_uv[i.x];
            glm::vec2 uv02 = vec2_uv[i.z] - vec2_uv[i.x];

            glm::mat2 tb_coords(uv01, uv02);
            glm::mat2x3 edge_vectors(p01, p02);
            // tangent - bitangent bases. not exactly perpendicular, since uv is computed in sphere coordinate
            // document this
            glm::mat2x3 tb_bases = edge_vectors * glm::inverse(tb_coords);
            tb_bases[0] = glm::normalize(tb_bases[0]);

            if (!computed[i.x]) {
                vec3_tb[i.x] = tb_bases[0];
                computed[i.x] = true;
            }
            if (!computed[i.y]) {
                vec3_tb[i.y] = tb_bases[0];
                computed[i.y] = true;
            }
            if (!computed[i.z]) {
                vec3_tb[i.z] = tb_bases[0];
                computed[i.z] = true;
            }
        } 
    } */

    size_t v_size = vec3_pb.size();
    for (int i = 0; i < v_size; ++i) {
        if (attrib & VertexAttrib::POS) {
            vb.insert(vb.end(), {vec3_pb[i].x, vec3_pb[i].y, vec3_pb[i].z});
        }
        if (attrib & VertexAttrib::NORM) {
            vb.insert(vb.end(), {vec3_nb[i].x, vec3_nb[i].y, vec3_nb[i].z});
        }
        if (attrib & VertexAttrib::UV) {
            // 4x repeated on u, 2x repeated on v
            vb.insert(vb.end(), {vec2_uvb[i].x * 4.0f, vec2_uvb[i].y * 2.0f});
        }
        if (attrib & VertexAttrib::TANGENT) {
            vb.insert(vb.end(), {vec3_tb[i].x, vec3_tb[i].y, vec3_tb[i].z});
        }
    }

    // set index buffer
    if (i_type == IndexType::LINE) {
        tri2line(vec3_ib, ib);
    } else if (i_type == IndexType::TRIANGLE) {
        ib.clear();
        for (auto& i : vec3_ib) {
            ib.insert(ib.end(), { i.x, i.y, i.z });
        }
    } else {
        assert(false);
    }
}

void ProceduralShapes::gen_cube(std::vector<float>& vb,
                                VertexAttrib attrib,
                                glm::vec3 half_dim,
                                IndexType i_type) {
    
    // TODO: This is a counter-clock wise cube
    vb = {
    	// positions          
    	-1.0f,  1.0f, -1.0f,
    	-1.0f, -1.0f, -1.0f,
    	 1.0f, -1.0f, -1.0f,
    	 1.0f, -1.0f, -1.0f,
    	 1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,

    	-1.0f, -1.0f,  1.0f,
    	-1.0f, -1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f, -1.0f,
    	-1.0f,  1.0f,  1.0f,
    	-1.0f, -1.0f,  1.0f,

     	1.0f, -1.0f, -1.0f,
     	1.0f, -1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f, -1.0f,
     	1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
    	-1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f, -1.0f,  1.0f,
    	-1.0f, -1.0f,  1.0f,

    	-1.0f,  1.0f, -1.0f,
     	1.0f,  1.0f, -1.0f,
     	1.0f,  1.0f,  1.0f,
     	1.0f,  1.0f,  1.0f,
    	-1.0f,  1.0f,  1.0f,
    	-1.0f,  1.0f, -1.0f,

    	-1.0f, -1.0f, -1.0f,
    	-1.0f, -1.0f,  1.0f,
     	1.0f, -1.0f, -1.0f,
     	1.0f, -1.0f, -1.0f,
    	-1.0f, -1.0f,  1.0f,
     	1.0f, -1.0f,  1.0f
    };
}

void ProceduralShapes::gen_z_cone(std::vector<float>& vb,
                                  std::vector<uint16_t>& ib,
                                  VertexAttrib attrib,
                                  float r_top,
                                  float r_bottom,
                                  float height,
                                  int sectors,
                                  int stacks,
                                  IndexType i_type) {
    ShapeData data;
    auto& vec3_pb  = data.vec3_pb;
    auto& vec3_nb  = data.vec3_nb;
    auto& vec2_uvb = data.vec2_uvb;
    auto& vec3_tb  = data.vec3_tb;
    auto& vec3_ib  = data.vec3_ib;

    float delta_phi = M_PI * 2.0f / sectors;
    float delta_r = (r_top - r_bottom) / stacks;
    float delta_z = height / stacks;

    // generate uv buffer
    for (int sta = 0; sta <= stacks; ++sta) {
        float v = (float)sta / stacks;
        for (int sec = 0; sec <= sectors; ++sec) {
            float u = 0.0f;
            // handle seam
            if (sec == 0) {
                u = 0.0f;
            } else if (sec == sectors) {
                u = 1.0f;
            } else {
                u = (float)sec / sectors;
            }
            vec2_uvb.emplace_back(u, v);
        }
    }

    // generate position buffer
    float z_start = -height / 2;
    for (glm::vec2& uv : vec2_uvb) {
        float r = r_bottom + (r_top - r_bottom) * uv.y;
        float phi = 0.0f;
        // handle seam
        if (uv.x == 0.0f) {
            phi = 0.0f;
        } else if (uv.x == 1.0f) {
            phi = 0.0f;
        } else {
            phi = M_PI * 2.0f * uv.x;
        }
        float x = r * cos(phi);
        float y = r * sin(phi);
        float z = z_start + uv.y * height;
        vec3_pb.emplace_back(x, y, z);
    }

    // generate tangent buffer
    glm::vec3 t0(0.0f, 1.0f, 0.0f);
    for (glm::vec2& uv : vec2_uvb) {
        glm::quat t_rot = glm::angleAxis(uv.x * glm::two_pi<float>(),
                                         glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 t = glm::normalize(glm::rotate(t_rot, t0));
        vec3_tb.push_back(t);
    }

    // generate normal buffer
    float angle = std::atan2(r_top - r_bottom, height);
    for (int i = 0; i < vec3_pb.size(); ++i) {
        glm::vec3& p = vec3_pb[i];
        glm::vec3& t = vec3_tb[i];
        glm::vec3 n(p.x, p.y, 0.0f);
        glm::quat n_rot = glm::angleAxis(angle, t);
        n = glm::normalize(glm::rotate(n_rot, n));
        vec3_nb.push_back(n);
    }

    // set indices
    // v -- axial index  -- stacks
    // u -- radial index -- sectors
    //  p01 -- p11
    //   |    / |
    //   |   /  |
    //   |  /   |
    //   p00 -- p10
    for (uint16_t v = 0; v < stacks; ++v) {
        for (uint16_t u = 0; u < sectors; ++u) {
            uint16_t i00 = v * (sectors + 1) + u;
            uint16_t i10 = i00 + 1;
            uint16_t i01 = i00 + (sectors + 1);
            uint16_t i11 = i01 + 1;
            
            // pointing out
            vec3_ib.push_back({i00, i11, i01});
            vec3_ib.push_back({i00, i10, i11});
        }
    }

   // bottom lid
    ShapeData bottom_data = std::move(make_lid(data,
                                      0, sectors + 1,
                                      glm::vec3(0.0f, 0.0f, -height / 2),
                                      glm::vec3(0.0f, 0.0f, -1.0f),
                                      false));

    // top lid
    ShapeData top_data = std::move(make_lid(data,
                                   stacks * (sectors + 1), (stacks + 1) * (sectors + 1),
                                   glm::vec3(0.0f, 0.0f, height / 2),
                                   glm::vec3(0.0f, 0.0f, 1.0f),
                                   true));
    
    data = std::move(assemble(top_data, data));
    data = std::move(assemble(data, bottom_data));

    size_t v_size = vec3_pb.size();
    for (int i = 0; i < v_size; ++i) {
        if (attrib & VertexAttrib::POS) {
            vb.insert(vb.end(), {vec3_pb[i].x, vec3_pb[i].y, vec3_pb[i].z});
        }
        if (attrib & VertexAttrib::NORM) {
            vb.insert(vb.end(), {vec3_nb[i].x, vec3_nb[i].y, vec3_nb[i].z});
        }
        if (attrib & VertexAttrib::UV) {
            vb.insert(vb.end(), {vec2_uvb[i].x, vec2_uvb[i].y});
        }
        if (attrib & VertexAttrib::TANGENT) {
            vb.insert(vb.end(), {vec3_tb[i].x, vec3_tb[i].y, vec3_tb[i].z});
        }
    }

    if (i_type == IndexType::LINE) {
        tri2line(vec3_ib, ib);
    } else if (i_type == IndexType::TRIANGLE) {
        ib.clear();
        for (auto& i : vec3_ib) {
            ib.insert(ib.end(), { i.x, i.y, i.z });
        }
    } else {
        assert(false);
    }
}

void ProceduralShapes::init_icosphere(float r, PositionBuffer& pb, IndexBuffer& ib) {
    pb.clear();
    ib.clear();
    
    // top-most
    pb.push_back(glm::vec3(0.0f, 0.0f, r));

    // top layer
    float z = r * std::sin(std::atan(0.5));
    float xy = r * std::cos(std::atan(0.5));
    for (int i = 0; i < 5; ++i) {
        pb.push_back(glm::vec3(
                float(xy * std::cos(glm::radians(72.0 * i))),
                float(xy * std::sin(glm::radians(72.0 * i))),
                z));
    }
    
    // bottom layer
    for (int i = 0; i < 5; ++i) {
        pb.push_back(glm::vec3(
                float(xy * std::cos(glm::radians(36.0 + 72.0 * i))),
                float(xy * std::sin(glm::radians(36.0 + 72.0 * i))),
                -z));
    }
    
    // bottom-most
    pb.push_back(glm::vec3(0.0f, 0.0f, -r));

    // construct triangles
    // the 'roof'
    for (uint16_t i = 1; i <= 4; ++i) {
        ib.push_back({0, i, i + 1});
    }
    ib.push_back({0, 5, 1});
    
    // the 'walls'
    for (uint16_t i = 1; i <= 4; ++i) {
        ib.push_back({i, i + 5, i + 1});
    }
    ib.push_back({5, 10, 1});
    
    for (uint16_t i = 2; i <= 5; ++i) {
        ib.push_back({i, i + 4, i + 5});
    }
    ib.push_back({1, 10, 6});

    // the 'bottom'
    for (uint16_t i = 6; i <= 9; ++i) {
        ib.push_back({11, i + 1, i});
    }
    ib.push_back({11, 6, 10});
}

void ProceduralShapes::add_icosphere_lod(float r, PositionBuffer& pb, IndexBuffer& ib) {
    std::map<uint32_t, uint16_t> extrusion_lut;

    auto extrusion = [&](glm::vec3& p0, glm::vec3& p1) {
        glm::vec3 mid = (p0 + p1) * 0.5f;
        return r * glm::normalize(mid);
    };

    auto register_extrusion = [&](uint16_t i0, uint16_t i1) {
        uint32_t key = encode(i0, i1);
        auto iter = extrusion_lut.begin();
        if ((iter = extrusion_lut.find(key)) != extrusion_lut.end()) {
            // existing extrusion point
            return iter->second;
        } else {
            // new extrusion point
            glm::vec3 p0 = pb[i0];
            glm::vec3 p1 = pb[i1];
            extrusion_lut[key] = (uint16_t)pb.size();
            pb.push_back(extrusion(p0, p1));
            return extrusion_lut[key];
        }
    };


    // traverse triangles
    IndexBuffer ib_new;
    for (size_t k = 0; k < ib.size(); ++k) {
        auto& i = ib[k];

        uint16_t ext_i01 = register_extrusion(i[0], i[1]);
        uint16_t ext_i12 = register_extrusion(i[1], i[2]);
        uint16_t ext_i20 = register_extrusion(i[2], i[0]);
        ib_new.push_back(u16vec3(i[0], ext_i01, ext_i20));
        ib_new.push_back(u16vec3(i[1], ext_i12, ext_i01));
        ib_new.push_back(u16vec3(i[2], ext_i20, ext_i12));
        ib_new.push_back(u16vec3(ext_i20, ext_i01, ext_i12));
    }

    ib.clear();
    ib = std::move(ib_new);
}

void ProceduralShapes::tri2line(const IndexBuffer& src, std::vector<uint16_t>& dest) {
    // TODO: every line gets drawn twice. Fix this
    std::set<uint32_t> connectivity_lut;

    dest.clear();
    for (auto& i : src) {
        if (connectivity_lut.insert(encode(i.x, i.y)).second) {
            dest.insert(dest.end(), {i.x, i.y});
        }
        if (connectivity_lut.insert(encode(i.y, i.z)).second) {
            dest.insert(dest.end(), {i.y, i.z});
        }
        if (connectivity_lut.insert(encode(i.z, i.x)).second) {
            dest.insert(dest.end(), {i.z, i.x});
        }
    }
}

ProceduralShapes::ShapeData ProceduralShapes::assemble(const ShapeData& s1, const ShapeData& s2) {
    ShapeData result;
    auto& vec3_pb  = result.vec3_pb;
    auto& vec3_nb  = result.vec3_nb;
    auto& vec2_uvb = result.vec2_uvb;
    auto& vec3_tb  = result.vec3_tb;
    auto& vec3_ib  = result.vec3_ib;

    // copy position buffer
    vec3_pb.insert(vec3_pb.end(), s1.vec3_pb.begin(), s1.vec3_pb.end());
    vec3_pb.insert(vec3_pb.end(), s2.vec3_pb.begin(), s2.vec3_pb.end());

    //copy normal buffer
    vec3_nb.insert(vec3_nb.end(), s1.vec3_nb.begin(), s1.vec3_nb.end());
    vec3_nb.insert(vec3_nb.end(), s2.vec3_nb.begin(), s2.vec3_nb.end());

    //copy uv buffer
    vec2_uvb.insert(vec2_uvb.end(), s1.vec2_uvb.begin(), s1.vec2_uvb.end());
    vec2_uvb.insert(vec2_uvb.end(), s2.vec2_uvb.begin(), s2.vec2_uvb.end());

    //copy tangent buffer
    vec3_tb.insert(vec3_tb.end(), s1.vec3_tb.begin(), s1.vec3_tb.end());
    vec3_tb.insert(vec3_tb.end(), s2.vec3_tb.begin(), s2.vec3_tb.end());

    // concatenate index buffers
    vec3_ib.insert(vec3_ib.end(), s1.vec3_ib.begin(), s1.vec3_ib.end());
    vec3_ib.insert(vec3_ib.end(), s2.vec3_ib.begin(), s2.vec3_ib.end());
    uint16_t id_offset = (uint16_t)s1.vec3_pb.size();
    for (size_t i = s1.vec3_ib.size(); i < vec3_ib.size(); ++i) {
        vec3_ib[i] += u16vec3(id_offset);
    }

    return result;
}

ProceduralShapes::ShapeData ProceduralShapes::make_lid(const ShapeData& data,
                                                       size_t beg,
                                                       size_t end,
                                                       glm::vec3 center,
                                                       glm::vec3 normal,
                                                       bool ccw) {
    ShapeData lid_data;
    auto& vec3_pb  = lid_data.vec3_pb;
    auto& vec3_nb  = lid_data.vec3_nb;
    auto& vec2_uvb = lid_data.vec2_uvb;
    auto& vec3_tb  = lid_data.vec3_tb;
    auto& vec3_ib  = lid_data.vec3_ib;

    // position buffer
    for (size_t i = beg; i < end; ++i) {
        vec3_pb.push_back(data.vec3_pb[i]);
    }
    for (size_t i = beg; i < end - 1; ++i) {
        vec3_pb.push_back(center);
    }
    
    // normal buffer
    vec3_nb.insert(vec3_nb.end(), vec3_pb.size(), normal);

    // uv buffer
    for (size_t i = beg; i < end; ++i) {
        vec2_uvb.push_back(data.vec2_uvb[i]);
        vec2_uvb.back().y = 1.0f;
    }
    for (size_t i = beg; i < end - 1; ++i) {
        glm::vec2 center_uv = (data.vec2_uvb[i] + data.vec2_uvb[i + 1]) * 0.5f;
        center_uv.y = 0.0f;
        vec2_uvb.push_back(center_uv);
    }

    // tangent buffer
    for (size_t i = beg; i < end; ++i) {
        vec3_tb.push_back(data.vec3_tb[i]);
    }
    for (size_t i = beg; i < end - 1; ++i) {
        glm::vec3 center_t = (data.vec3_tb[i] + data.vec3_tb[i + 1]) * 0.5f;
        center_t = glm::normalize(center_t);
        vec3_tb.push_back(center_t);
    }

    // index buffer
    size_t n = end - beg;
    for (size_t i = 0; i < n - 1; ++i) {
        if (ccw) {
            vec3_ib.push_back(glm::vec3(i, i + 1, i + n));
        } else {
            vec3_ib.push_back(glm::vec3(i + 1, i, i + n));
        }
    }

    return lid_data;
}