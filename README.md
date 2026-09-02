# vulkan_boilerplate

A modular C++20 playground for Vulkan graphics and compute experiments, using
GLFW, GLM, and Dear ImGui.

## Build

Requirements: CMake 3.24+, a C++20 compiler, Vulkan 1.3 development files,
GLFW, GLM, `glslangValidator`, and a Vulkan-capable driver. Dear ImGui is pinned
and downloaded by CMake.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/vulkan_boilerplate --preset mixed
```

Available presets:

```bash
./build/vulkan_boilerplate --list-presets
```

Use `--no-validation` when the Vulkan validation layer is not installed. See
[PLAN.md](PLAN.md) for the architecture and roadmap.

