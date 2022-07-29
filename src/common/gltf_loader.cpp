#include <iostream>
#include <queue>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "gltf_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace io {

bool load_gltf(const std::string& name, mdl::SkeletonMesh& skel_mesh) {
    tinygltf::TinyGLTF gltf_ctx;
    tinygltf::Model gltf_model;;
    std::string gltf_err;
    std::string gltf_warn;
    bool gltf_ret = gltf_ctx.LoadASCIIFromFile(&gltf_model, &gltf_err, &gltf_warn, name);
    if (!gltf_ret) {
        std::cout << "Cannot open gltf file. File name =  " << name << std::endl;
        return false;
    }

    assert(gltf_model.scenes.size() == 1);
    assert(gltf_model.scenes[0].nodes.size() == 1);
    assert(gltf_model.scenes[0].nodes[0] == 0);

    load_nodes(gltf_model, skel_mesh);
    load_materials(gltf_model, skel_mesh);
    load_meshes(gltf_model, skel_mesh);

    return true;
}

void load_nodes(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh) {
    for (int i = 0; i < gltf_model.nodes.size(); ++i) {
        skel_mesh.nodes.emplace_back();
        mdl::Node& node = skel_mesh.nodes.back();
        tinygltf::Node& gltf_node = gltf_model.nodes[i];

        node.t = glm::make_vec3(gltf_node.translation.data());
        node.r = glm::make_quat(gltf_node.rotation.data());
        node.s = glm::make_vec3(gltf_node.scale.data());
        node.node_space_mat = glm::translate(node.t) * glm::toMat4(node.r) * glm::scale(node.s);
        node.name = gltf_node.name;
    }
    // build node hierarchy
    std::queue<int> q;
    q.push(0);
    while(!q.empty()) {
        int parent_id = q.front();
        q.pop();
        mdl::Node& parent = skel_mesh.nodes[parent_id];

        parent.children = gltf_model.nodes[parent_id].children;
        for (int child_id : gltf_model.nodes[parent_id].children) {
            q.push(child_id);
            mdl::Node& child = skel_mesh.nodes[parent_id];

            child.parent = parent_id;
            child.model_space_mat = parent.model_space_mat * child.node_space_mat;
        }
    }
}

void load_materials(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh) {
    for (int i = 0; i < gltf_model.materials.size(); ++i) {
        skel_mesh.materials.emplace_back();
        mdl::Material& material = skel_mesh.materials.back();
        tinygltf::Material& gltf_material = gltf_model.materials[i];
        material.name = gltf_material.name;

        tinygltf::PbrMetallicRoughness& gltf_pbr = gltf_material.pbrMetallicRoughness;
        material.albedo = glm::make_vec4(gltf_pbr.baseColorFactor.data());
        material.metallic = float(gltf_pbr.metallicFactor);
        material.roughness = float(gltf_pbr.roughnessFactor);
        material.shader_name = "pbr";
    }
}

template<typename T>
void parse_accesor(std::vector<T>& dst,
                   tinygltf::Model& model,
                   int acc_id,
                   int component_type,
                   int type) {
    tinygltf::Accessor& acc = model.accessors[acc_id];
    tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    tinygltf::Buffer& buf = model.buffers[bv.buffer];
    assert(acc.componentType == component_type);
    assert(acc.type == type);
    assert(bv.byteStride == 0);
    dst.resize(acc.count);
    memcpy(dst.data(),
           buf.data.data() + acc.byteOffset + bv.byteOffset,
           bv.byteLength);
}

void load_meshes(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh) {
    for (int i = 0; i < gltf_model.meshes.size(); ++i) {
        skel_mesh.meshes.emplace_back();
        mdl::Mesh& mesh = skel_mesh.meshes.back();
        tinygltf::Mesh& gltf_mesh = gltf_model.meshes[i];
        mesh.name = gltf_mesh.name;
        
        // parse every primitive
        for (auto& gltf_prim : gltf_mesh.primitives) {
            assert(gltf_prim.mode == TINYGLTF_MODE_TRIANGLES);
            mesh.primitives.emplace_back();
            mdl::MeshPrimitive& prim = mesh.primitives.back();
            std::map<std::string, int>& gltf_attrib = gltf_prim.attributes;

            // positions
            parse_accesor(prim.positions,
                          gltf_model,
                          gltf_attrib["POSITION"],
                          TINYGLTF_COMPONENT_TYPE_FLOAT,
                          TINYGLTF_TYPE_VEC3);

            // normals
            bool has_normal = gltf_attrib.find("NORMAL") != gltf_attrib.end();
            if (has_normal) {
                parse_accesor(prim.normals,
                              gltf_model,
                              gltf_attrib["NORMAL"],
                              TINYGLTF_COMPONENT_TYPE_FLOAT,
                              TINYGLTF_TYPE_VEC3);
            }

            // tangents
            bool has_tangent = gltf_attrib.find("TANGENT") != gltf_attrib.end();
            if (has_tangent) {
                std::vector<glm::vec4> tangents_vec4;
                parse_accesor(tangents_vec4,
                              gltf_model,
                              gltf_attrib["TANGENT"],
                              TINYGLTF_COMPONENT_TYPE_FLOAT,
                              TINYGLTF_TYPE_VEC4);
                prim.tangents.resize(tangents_vec4.size());
                prim.bitangents.resize(tangents_vec4.size());
                for (int i = 0; i < tangents_vec4.size(); ++i) {
                    prim.tangents[i] = glm::vec3(tangents_vec4[i]);
                    prim.bitangents[i] = glm::normalize(glm::cross(prim.normals[i],
                                                                   prim.tangents[i]));
                }
                
            }

            // uvs
            bool has_uv = gltf_attrib.find("TEXCOORD_0") != gltf_attrib.end();
            if (has_uv) {
                parse_accesor(prim.uvs,
                              gltf_model,
                              gltf_attrib["TEXCOORD_0"],
                              TINYGLTF_COMPONENT_TYPE_FLOAT,
                              TINYGLTF_TYPE_VEC2);
            }

            // indices
            parse_accesor(prim.indices,
                          gltf_model,
                          gltf_prim.indices,
                          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                          TINYGLTF_TYPE_SCALAR);

            // associate materials
            prim.material_id = gltf_prim.material;
        }
        
    }
}

} // namespace io
