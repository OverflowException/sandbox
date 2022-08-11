// returns negative
/* essentially converts gl_FragCoord.z to (u_modelView * a_position).z
    the result of (u_modelViewProj * a_position) is called clip space coordinate
    what opengl rasterizer does behind the scene is converting clip space coordinate to window space coordinate i.e. gl_FragCoord
        1. gl_FragCoord.w = 1.0f / clip.w
        2. gl_FragCoord.z = clip.z / clip.w * 0.5f + 0.5f
*/
float depth_eye_space(vec4 wnd_coord, mat4 inv_proj) {
    float wnd_z = wnd_coord.z;
    float clip_w = 1.0f / wnd_coord.w;
    float clip_z = (wnd_z * 2.0f - 1.0f) * clip_w;
    vec4 clip_coord = vec4(0.0f, 0.0f, clip_z, clip_w);
    float depth = (inv_proj * clip_coord).z;
    return depth;
}

float distance_weight(vec4 color, float depth, int mode) {
    if (mode == 0) {
        return 1.0f;
    }

    float alpha = color.a;
    float depth_func = 0.0f;
    // shrink by 10 compared to weight functions proposed by essay, to prevent overflow
    if (mode == 1) {
        depth_func = 1.0f / (1e-5 + pow(depth / 5.0f, 2.0f) + pow(depth / 200.0f, 6.0f));
    } else if (mode == 2) {
        depth_func = 1.0f / (1e-5 + pow(depth / 10.0f, 3.0f) + pow(depth / 200.0f, 6.0f));
    } else if (mode == 3) {
        depth_func = 0.003f / (1e-5 + pow(depth / 200.0f, 4.0f));
    } else {
        depth_func = 1.0f / alpha;
    }

    float weight = alpha * clamp(depth_func, 0.001, 300.0);
    return weight;
}

