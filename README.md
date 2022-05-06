1. cmake -Bbuild/release -DCMAKE_BUILD_TYPE=Release to build release version, good with renderdoc
2. build/vscode-cmaketools is actually debug build
3. use texturec to generate .ktx for cubemaps. Write this operation into cmakelists. texturev to view.
4. equirectangular map should have width twice as its height. If not, edit with pinta
5. strip cubmap?

