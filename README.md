# FFPlay-Cpp
This is a (modern) C++ rewrite of the original [ffplay](https://ffmpeg.org/ffplay.html).

## Why?
Because the original ffplay is a mess (which might intended to be so), which makes reference to a minimal video player quite hard (at least to me).

## Build
- CMake >= 3.28 that has proper C++20 modules support
- SDL2 (just like the original ffplay, and you can get from vcpkg)
- ffmpeg libraries (you can get from vcpkg)
- A C++23 compatible C++ building system.
I recommend getting them prepared with [vcpkg](vcpkg.io).

Development are done on Windows 10 with Visual Studio 2026.

