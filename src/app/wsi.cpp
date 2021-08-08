#include <andromeda/app/wsi.hpp>

#include <andromeda/app/log.hpp>

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace andromeda {

Window::Window(std::string_view title, uint32_t width, uint32_t height) {
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("Failed to initialize GLFW.");
	}

	// For Vulkan we need to set GLFW's API to no API.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	handle = reinterpret_cast<void*>(glfwCreateWindow(width, height, title.data(), nullptr, nullptr));

	width_px = width;
	height_px = height;

	LOG_FORMAT(LogLevel::Info, "Created app window with size {}x{} px.", width, height);
}

Window::~Window() {
	// Destroy the window and deinitialize GLFW.
	glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(handle));
	glfwTerminate();
}

uint32_t Window::width() const {
	return width_px;
}

uint32_t Window::height() const {
	return height_px;
}

void Window::poll_events() {
	glfwPollEvents();
}

bool Window::is_open() const {
	return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(handle)) == GLFW_FALSE;
}

void Window::close() {
	glfwSetWindowShouldClose(reinterpret_cast<GLFWwindow*>(handle), GLFW_TRUE);
}

void Window::hide() {
	glfwHideWindow(reinterpret_cast<GLFWwindow*>(handle));
}

std::vector<const char*> Window::window_extension_names() const {
	uint32_t count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);

	if (extensions == nullptr) return {};
	return std::vector<const char*>{extensions, extensions + count};
}

VkSurfaceKHR Window::create_surface(VkInstance instance) const {
	VkSurfaceKHR surface = nullptr;
	glfwCreateWindowSurface(instance, reinterpret_cast<GLFWwindow*>(handle), nullptr, &surface);
	return surface;
}

void* Window::platform_handle() {
	return handle;
}

} // namespace andromeda