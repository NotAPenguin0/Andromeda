#ifndef ANDROMEDA_MATH_HPP_
#define ANDROMEDA_MATH_HPP_

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glm {

// Extension for glm to rotate correctly around multiple axis using euler angles
inline mat4 rotate(mat4 const& mat, vec3 euler) {
    mat4 result;
    result = rotate(mat, euler.z, vec3{ 0, 0, 1 });
    result = rotate(result, euler.y, vec3{ 0, 1, 0 });
    result = rotate(result, euler.x, vec3{ 1, 0, 0 });
    return result;
}

} // namespace glm

#endif