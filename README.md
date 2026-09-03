# vulkan_boilerplate

A modular C++20 playground for Vulkan graphics and compute experiments, using
GLFW, GLM, and Dear ImGui.

The build is split into reusable targets:

- `vkexp_core`: window, Vulkan context, frame loop, module lifecycle, and
  reusable Vulkan resources;
- `vkexp_profiling`: registered CPU/GPU timing metrics;
- `vkexp_imgui`: the generic GLFW/Vulkan ImGui backend and profiler panel;
- `vkexp_demo`: the triangle, compute blur, presets, and demo UI;
- `vulkan_boilerplate`: the small composition root in `src/main.cpp`.

`Application` does not select modules. A derived project creates them in its
composition root and adds them with `Application::addModule()`. Shared
experiment data lives in `DemoState`, outside the core lifecycle API.

The Vulkan scene renders into an off-screen texture shown in the ImGui
**Viewport** window. The native window remains a simple background for the UI.
ImGui saves the positions and sizes of the **Controls**, **Viewport**, and
**Blur Output** windows to the active build directory's `imgui.ini`. Edit
`config/window.preset` to change the initial size of the native window.

The **Start** button dispatches a compute shader that applies a configurable
box blur to the current triangle texture. Its result appears in the independent
**Blur Output** window and remains unchanged until the next dispatch.

## Profiler

The persistent **Profiler** window reports CPU and GPU timings for the complete
frame and metrics registered by the active modules, including graphics,
compute blur, ImGui, and demo UI. Each metric keeps the latest 120 samples and
exposes current, average, minimum, maximum, and p95 values.

GPU measurements use Vulkan timestamp queries and are resolved after the frame
fence without stalling the command stream. The panel reports when timestamps
are unsupported. CPU and GPU histories can be inspected independently from the
metric selector.

## Build

Requirements: CMake 3.24+, a C++20 compiler, Vulkan 1.3 development files,
GLFW, GLM, Ninja, `glslangValidator`, and a Vulkan-capable driver. GLFW and GLM
are found as system packages. Dear ImGui is pinned and downloaded by CMake.

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/vulkan_boilerplate --preset mixed
```

Available presets:

```bash
./build/debug/vulkan_boilerplate --list-presets
```

Debug builds enable validation when the layer is installed and print warnings
and errors through `VK_EXT_debug_utils`. Use `--no-validation` to disable it.

## Creating a specialized project

The recommended workflow keeps the boilerplate as an upstream remote. First
create an empty repository for the new project, without an automatically
generated README or license, then run:

```bash
git clone --branch main --single-branch \
  git@github.com:geotyper/vulkan_boilerplate.git my_new_project
cd my_new_project
git remote rename origin boilerplate
git remote add origin git@github.com:geotyper/my_new_project.git
git push -u origin main
```

Future boilerplate fixes can then be inspected and integrated explicitly:

```bash
git fetch boilerplate
git log --oneline main..boilerplate/main
git cherry-pick <commit>
```

Changing the clone directory alone does not rename the CMake project. For a
fully independent product, update the project and executable names in
`CMakeLists.txt`, the title in `src/main.cpp`, and the commands in this README.
The `vkexp` namespace and target prefix may stay unchanged when they identify
the embedded framework.

For a repository with fresh history, enable GitHub's “Template repository”
setting and create the new repository from the template after the desired
boilerplate changes have reached `main`.

See [PLAN.md](PLAN.md) for the architecture and remaining roadmap.

