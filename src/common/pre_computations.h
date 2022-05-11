#pragma once

#include <vector>

#include "bgfx/bgfx.h"

namespace pcp {

struct UniformContext {
	bgfx::UniformHandle hdl;
	void* value;
	uint16_t num;
};

bgfx::TextureHandle convolute_cube_map(bgfx::TextureHandle cube_tex,
										int res,
										bgfx::ProgramHandle prog,
										std::vector<UniformContext> ucs);


bgfx::TextureHandle gen_irradiance_map(bgfx::TextureHandle cube_tex, int res);

void blit_cube_map(bgfx::TextureHandle dst,
					int dst_mip,
					bgfx::TextureHandle src,
					int src_mip);

bgfx::TextureHandle gen_prefilter_map(bgfx::TextureHandle cube_tex,
										int res,
										int mip_levels);

bgfx::TextureHandle gen_brdf_lut(int res);

}