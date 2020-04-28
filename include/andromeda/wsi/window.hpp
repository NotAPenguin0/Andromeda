#ifndef ANDROMEDA_WSI_WINDOW_HPP_
#define ANDROMEDA_WSI_WINDOW_HPP_

#include <andromeda/util/types.hpp>

#include <phobos/forward.hpp>
#include <string_view>

namespace andromeda::wsi {

class Window {
public:
	Window(uint32_t width, uint32_t height, std::string_view title);
	~Window();

	void close();
	void poll_events();

	ph::WindowContext* handle();

	uint32_t width() const;
	uint32_t height() const;

	bool is_open() const;

private:
	ph::WindowContext* window;
};


} // namespace andromeda::wsi

#endif