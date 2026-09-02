#include "vkexp/core/Application.hpp"
#include "vkexp/presets/PresetRegistry.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void printHelp(const char* executable) {
    std::cout << "Usage: " << executable << " [--preset NAME] [--no-validation]\n"
              << "       " << executable << " --list-presets\n";
}

} // namespace

int main(const int argc, char** argv) {
    try {
        vkexp::PresetRegistry presets;
        std::string presetName = "mixed";
#ifdef VKEXP_ENABLE_VALIDATION
        bool validationEnabled = true;
#else
        bool validationEnabled = false;
#endif

        for (int i = 1; i < argc; ++i) {
            const std::string_view argument = argv[i];
            if (argument == "--preset") {
                if (++i >= argc) {
                    throw std::runtime_error("--preset requires a name");
                }
                presetName = argv[i];
            } else if (argument == "--no-validation") {
                validationEnabled = false;
            } else if (argument == "--list-presets") {
                for (const auto& preset : presets.all()) {
                    std::cout << preset.name << "\t" << preset.description << '\n';
                }
                return 0;
            } else if (argument == "--help" || argument == "-h") {
                printHelp(argv[0]);
                return 0;
            } else {
                throw std::runtime_error("Unknown argument: " + std::string{argument});
            }
        }

        presets.loadWindowPreset(VKEXP_WINDOW_PRESET);
        vkexp::Application app{presets.require(presetName), validationEnabled};
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

