#include <bgfx_shader.sh>

void main() {
    gl_FragDepth = gl_FragCoord.z;
}