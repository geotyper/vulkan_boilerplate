# Vulkan Experiment Framework Plan

## Purpose

Build a compact C++20 playground for Vulkan experiments. The program owns one
application loop and composes independent modules for rendering, compute work,
the ImGui interface, and startup presets.

## Architecture

```text
Application
  -> Window
  -> VulkanContext
  -> PresetRegistry
  -> ComputeModule
  -> GraphicsModule
  -> ImGuiModule
```

Every runtime subsystem implements the `Module` lifecycle:

1. `onAttach(AppContext&)` creates long-lived resources.
2. `onUpdate(AppContext&, FrameInfo)` advances per-frame state.
3. `onRender(AppContext&, FrameInfo)` records or submits its work.
4. `onDetach(AppContext&)` releases resources in reverse order.

The application owns modules, guarantees their lifetime, and provides shared
state through `AppContext`. Modules do not own the application or one another.
The graphics module publishes an off-screen image through `RenderViewport`;
the ImGui module displays it without owning the renderer.

## Initial milestones

- [x] Create the CMake project and source/include layout.
- [x] Add a GLFW application loop and module lifecycle.
- [x] Add a Vulkan context responsible for instance, surface, device, queues,
      swapchain, synchronization, and frame presentation.
- [x] Separate graphics and compute pipeline modules.
- [x] Add an ImGui module with Vulkan/GLFW backends.
- [x] Add named startup presets selected with `--preset`.
- [x] Render experiments into a resizable ImGui viewport texture.
- [x] Run a button-triggered compute blur into a second viewport texture.
- [x] Persist ImGui window layout and load the native window size from a preset.
- [ ] Add shader hot reload and runtime validation output.
- [ ] Add reusable descriptor and resource allocators.
- [ ] Add off-screen compute-to-graphics image experiments.
- [ ] Add automated rendering smoke tests.

## Frame flow

```text
poll events -> begin frame -> update modules -> acquire image
            -> render off-screen viewport
            -> optionally dispatch compute blur
            -> compose ImGui over background -> submit -> present
```

The first scaffold keeps GPU recording hooks explicit while avoiding a large
renderer abstraction too early. Experiments can extend a module or introduce a
new one without changing the core loop.

## Presets

Presets are data-only startup configurations. The initial registry contains:

- `graphics`: graphics enabled, compute disabled.
- `compute`: compute enabled, graphics disabled.
- `mixed`: both pipelines enabled (default).

Future presets may also choose shaders, dispatch dimensions, clear colours,
camera state, and module-specific parameters.

## Directory layout

```text
include/vkexp/
  core/       application, context, module, window
  graphics/   graphics pipeline module
  compute/    compute pipeline module
  ui/         ImGui module
  presets/    preset definitions and registry
src/          implementation files mirroring include/vkexp
shaders/      GLSL shader experiments
```

## Build strategy

CMake finds Vulkan and obtains GLFW, GLM, and Dear ImGui with `FetchContent`.
Validation layers are enabled in debug builds when available. Optional shader
targets use `glslc` when it is installed.
