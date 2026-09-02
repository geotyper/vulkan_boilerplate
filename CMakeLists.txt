cmake_minimum_required(VERSION 3.24)

project(vulkan_boilerplate VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VKEXP_ENABLE_VALIDATION "Enable Vulkan validation layers in debug builds" ON)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/window.preset)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config)
configure_file(config/window.preset
               ${CMAKE_CURRENT_BINARY_DIR}/config/window.preset
               COPYONLY)

find_package(Vulkan 1.3 REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

set(SHADER_SOURCES
    triangle.vert
    triangle.frag
    experiment.comp
)

set(SHADER_BINARIES)
foreach(SHADER ${SHADER_SOURCES})
    set(SPIRV ${SHADER_BINARY_DIR}/${SHADER}.spv)
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SOURCE_DIR}/${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling ${SHADER}"
        VERBATIM
    )
    list(APPEND SHADER_BINARIES ${SPIRV})
endforeach()
add_custom_target(vkexp_shaders ALL DEPENDS ${SHADER_BINARIES})

add_executable(vulkan_boilerplate
    src/main.cpp
    src/core/Application.cpp
    src/core/VulkanContext.cpp
    src/core/Window.cpp
    src/graphics/GraphicsModule.cpp
    src/compute/ComputeModule.cpp
    src/ui/ImGuiModule.cpp
    src/presets/PresetRegistry.cpp
    src/profiling/CpuProfiler.cpp
    src/profiling/GpuProfiler.cpp
    src/profiling/ProfilerPanel.cpp
)
add_dependencies(vulkan_boilerplate vkexp_shaders)

target_include_directories(vulkan_boilerplate PRIVATE include)
target_link_libraries(vulkan_boilerplate PRIVATE Vulkan::Vulkan glfw glm::glm imgui)
target_compile_definitions(vulkan_boilerplate PRIVATE
    VKEXP_SHADER_DIR="${SHADER_BINARY_DIR}"
    VKEXP_WINDOW_PRESET="${CMAKE_CURRENT_BINARY_DIR}/config/window.preset"
    VKEXP_IMGUI_INI="${CMAKE_CURRENT_BINARY_DIR}/imgui.ini"
    $<$<AND:$<BOOL:${VKEXP_ENABLE_VALIDATION}>,$<CONFIG:Debug>>:VKEXP_ENABLE_VALIDATION>
)

if(MSVC)
    target_compile_options(vulkan_boilerplate PRIVATE /W4 /permissive-)
else()
    # Vulkan's C structs are conventionally zero-initialized with only sType
    # specified first, which otherwise produces hundreds of unhelpful warnings.
    target_compile_options(vulkan_boilerplate PRIVATE
        -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
    )
endif()

