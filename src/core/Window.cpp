#include "vkexp/core/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <string>

namespace vkexp {

Window::Window(const int width, const int height, const std::string_view title) {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("GLFW initialization failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    const std::string ownedTitle{title};
    window_ = glfwCreateWindow(width, height, ownedTitle.c_str(), nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("GLFW window creation failed");
    }
}

Window::~Window() {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_) == GLFW_TRUE; }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitForVisibleFramebuffer() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &width, &height);
    }
}

} // namespace vkexp

