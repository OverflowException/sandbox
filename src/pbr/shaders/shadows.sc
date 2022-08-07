vec3 light_space_frag_ndc(mat4 light_view_proj, vec3 frag_pos_world) {
    vec4 frag_pos_clip = light_view_proj * vec4(frag_pos_world, 1.0f);
    frag_pos_clip = frag_pos_clip / vec4(frag_pos_clip.w);
    return vec3(frag_pos_clip);
}

bool in_shadow(sampler2D atlas,
               vec3 light_space_ndc,
               int atlas_id,
               int atlas_count,
               vec2 offset,
               float bias) {
    float frag_depth = light_space_ndc.z * 0.5f + 0.5f;
    vec2 atlas_uv = light_space_ndc.xy * vec2(0.5f) + vec2(0.5f);
    atlas_uv += offset;
    atlas_uv.x = (float(atlas_id) + atlas_uv.x) / float(atlas_count);

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
                            int atlas_count) {
    float bias = max(0.01f * (1.0f - dot(l, n)), 0.005f);

    float visibility = 0.0f;
    vec2 offset = vec2(1.0f) / vec2(1024.0f);

    visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2(-0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 0.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 0.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 0.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 0.5,  1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 1.5, -1.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 1.5, -0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 1.5,  0.5) * offset, bias) ? 0.0f : 1.0f;
	visibility += in_shadow(atlas, light_space_ndc, atlas_id, atlas_count, vec2( 1.5,  1.5) * offset, bias) ? 0.0f : 1.0f;

    return visibility / 16.0f;
}

