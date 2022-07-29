1. git submodule update --init --recursive
2. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release


NOTE:
1. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release to build release version, good with renderdoc
2. build/vscode-cmaketools is actually debug build
3. use texturec to generate .ktx for cubemaps. Write this operation into cmakelists. texturev to view.
4. equirectangular map should have width twice as its height. If not, edit with pinta
5. For metallicRoughnessTexture, its channels could be more commonly known as ORM: O--Occlusion--Red channel, R--Roughness--Green channel, M--Metallic--Blue channel

TODO:
1. brdf lut behaves weirdly. There's a little round artifact in the middle when rendering surfaces with low roughness
2. add mipmapping feature to IBL prefiltering. To solve the problem of dotty bright area in envronment map
3. HDR tonemapping pass in a separate pass. object + skybox

Document:
1. generate vertex tangent attribute on sphere
2. icosphere seam
3. brdf lut border clamping
