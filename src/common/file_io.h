#pragma once

#include <array>
#include <string>

#include "bgfx/bgfx.h"

#include "bimg/bimg.h"
#include "bimg/decode.h"
#include "skeleton_mesh.h"

namespace io {

const bgfx::Memory* load_memory(const char* filename);

bgfx::ShaderHandle load_shader(const char* shader);

bgfx::ProgramHandle load_program(const char* vs_name, const char* fs_name);

void image_release_cb(void* _ptr, void* _userData);

bimg::ImageContainer* load_image_to_container(const std::string& name);

bgfx::TextureHandle load_texture_2d(const std::string& name);

// for 6 individual images.
// names order: +x, -x, +y, -y, +z, -z
bgfx::TextureHandle load_texture_cube(const std::array<std::string, 6>& names);

// for a single equirectangular map. ktx format only
bgfx::TextureHandle load_texture_cube_immutable_ktx(const std::string& name);

}