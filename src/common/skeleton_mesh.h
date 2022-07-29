#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>
#include "bgfx/bgfx.h"

namespace mdl {

struct Node {
    glm::vec3 t = glm::vec3(1.0f);
    glm::quat r = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 s;

    glm::mat4 node_space_mat  = glm::mat4(1.0f);
    glm::mat4 model_space_mat = glm::mat4(1.0f);

    std::string      name   = "";
    int              parent = -1;
    std::vector<int> children;
};

struct Material {
    std::string     name;
    glm::vec3       albedo      = glm::vec3(1.0f);
    float           metallic    = 0.1f;
    float           roughness   = 0.1f;
    std::string     shader_name   = "";
};

struct MeshPrimitive {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
    std::vector<glm::vec2> uvs;
    std::vector<uint16_t>  indices;
    int                    material_id;

    std::string name = "";
    int         node_id;
};

struct Mesh {
    std::string                 name;
    std::vector<MeshPrimitive>  primitives;

    // TODO: blend weights
};

struct SkeletonMesh {

    void submit_render();

    std::vector<Node>       nodes;
    std::vector<Mesh>       meshes;
    std::vector<Material>   materials;
};

}
