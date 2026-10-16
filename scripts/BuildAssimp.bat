@echo off
pushd ..\
cmake -S Mayhem/vendor/assimp -B Mayhem/vendor/assimp/build ^
  -DASSIMP_BUILD_TESTS=OFF ^
  -DASSIMP_BUILD_ASSIMP_TOOLS=OFF ^
  -DASSIMP_BUILD_ZLIB=ON ^
  -DASSIMP_WARNINGS_AS_ERRORS=OFF ^
  -DUSE_STATIC_CRT=ON
cmake --build Mayhem/vendor/assimp/build --config Debug
cmake --build Mayhem/vendor/assimp/build --config Release
popd
pause