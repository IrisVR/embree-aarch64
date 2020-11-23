
# Provenance
This is a fork of repo https://github.com/lighttransport/embree-aarch64.git.

# Building for ANDROID
Building Embree mostly follows the *How to build for Android arm64-v8a* README in the repo, with the following additions/modifications
(some needed just to setup yor build environment)
1. download cmake
1. download ninja and add to git bash path e.g. export PATH=$PATH:"/C/Program Files/Ninja/"
1. set env variable CMAKE_GENERATOR to Ninja
1. Edit Android NDK path in scripts/bootstrap-android-cmake-linux.sh to e.g. $HOME/AppData/Local/Android/Sdk/ndk-bundle
1. Add / Remove additional compile options in scripts/bootstrap-android-cmake-linux.sh

```
  -DEMBREE_RAY_MASK=ON \
  -DEMBREE_GEOMETRY_CURVE=OFF \
  -DEMBREE_GEOMETRY_GRID=OFF \
  -DEMBREE_GEOMETRY_POINT=OFF \
  -DEMBREE_GEOMETRY_QUAD=OFF \
  -DEMBREE_GEOMETRY_SUBDIVISION=OFF \
  -DEMBREE_GEOMETRY_USER=OFF \
  -DEMBREE_COMPACT_POLYS=ON \
```

# Building for WINDOWS
To build Embree for Windows follow the README in the repo that suggests to use cmake-gui to generate a Visual Studio 2019 solution.
Use cmake-gui also to add / remove the compile options as listed above

cmake-gui will complain about missing PNG and JPEG libraries but that's not an issue as we are not using any imaging part of embree for Edifce
