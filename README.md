1. git submodule update --init --recursive
2. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release


NOTE:
1. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release to build release version, good with renderdoc
2. build/vscode-cmaketools is actually debug build
3. use texturec to generate .ktx for cubemaps. Write this operation into cmakelists. texturev to view.
4. equirectangular map should have width twice as its height. If not, edit with pinta

TODO:
1. brdf lut behaves weirdly. There's a little round artifact in the middle when rendering surfaces with low roughness (solved, with texture wrapping)
2. add mipmapping feature to IBL prefiltering. To solve the problem of dotty bright area in envronment map
3. HDR tonemapping pass in a separate pass. object + skybox
4. try different searching technique for POM, linear search shows layered artifacts
5. Bitangent could be pre-computed as an attribute. Does not have to be computed in shader

Document:
1. generate vertex tangent attribute on sphere
2. icosphere seam
3. brdf lut border clamping
4. parallax mapping step number's impact
5. prism-based parallax mapping. With demonstrations showing ray length map, uv map, and artifacts

