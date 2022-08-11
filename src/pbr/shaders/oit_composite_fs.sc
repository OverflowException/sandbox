$input v_frag_pos

#include <bgfx_shader.sh>

SAMPLER2D(s_accumu, 0);
SAMPLER2D(s_revealage, 1);

// epsilon number
const float EPSILON = 0.00001f;

void main() {
    vec2 uv = v_frag_pos.xy * vec2(0.5f) + vec2(0.5f);

	float revealage = texture2D(s_revealage, uv).r;
    if (revealage == 1.0f) {
        discard;;
    }

    vec4 accumulation = texture2D(s_accumu, uv);

    // TODO: prevent overflow

	// prevent floating point precision bug
	vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

	// blend pixels
	gl_FragColor = vec4(average_color, 1.0f - revealage);
}