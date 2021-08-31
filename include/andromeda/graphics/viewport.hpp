#pragma once

#include <string_view>
#include <string>
#include <cstdint>

#include <andromeda/ecs/entity.hpp>

namespace andromeda::gfx {

constexpr inline uint32_t MAX_VIEWPORTS = 8;

class Renderer;

/**
 * @class Viewport  
 * @brief Describes a viewport to render a scene into.
*/
class Viewport {
public:
	Viewport() = default;

	/**
	 * @brief Get the index of the viewport.
	 * @return Index of the viewport, between 0 and MAX_VIEWPORTS.
	*/
	inline uint32_t index() const {
		return id;
	}

	/**
	 * @brief Get the name of the color attachment associated with this viewport
	 * @return Name of the color attachment.
	*/
	inline std::string_view target() const {
		return target_name;
	}

	/**
	 * @brief Get the width of the viewport (in pixels).
	 * @return Width of the viewport.
	*/
	inline uint32_t width() const {
		return width_px;
	}

	/**
	 * @brief Get the height of the viewport (in pixels).
	 * @return Height of the viewport.
	*/
	inline uint32_t height() const {
		return height_px;
	}

	/**
	 * @brief Get the camera used to view this viewport.
	 * @return Entity id of the camera entity.
	*/
	inline ecs::entity_t camera() const {
		return camera_id;
	}

	/**
	 * @brief Creates a string unique to a viewport by appending the viewport ID to it.
     * @param vp Viewport to link the string to.
	 * @param str Source string to create a unique string from
	 * @return String that is guaranteed to be unique to the viewport.
	*/
	static inline std::string local_string(Viewport const& vp, std::string const& str) {
		return str + " [VP: " + std::to_string(vp.id) + "]";
	}

	/**
	 * @brief Creates a string unique to a viewport by appending the viewport ID to it.
	 * @param id Id of the viewport to link the string to.
	 * @param str Source string to create a unique string from
	 * @return String that is guaranteed to be unique to the viewport.
	*/
	static inline std::string local_string(uint32_t const id, std::string const& str) {
		return str + " [VP: " + std::to_string(id) + "]";
	}

private:
	friend class Renderer;

	/**
	 * @brief Index of the viewport. Must be smaller than MAX_VIEWPORTS.
	*/
	uint32_t id = 0;

	/**
	 * @brief Name of the attachment to render the final image to.
	*/
	std::string target_name;

	/**
	 * @brief Width (in pixels) of the viewport.
	*/
	uint32_t width_px = 0;

	/**
	 * @brief Height (in pixels) of the viewport.
	*/
	uint32_t height_px = 0;

	/**
	 * @brief Identifier of the entity that stores the camera component.
	*/
	ecs::entity_t camera_id = ecs::no_entity;
};

} // namespace andromeda::gfx