@echo off

call "vcpkg/bootstrap-vcpkg.bat"

"vcpkg/vcpkg.exe" install glfw3:x64-windows-static
"vcpkg/vcpkg.exe" install glm:x64-windows-static

"vcpkg/vcpkg.exe" integrate project
