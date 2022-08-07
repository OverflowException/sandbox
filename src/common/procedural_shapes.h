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
    typedef std::vector<u16vec3> IndexBuffer;
    typedef std::vector<glm::vec3> TangentBuffer;
    struct ShapeData {
        PositionBuffer  vec3_pb;
        NormalBuffer    vec3_nb;
        UVBuffer        vec2_uvb;
        TangentBuffer   vec3_tb;
        IndexBuffer     vec3_ib;
    };
    
    static void gen_ico_sphere(std::vector<float>& vb,
                               std::vector<uint16_t>& ib,
                               VertexAttrib attrib,
                               float r,
                               int lod,
                               IndexType i_type);
    
    static void gen_z_cone(std::vector<float>& vb,
                           std::vector<uint16_t>& ib,
                           VertexAttrib attrib,
                           float r_top,
                           float r_bottom,
                           float height,
                           int sectors,
                           int stacks,
                           IndexType i_type);

    static void gen_z_quad(std::vector<float>& vb,
                           std::vector<uint16_t>& ib,
                           glm::vec2 half_dim);

    // Cube with hard edge
    // Geometry is so simple that it does not need index buffer
    static void gen_cube(std::vector<float>& vb,
                         VertexAttrib attrib,
                         glm::vec3 half_dim,
                         IndexType i_type);

private:
    
    static void init_icosphere(float r, PositionBuffer& pb, IndexBuffer& ib);
    
    static void add_icosphere_lod(float r, PositionBuffer& pb, IndexBuffer& ib);
    
    static void tri2line(const IndexBuffer& src, std::vector<uint16_t>& dest);

    static ShapeData assemble(const ShapeData& s1, const ShapeData& s2);

    static ShapeData make_lid(const ShapeData& data,
                              size_t beg,
                              size_t end,
                              glm::vec3 center,
                              glm::vec3 normal,
                              bool ccw);

    // encode 2 uint16_t. Interchangeable.
    static uint32_t encode(uint16_t a, uint16_t b) {
        return uint32_t(std::max(a, b)) * uint32_t((std::max(a, b)) + 1) / 2 + uint32_t(std::min(a, b));
    }
};
