#include "procedural_shapes.h"

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
                                      IndexType i_type,
                                      BufData* buf_data) {
    PositionBuffer vec3_pb;
    NormalBuffer vec3_nb;
    UVBuffer vec2_uv;
    TangentBuffer vec3_tb;
    IndexBuffer vec3_ib;
    
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
            vec2_uv.push_back(glm::vec2(u, v));
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
                        glm::vec2& uv = vec2_uv[*i_ptr];
                        if (p.y < 0.0f && p.z != r && p.z != -r) {
                            // leave out the top and bottom point
                            vec3_pb.push_back(p);
                            vec3_nb.push_back(n);
                            vec2_uv.emplace_back(uv.x + 1.0f, uv.y);
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
                        glm::vec2& uv = vec2_uv[*i_ptr];
                        if (p.y == 0.0f && p.z != r && p.z != -r) {
                            // leave out the top and bottom point
                            vec3_pb.push_back(p);
                            vec3_nb.push_back(n);
                            vec2_uv.emplace_back(uv.x - 1.0f, uv.y);
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
            glm::vec2& uv0 = vec2_uv[i.x];
            glm::vec2& uv1 = vec2_uv[i.y];
            glm::vec2& uv2 = vec2_uv[i.z];

            if (p0.z == r || p0.z == -r) {
                vec3_pb.push_back(p0);
                vec3_nb.push_back(vec3_nb[i.x]);
                vec2_uv.emplace_back((uv1.x + uv2.x) / 2, uv0.y);
                i.x = vec3_pb.size() - 1;
            } else if (p1.z == r || p1.z == -r) {
                vec3_pb.push_back(p1);
                vec3_nb.push_back(vec3_nb[i.y]);
                vec2_uv.emplace_back((uv0.x + uv2.x) / 2, uv1.y);
                i.y = vec3_pb.size() - 1;
            } else if (p2.z == r || p2.z == -r) {
                vec3_pb.push_back(p2);
                vec3_nb.push_back(vec3_nb[i.z]);
                vec2_uv.emplace_back((uv0.x + uv1.x) / 2, uv2.y);
                i.z = vec3_pb.size() - 1;
            }
        }
    }

    if (attrib & VertexAttrib::TANGENT) {
        for (int i = 0; i < vec3_pb.size(); ++i) {
            const glm::vec2& uv = vec2_uv[i];
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

    // user needs intermediate data
    if (buf_data) {
        buf_data->attrib = attrib;
        buf_data->pb = std::move(vec3_pb);
        buf_data->nb = std::move(vec3_nb);
        buf_data->uvb = std::move(vec2_uv);
        buf_data->tb = std::move(vec3_tb);
        buf_data->ib = std::move(vec3_ib);
        return;
    }

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
            vb.insert(vb.end(), {vec2_uv[i].x * 4.0f, vec2_uv[i].y * 2.0f});
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

void ProceduralShapes::gen_z_cylinder(std::vector<float>& pb,
                                      std::vector<uint16_t>& ib,
                                      float r,
                                      float height,
                                      int sectors,
                                      int stacks,
                                      IndexType i_type) {
    PositionBuffer vec3_pb;
    IndexBuffer vec3_ib;
    z_cylinder(vec3_pb, vec3_ib, r, height, sectors, stacks);
    
    // copy to pb, ib
    pb.resize(vec3_pb.size() * 3);
    memcpy(pb.data(), vec3_pb.data(), pb.size() * sizeof(float));
    
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

void ProceduralShapes::gen_z_capsule(std::vector<float>& pb,
                                     std::vector<uint16_t>& ib,
                                     float r,
                                     float height,
                                     int sectors,
                                     int stacks,
                                     IndexType i_type) {
    PositionBuffer vec3_pb;
    IndexBuffer vec3_ib;
    
    z_cylinder(vec3_pb, vec3_ib, r, 3 * height, sectors, 3 * stacks);
    
    float half_height = height / 2;
    float phi_end = M_PI_2;
    float phi_start = M_PI_2 / (stacks + 1);
    // morph bottom
    morph_cylinder2hemisphere(vec3_pb.begin(),
                              glm::vec3(0.0, 0.0, -half_height),
                              r,
                              sectors,
                              stacks,
                              M_PI - phi_start,
                              M_PI - phi_end);
    
    // morph dome
    morph_cylinder2hemisphere(vec3_pb.begin() + 2 * sectors * stacks,
                              glm::vec3(0.0, 0.0, half_height),
                              r,
                              sectors,
                              stacks,
                              phi_end,
                              phi_start);
    
    // center of top and bottom
    glm::vec3& top = vec3_pb.back();
    glm::vec3& bottom = *(vec3_pb.end() - 2);
    top.z = half_height + r;
    bottom.z = -top.z;
    
    // copy to pb, ib
    pb.resize(vec3_pb.size() * 3);
    memcpy(pb.data(), vec3_pb.data(), pb.size() * sizeof(float));
    
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
    // traverse triangles
    IndexBuffer ib_new;
    for (size_t k = 0; k < ib.size(); ++k) {
        auto& i = ib[k];

        glm::vec3 p0 = pb[i.x];
        glm::vec3 p1 = pb[i.y];
        glm::vec3 p2 = pb[i.z];
        
        glm::vec3 p01 = extrude(r, p0, p1);
        glm::vec3 p12 = extrude(r, p1, p2);
        glm::vec3 p20 = extrude(r, p2, p0);
        
        // add new vertices to vb
        pb.push_back(p01);
        pb.push_back(p12);
        pb.push_back(p20);
        
        // add new indices
        uint16_t pos01 = pb.size() - 3;
        uint16_t pos12 = pb.size() - 2;
        uint16_t pos20 = pb.size() - 1;
        ib_new.push_back(glm::vec3(i.x, pos01, pos20));
        ib_new.push_back(glm::vec3(i.y, pos12, pos01));
        ib_new.push_back(glm::vec3(i.z, pos20, pos12));
        ib_new.push_back(glm::vec3(pos20, pos01, pos12));
    }

    ib.clear();
    ib = std::move(ib_new);
}

glm::vec3 ProceduralShapes::extrude(float r, const glm::vec3& p0, const glm::vec3& p1) {
    glm::vec3 mid = (p0 + p1) * 0.5f;
    return r * normalize(mid);
}

void ProceduralShapes::tri2line(const IndexBuffer& src, std::vector<uint16_t>& dest) {
    // TODO: every line gets drawn twice. Fix this
    dest.clear();
    for (auto& i : src) {
        dest.insert(dest.end(), {
            i.x, i.y,
            i.y, i.z,
            i.z, i.x
        });
    }
}

void ProceduralShapes::morph_cylinder2hemisphere(PositionBuffer::iterator beg,
                                                 glm::vec3 center,
                                                 float r,
                                                 int sectors,
                                                 int stacks,
                                                 float phi_start,
                                                 float phi_end) {
    // v -- axial index  -- stacks
    // u -- radial index -- sectors
    float delta_phi = (phi_end - phi_start) / stacks;
    float phi = phi_start;
    for (uint16_t v = 0; v <= stacks; ++v, phi += delta_phi) {
        for (uint16_t u = 0; u < sectors; ++u) {
            float sin_phi = sin(phi);
            glm::vec3& p = *(beg + v * sectors + u);
            p.x = (p.x - center.x) * sin_phi + center.x;
            p.y = (p.y - center.y) * sin_phi + center.y;
            p.z = r * cos(phi) + center.z;
        }
    }
}

void ProceduralShapes::z_cylinder(PositionBuffer& pb, IndexBuffer& ib, float r, float height, int sectors, int stacks) {
    float delta_phi = M_PI * 2.0 / sectors;
    float delta_z = height / stacks;
    
    // compute vertex positions
    float phi = 0.0;
    float half_height = height / 2;
    float z = -half_height;
    for (int sta = 0; sta <= stacks; ++sta, z += delta_z) {
        for (int sec = 0; sec < sectors; ++sec, phi += delta_phi) {
            pb.push_back({r * cos(phi), r * sin(phi), z});
        }
    }
    // center of top and bottom
    pb.push_back({0.0, 0.0, -half_height});
    pb.push_back({0.0, 0.0, half_height});
    
    
    // set indices
    // v -- axial index  -- stacks
    // u -- radial index -- sectors
    //  p01 -- p11
    //   |    / |
    //   |   /  |
    //   |  /   |
    //   p00 -- p10
    for (uint16_t v = 0; v < stacks; ++v) {
        for (uint16_t u = 1; u <= sectors; ++u) {
            uint16_t i10 = v * sectors + u;
            uint16_t i00 = i10 - 1;
            uint16_t i01 = i00 + sectors;
            uint16_t i11 = i01 + 1;
            
            // loop back
            if (u == sectors) {
                i10 -= sectors;
                i11 -= sectors;
            }
            
            ib.push_back({i00, i11, i01});
            ib.push_back({i00, i10, i11});
        }
    }
    
    //        top
    //       /  |
    //      /   |
    //     /    |
    //   p0 --  p1
    //     \    |
    //      \   |
    //       \  |
    //      bottom
    uint16_t i_top = pb.size() - 1;
    uint16_t i_bottom = pb.size() - 2;
    for (uint16_t u = 1; u <= sectors; ++u) {
        // bottom
        uint16_t i1 = u;
        uint16_t i0 = u - 1;
        if (u == sectors) {
            i1 -= sectors;
        }
        ib.push_back({i_bottom, i1, i0});
        
        // top
        i1 += stacks * sectors;
        i0 += stacks * sectors;
        ib.push_back({i_top, i0, i1});
    }
}

void ProceduralShapes::displace_prism(std::vector<float>& vb,
                                      std::vector<uint16_t>& ib,
                                      float height,
                                      const BufData& buf) {
    assert(buf.attrib == VertexAttrib::POS_NORM_UV_TANGENT);

    // TODO: uint16_t index might not be sufficient
    PositionBuffer dst_pb;
    NormalBuffer dst_nb;
    UVWBuffer dst_uvwb;
    TangentBuffer dst_tb;
    IndexBuffer dst_ib;

    PositionBuffer dst_displaced_pb;
    UVWBuffer dst_displaced_uvwb;

    PositionBuffer prism_bottom_pb;
    PositionBuffer prism_top_pb;
    UVBuffer prism_uvb;

    const PositionBuffer& src_pb = buf.pb;
    const NormalBuffer& src_nb = buf.nb;
    const UVBuffer& src_uvb = buf.uvb;
    const TangentBuffer& src_tb = buf.tb;
    const IndexBuffer& src_ib = buf.ib;

    for (const u16vec3& i : src_ib) {
        // positions of top slab vertices
        uint16_t i0 = i.x;
        uint16_t i1 = i.y;        
        uint16_t i2 = i.z;
        const glm::vec3& p0 = src_pb[i0];
        const glm::vec3& p1 = src_pb[i1];
        const glm::vec3& p2 = src_pb[i2];

        const glm::vec3& n0 = src_nb[i0];
        const glm::vec3& n1 = src_nb[i1];
        const glm::vec3& n2 = src_nb[i2];

        const glm::vec3& uvw0 = glm::vec3(src_uvb[i0], 0.0f);
        const glm::vec3& uvw1 = glm::vec3(src_uvb[i1], 0.0f);
        const glm::vec3& uvw2 = glm::vec3(src_uvb[i2], 0.0f);

        const glm::vec3& t0 = src_tb[i0];
        const glm::vec3& t1 = src_tb[i1];
        const glm::vec3& t2 = src_tb[i2];

        glm::vec3 displaced_p0 = p0 + height * n0;
        glm::vec3 displaced_p1 = p1 + height * n1;
        glm::vec3 displaced_p2 = p2 + height * n2;

        glm::vec3 displaced_uvw0 = uvw0 + glm::vec3(0.0f, 0.0f, height);
        glm::vec3 displaced_uvw1 = uvw1 + glm::vec3(0.0f, 0.0f, height);
        glm::vec3 displaced_uvw2 = uvw2 + glm::vec3(0.0f, 0.0f, height);

        dst_pb.insert(dst_pb.end(), {p0, p1, p2});
        dst_nb.insert(dst_nb.end(), {n0, n1, n2});
        dst_uvwb.insert(dst_uvwb.end(), {uvw0, uvw1, uvw2});
        dst_tb.insert(dst_tb.end(), {t0, t1, t2});
        dst_displaced_pb.insert(dst_displaced_pb.end(), {displaced_p0, displaced_p1, displaced_p2});
        dst_displaced_uvwb.insert(dst_displaced_uvwb.end(), {displaced_uvw0, displaced_uvw1, displaced_uvw2});
        prism_bottom_pb.insert(prism_bottom_pb.end(), {p0, p1, p2});
        prism_top_pb.insert(prism_top_pb.end(), {displaced_p0, displaced_p1, displaced_p2});
        prism_uvb.insert(prism_uvb.end(), {glm::vec2(uvw0), glm::vec2(uvw1), glm::vec2(uvw2)});

        uint16_t dst_i0 = (uint16_t(dst_pb.size()) - 3) * 2;
        uint16_t dst_i1 = dst_i0 + 1;
        uint16_t dst_i2 = dst_i0 + 2;
        uint16_t dst_i3 = dst_i0 + 3;
        uint16_t dst_i4 = dst_i0 + 4;
        uint16_t dst_i5 = dst_i0 + 5;
        // slabs
        dst_ib.emplace_back(dst_i0, dst_i2, dst_i1);
        dst_ib.emplace_back(dst_i3, dst_i4, dst_i5);
        // walls
        dst_ib.emplace_back(dst_i0, dst_i1, dst_i4);
        dst_ib.emplace_back(dst_i0, dst_i4, dst_i3);
        dst_ib.emplace_back(dst_i0, dst_i3, dst_i5);
        dst_ib.emplace_back(dst_i0, dst_i5, dst_i2);
        dst_ib.emplace_back(dst_i1, dst_i2, dst_i5);
        dst_ib.emplace_back(dst_i1, dst_i5, dst_i4);
    }

    // copy to ib
    for (auto& i : dst_ib) {
        ib.insert(ib.end(), { i.x, i.y, i.z });
    }

    // copy attributes to vb
    size_t v_size = dst_pb.size();
    assert(v_size % 3 == 0);
    for (int i = 0; i < v_size; i += 3) {
        // bottom slab, original
        for (int j = 0; j < 3; ++j) {
            int k = i + j;
            vb.insert(vb.end(), {dst_pb[k].x, dst_pb[k].y, dst_pb[k].z});
            vb.insert(vb.end(), {dst_nb[k].x, dst_nb[k].y, dst_nb[k].z});
            vb.insert(vb.end(), {dst_uvwb[k].x, dst_uvwb[k].y, dst_uvwb[k].z});
            vb.insert(vb.end(), {dst_tb[k].x, dst_tb[k].y, dst_tb[k].z});

            vb.insert(vb.end(), {prism_bottom_pb[i + 0].x, prism_bottom_pb[i + 0].y, prism_bottom_pb[i + 0].z});
            vb.insert(vb.end(), {prism_bottom_pb[i + 1].x, prism_bottom_pb[i + 1].y, prism_bottom_pb[i + 1].z});
            vb.insert(vb.end(), {prism_bottom_pb[i + 2].x, prism_bottom_pb[i + 2].y, prism_bottom_pb[i + 2].z});

            vb.insert(vb.end(), {prism_top_pb[i + 0].x, prism_top_pb[i + 0].y, prism_top_pb[i + 0].z});
            vb.insert(vb.end(), {prism_top_pb[i + 1].x, prism_top_pb[i + 1].y, prism_top_pb[i + 1].z});
            vb.insert(vb.end(), {prism_top_pb[i + 2].x, prism_top_pb[i + 2].y, prism_top_pb[i + 2].z});
            
            vb.insert(vb.end(), {prism_uvb[i + 0].x, prism_uvb[i + 0].y});
            vb.insert(vb.end(), {prism_uvb[i + 1].x, prism_uvb[i + 1].y});
            vb.insert(vb.end(), {prism_uvb[i + 2].x, prism_uvb[i + 2].y});
        }
        // top slab, displaced
        for (int j = 0; j < 3; ++j) {
            int k = i + j;
            vb.insert(vb.end(), {dst_displaced_pb[k].x, dst_displaced_pb[k].y, dst_displaced_pb[k].z});
            vb.insert(vb.end(), {dst_nb[k].x, dst_nb[k].y, dst_nb[k].z});
            vb.insert(vb.end(), {dst_displaced_uvwb[k].x, dst_displaced_uvwb[k].y, dst_displaced_uvwb[k].z});
            vb.insert(vb.end(), {dst_tb[k].x, dst_tb[k].y, dst_tb[k].z});

            vb.insert(vb.end(), {prism_bottom_pb[i + 0].x, prism_bottom_pb[i + 0].y, prism_bottom_pb[i + 0].z});
            vb.insert(vb.end(), {prism_bottom_pb[i + 1].x, prism_bottom_pb[i + 1].y, prism_bottom_pb[i + 1].z});
            vb.insert(vb.end(), {prism_bottom_pb[i + 2].x, prism_bottom_pb[i + 2].y, prism_bottom_pb[i + 2].z});

            vb.insert(vb.end(), {prism_top_pb[i + 0].x, prism_top_pb[i + 0].y, prism_top_pb[i + 0].z});
            vb.insert(vb.end(), {prism_top_pb[i + 1].x, prism_top_pb[i + 1].y, prism_top_pb[i + 1].z});
            vb.insert(vb.end(), {prism_top_pb[i + 2].x, prism_top_pb[i + 2].y, prism_top_pb[i + 2].z});
            
            vb.insert(vb.end(), {prism_uvb[i + 0].x, prism_uvb[i + 0].y});
            vb.insert(vb.end(), {prism_uvb[i + 1].x, prism_uvb[i + 1].y});
            vb.insert(vb.end(), {prism_uvb[i + 2].x, prism_uvb[i + 2].y});
        }
    }
}


