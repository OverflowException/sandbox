#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>

#include "file_io.h"

#include "bx/error.h"

namespace io {

// allocator
class Allocator : public bx::AllocatorI {
public:
	void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) {
		if (_size == 0) {
			free(_ptr);
			return nullptr;
		}
		else {
			return malloc(_size);
		}
	}
};

const bgfx::Memory* load_memory(const char* filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "Cannot open file " << filename << std::endl;
    }
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1));
	if (file.read((char*)mem->data, size)) {
		mem->data[mem->size - 1] = '\0';
		return mem;
	}
	return nullptr;
}

bgfx::ShaderHandle load_shader(const char* shader) {
	return bgfx::createShader(load_memory(shader));
}

bgfx::ProgramHandle load_program(const char* vs_name, const char* fs_name) {
	bgfx::ShaderHandle vs = load_shader(vs_name);
	bgfx::ShaderHandle fs = load_shader(fs_name);
	return bgfx::createProgram(vs, fs, true);
}

void image_release_cb(void* _ptr, void* _userData) {
	BX_UNUSED(_ptr);
	bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
	bimg::imageFree(imageContainer);
}

bimg::ImageContainer* load_image_to_container(const std::string& name) {
		std::ifstream ifs(name, std::ios::binary);
	if (!ifs.is_open()) {
		std::cout << "Cannot open texture file " << name << std::endl;
		return nullptr;
	}

	ifs.seekg(0, ifs.end);
	size_t size = static_cast<size_t>(ifs.tellg());
	if (size == 0) {
		std::cout << "Texture file " << name << " empty" << std::endl;
		return nullptr;
	}

	std::vector<char> buf(size);
	ifs.seekg(0, ifs.beg);
	ifs.read(buf.data(), static_cast<std::streamsize>(size));
	ifs.close();

	bx::AllocatorI* allocator = new Allocator;
	bx::Error err;
	bimg::ImageContainer* container = bimg::imageParse(allocator, buf.data(), (uint32_t)buf.size(), bimg::TextureFormat::Count, &err);

	if (!container) {
		std::cout << "Texture file " << name << ", parse error ";
		std::cout << err.getMessage().getPtr() << std::endl;
		return nullptr;
	}

	return container;
}

bgfx::TextureHandle load_texture_2d(const std::string& name) {
	bimg::ImageContainer* c = load_image_to_container(name);
	if (!c) {
		return BGFX_INVALID_HANDLE;
	}

	const bgfx::Memory* mem = bgfx::makeRef(c->m_data, c->m_size, image_release_cb, c);
	assert(c->m_numMips == 1);
	bgfx::TextureHandle handle = bgfx::createTexture2D(uint16_t(c->m_width),
														uint16_t(c->m_height),
														false,
														c->m_numLayers,
														bgfx::TextureFormat::Enum(c->m_format),
														0);
	bgfx::updateTexture2D(handle, c->m_numLayers, 0, 0, 0, c->m_width, c->m_height, mem);
	return handle;
}

// for 6 individual images.
// names order: +x, -x, +y, -y, +z, -z
bgfx::TextureHandle load_texture_cube(const std::array<std::string, 6>& names) {
	uint8_t side_order[6] = {
		BGFX_CUBE_MAP_POSITIVE_X, BGFX_CUBE_MAP_NEGATIVE_X,
		BGFX_CUBE_MAP_POSITIVE_Y, BGFX_CUBE_MAP_NEGATIVE_Y,
		BGFX_CUBE_MAP_POSITIVE_Z, BGFX_CUBE_MAP_NEGATIVE_Z,
	};

	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	for (int i = 0; i < 6; ++i) {
		bimg::ImageContainer* c = load_image_to_container(names[i]);
		if (!c) {
			return BGFX_INVALID_HANDLE;
		}

		assert(c->m_numMips == 1);
		if (!bgfx::isValid(handle)) {
			// The first side
			handle = bgfx::createTextureCube(uint16_t(c->m_width),
											false,
											c->m_numLayers,
											bgfx::TextureFormat::Enum(c->m_format),
											BGFX_SAMPLER_UVW_CLAMP);
		}

		const bgfx::Memory* mem = bgfx::makeRef(c->m_data, c->m_size, image_release_cb, c);
		bgfx::updateTextureCube(handle, c->m_numLayers, side_order[i], 0, 0, 0, c->m_width, c->m_height, mem);
	}

	return handle;
}

// for a single equirectangular map. ktx format only
bgfx::TextureHandle load_texture_cube_immutable_ktx(const std::string& name) {
	bimg::ImageContainer* c = load_image_to_container(name);
	if (!c) {
		return BGFX_INVALID_HANDLE;
	}
	assert(c->m_numMips == 1);
	assert(c->m_cubeMap); // this will in fact check if image is in .ktx format. .png will fail here

	const bgfx::Memory* mem = bgfx::makeRef(c->m_data, c->m_size, image_release_cb, c);
	bgfx::TextureHandle handle = bgfx::createTextureCube(uint16_t(c->m_width),
														false,
														c->m_numLayers,
														bgfx::TextureFormat::Enum(c->m_format),
														BGFX_SAMPLER_UVW_CLAMP,
														mem);
	return handle;
}
}