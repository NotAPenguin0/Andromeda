#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <andromeda/thread/locked_value.hpp>
#include <andromeda/ecs/registry.hpp>

#include <unordered_map>

namespace glm {
/**
 * @brief Construct a rotation matrix around multiple euler angles
 * @param mat Base matrix to apply the rotation to.
 * @param euler Euler angles to rotate around
 * @return Rotation matrix applying the rotations around euler in the correct order, after applying the given rotation.
 */
mat4 rotate(mat4 const& mat, vec3 euler);

} // namespace glm

namespace andromeda::math {

/**
 * @brief Computes the transform matrix of entity's local space to world space by applying parent transforms.
 * @param ent Entity to compute local to world matrix for.
 * @param ecs ECS registry.
 * @param lookup Lookup table with past results to avoid repeatedly computing parent transforms. Will be updated after this call.
 * @return A matrix transforming an entity's local space to world space.
*/
glm::mat4 local_to_world(ecs::entity_t ent, thread::LockedValue<ecs::registry const> const& ecs, std::unordered_map<ecs::entity_t, glm::mat4>& lookup);

/**
 * @brief Convert a set of euler angles defining a rotation to a direction vector relative to the default forward vector (0, 0, -1)
 * @param euler Euler angles with a given rotation (in degrees).
 * @return Normalized direction vector in world space.
 */
glm::vec3 euler_to_direction(glm::vec3 const& euler);

/**
 * @brief Convert an arbitrary transformation matrix to a set of euler angles (in degrees).
 *        Note that this might be fairly expensive to calculate (?) // TODO: Profile this
 * @param matrix Transformation matrix.
 * @return Euler angles representing a rotation in degrees.
 */
glm::vec3 matrix_to_euler(glm::mat4 const& matrix);

}