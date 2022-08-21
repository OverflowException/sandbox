int pick_cascade(vec4 thresholds_ndc, vec3 pos_ndc) {
    float z = pos_ndc.z;
    if (z >= thresholds_ndc.x && z <= thresholds_ndc.y) {
        return 0;
    } else if (z > thresholds_ndc.y && z <= thresholds_ndc.z) {
        return 1;
    } else if (z > thresholds_ndc.z && z <= thresholds_ndc.w) {
        return 2;
    } else {
        return 3;
    }
}

vec3 light_space_frag_ndc(mat4 light_view_proj, vec3 frag_pos_world) {
    vec4 frag_pos_clip = light_view_proj * vec4(frag_pos_world, 1.0f);
    frag_pos_clip = frag_pos_clip / vec4(frag_pos_clip.w);
    return vec3(frag_pos_clip);
}

bool in_shadow(sampler2D atlas,
               vec3 light_space_ndc,
               int atlas_id,
               int cascade_id,
               vec2 offset,
               float bias) {
    float frag_depth = light_space_ndc.z * 0.5f + 0.5f;
    vec2 atlas_uv = light_space_ndc.xy * vec2(0.5f) + vec2(0.5f);
    atlas_uv += offset;
    atlas_uv.x = (float(atlas_id) + atlas_uv.x) / float(MAX_LIGHT_COUNT);
    atlas_uv.y = (float(cascade_id) + atlas_uv.y) / float(CASCADE_COUNT);

    // float x_div = 1.0f / (1024.0f * float(atlas_count));
    // float y_div = 1.0f / (1024.0f * float(cascade_count));
    // atlas_uv.x = floor(atlas_uv.x / x_div) * x_div;
    // atlas_uv.y = floor(atlas_uv.y / y_div) * y_div;

    if(texture2D(atlas, atlas_uv).r < frag_depth - bias) {
        return true;
    } else {
        return false;
    }
}

float shadow_visibility_pcf(sampler2D atlas,
                            vec3 light_space_ndc,
                            vec3 l,
                            vec3 n,
                            int atlas_id,
                            int cascade_id) {
    float bias = max(0.01f * (1.0f - dot(l, n)), 0.005f);

    float visibility = 0.0f;
    float weight = 0.0f;
    vec2 offset = vec2(1.0f) / vec2(float(SHADOW_RESOLUTION));

    bias = 0.0f;

    // if (cascade_id >= 2) {
    //     visibility = in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, cascade_id, cascade_count, vec2(0.0f), bias) ? 0.0f : 1.0f;
    //     return visibility;
    // } else {
    //     visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	//     visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	//     visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	//     visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
    //     weight += 4.0f;
    //     if (cascade_id == 0) {
    //         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	//         visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
    //         weight += 12.0f;
    //     }
    // }
    // return visibility / weight;

    // visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(-0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	// visibility += in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2( 1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
    return visibility / 4.0f;

    // visibility = in_shadow(atlas, light_space_ndc, atlas_id, cascade_id, vec2(0.0f, 0.0f), 0.0f) ? 0.0f : 1.0f;
    // return visibility;
}

