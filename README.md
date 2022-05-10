git submodule update --init --recursive
cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release


NOTE:
1. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release to build release version, good with renderdoc
2. build/vscode-cmaketools is actually debug build
3. use texturec to generate .ktx for cubemaps. Write this operation into cmakelists. texturev to view.
4. equirectangular map should have width twice as its height. If not, edit with pinta

TODO:
1. brdf lut behaves weirdly. Try generate it on the fly
2. add mipmapping feature to IBL prefiltering. TO solve the problem of dotty bright area in envronment map
3. HDR tonemapping pass



