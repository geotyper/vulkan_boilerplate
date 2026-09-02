# vulkan_boilerplate

A modular C++20 playground for Vulkan graphics and compute experiments, using
GLFW, GLM, and Dear ImGui.

The Vulkan scene renders into an off-screen texture shown in the ImGui
**Viewport** window. The native window remains a simple background for the UI.
ImGui saves the positions and sizes of the **Controls**, **Viewport**, and
**Blur Output** windows to `build/imgui.ini`. Edit `config/window.preset` to
change the initial size of the native window.

The **Start** button dispatches a compute shader that applies a configurable
box blur to the current triangle texture. Its result appears in the independent
**Blur Output** window and remains unchanged until the next dispatch.

## Profiler

The persistent **Profiler** window reports CPU and GPU timings for the complete
frame, graphics pass, compute blur, and ImGui pass. Each metric keeps the latest
120 samples and exposes current, average, minimum, maximum, and p95 values.

GPU measurements use Vulkan timestamp queries and are resolved after the frame
fence without stalling the command stream. The panel reports when timestamps
are unsupported. CPU and GPU histories can be inspected independently from the
metric selector.

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

