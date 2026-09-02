#pragma once

#include <glm/vec4.hpp>

#include <string>

namespace vkexp {

struct Preset {
    std::string name;
    std::string description;
    bool graphicsEnabled{true};
    bool computeEnabled{true};
    bool showDemoWindow{false};
    glm::vec4 clearColor{0.025F, 0.035F, 0.055F, 1.0F};
    unsigned int computeGroups{1};
};

} // namespace vkexp

