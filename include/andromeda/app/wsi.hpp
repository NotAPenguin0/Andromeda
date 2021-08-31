#pragma once

#include <string_view>
#include <cstdint>

#include <phobos/window_interface.hpp>

struct GLFWwindow;

namespace andromeda {

namespace impl {
void resize_callback(GLFWwindow*, int, int);
}

/**
 * @class Window 
 * @brief Manages the main application window.
*/
class Window : public ph::WindowInterface {
public:
	/**
	 * @brief Initialize the window.
	 * @param title Title to display in the top bar of the window.
	 * @param width Window width in screen pixels.
	 * @param height Window height in screen pixels.
	*/
	Window(std::string_view title, uint32_t width, uint32_t height);

	Window(Window const&) = delete;
	Window(Window&&) = delete;

	Window& operator=(Window const&) = delete;
	Window& operator=(Window&&) = delete;

	/**
	 * @brief Destroys the window.
	*/
	~Window();

	/**
	 * @brief Get the width of the window.
	 * @return The width of the window, in screen pixels.
	*/

	uint32_t width() const override;

	/**
	 * @brief Get the height of the window.
	 * @return The height of the window, in screen pixels.
	*/
	uint32_t height() const override;

	/**
	 * @brief Maximizes the window.
	*/
	void maximize();

	/**
	 * @brief Updates the window, polls and processes input events;
	*/
	void poll_events();

	/**
	 * @brief Check if the window is still open.
	 * @return true if the window is currently opened, false otherwise
	*/
	bool is_open() const;

	/**
	 * @brief Signals that the window should be closed.
	*/
	void close();

	/**
	 * @brief Hides the window.
	*/
	void hide();

	/**
	 * @brief Grab the Vulkan extensions needed to create this window's surface.
	 * @return A vector with extension names required to create a Vulkan surface.
	*/
	std::vector<const char*> window_extension_names() const override;

	/**
	 * @brief Creates a Vulkan surface from this window.
	 * @param instance The Vulkan instance to create this surface with.
	 * @return The created Vulkan surface.
	*/
	VkSurfaceKHR create_surface(VkInstance instance) const override;

	/**
	 * @brief Returns a handle to the managed window.
	 * @return Type-erased handle to managed GLFWwindow.
	*/
	void* platform_handle();

private:
	// Handle to the actual window instance.
	void* handle = nullptr;
	uint32_t width_px = 0, height_px = 0;

	friend void impl::resize_callback(GLFWwindow*, int, int);
};

} // namespace andromeda