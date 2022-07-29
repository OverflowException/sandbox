#pragma once

#include "skeleton_mesh.h"

namespace io {

bool load_gltf(const std::string& name, mdl::SkeletonMesh& skel_mesh);

void load_nodes(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh);

void load_materials(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh);

void load_meshes(tinygltf::Model& gltf_model, mdl::SkeletonMesh& skel_mesh);

} // namespace io
