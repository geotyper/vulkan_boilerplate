#include "vkexp/presets/PresetRegistry.hpp"

#include <stdexcept>
#include <string>

namespace vkexp {

PresetRegistry::PresetRegistry()
    : presets_{
          Preset{"graphics", "Animated graphics pipeline", true, false, false,
                 {0.015F, 0.025F, 0.06F, 1.0F}, 1},
          Preset{"compute", "Compute dispatch with a quiet background", false, true, false,
                 {0.025F, 0.025F, 0.025F, 1.0F}, 8},
          Preset{"mixed", "Graphics and compute pipelines together", true, true, false,
                 {0.025F, 0.035F, 0.055F, 1.0F}, 4},
      } {}

const Preset& PresetRegistry::require(const std::string_view name) const {
    for (const auto& preset : presets_) {
        if (preset.name == name) {
            return preset;
        }
    }
    throw std::runtime_error("Unknown preset: " + std::string{name});
}

} // namespace vkexp

