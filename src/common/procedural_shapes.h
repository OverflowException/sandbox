#pragma once

#include <glm/vec3.hpp>

#include <vector>

class ProceduralShapes {
public:
    enum IndexType {
        LINE,
        TRIANGLE
    };

    enum VertexAttrib {
        POS      = 0x01,
        NORM     = 0x02,
        UV       = 0x04,
        TANGENT  = 0x08,
        POS_NORM = 0x03,
        POS_NORM_UV = 0x07,
        POS_NORM_UV_TANGENT = 0x0f
    };
    
    typedef glm::tvec3<uint16_t, glm::highp> u16vec3;
    typedef std::vector<glm::vec3> PositionBuffer;
    typedef std::vector<glm::vec3> NormalBuffer;
    typedef std::vector<glm::vec2> UVBuffer;
    typedef std::vector<glm::vec3> TangentBuffer;
    typedef std::vector<u16vec3> IndexBuffer;

    struct BufData {
        PositionBuffer pb;
        NormalBuffer nb;
        UVBuffer uvb;
        TangentBuffer tb;
        IndexBuffer ib;
    };
    
    static void gen_ico_sphere(std::vector<float>& vb,
                               std::vector<uint16_t>& ib,
                               VertexAttrib attrib,
                               float r,
                               int lod,
                               IndexType i_type,
                               BufData* data = nullptr);
    
    static void gen_z_cylinder(std::vector<float>& pb,
                               std::vector<uint16_t>& ib,
                               float r,
                               float height,
                               int sectors,
                               int stacks,
                               IndexType i_type);
    
    static void gen_z_capsule(std::vector<float>& pb,
                              std::vector<uint16_t>& ib,
                              float r,
                              float height,
                              int sectors,
                              int stacks,
                              IndexType i_type);

    // Cube with hard edge
    // Geometry is so simple that it does not need index buffer
    static void gen_cube(std::vector<float>& vb,
                         VertexAttrib attrib,
                         glm::vec3 half_dim,
                         IndexType i_type);

    static void displace_prism(std::vector<float>& vb,
                               std::vector<uint16_t>& ib,
                               const BufData& buf);
    
    
private:
    
    static void init_icosphere(float r, PositionBuffer& pb, IndexBuffer& ib);
    
    static void add_icosphere_lod(float r, PositionBuffer& pb, IndexBuffer& ib);
    
    static void z_cylinder(PositionBuffer& pb, IndexBuffer& ib, float r, float height, int sectors, int stacks);
    
    static void morph_cylinder2hemisphere(PositionBuffer::iterator beg,
                                          glm::vec3 center,
                                          float r,
                                          int sectors,
                                          int stacks,
                                          float phi_start,
                                          float phi_end);
    
    static glm::vec3 extrude(float r, const glm::vec3& p0, const glm::vec3& p1);
    
    static void tri2line(const IndexBuffer& src, std::vector<uint16_t>& dest);
};
