#include <andromeda/math/transform.hpp>

#include <andromeda/components/transform.hpp>
#include <andromeda/components/hierarchy.hpp>

#include <glm/gtx/matrix_decompose.hpp>

namespace glm {

mat4 rotate(mat4 const& mat, vec3 euler) {
    mat4 result;
    result = rotate(mat, euler.z, vec3{ 0, 0, 1 });
    result = rotate(result, euler.y, vec3{ 0, 1, 0 });
    result = rotate(result, euler.x, vec3{ 1, 0, 0 });
    return result;
}

} // namespace glm

namespace andromeda::math {

glm::mat4 local_to_world(ecs::entity_t ent, thread::LockedValue<ecs::registry const> const& ecs, std::unordered_map<ecs::entity_t, glm::mat4>& lookup) {
    // Look up this entity in the lookup table first, we might have already computed this transform.
    if (auto it = lookup.find(ent); it != lookup.end()) {
        return it->second;
    }

    auto const& hierarchy = ecs->get_component<Hierarchy>(ent);
    ecs::entity_t parent = hierarchy.parent;


    // The following algorithm is based on section 2 from
    // https://docs.unity3d.com/Packages/com.unity.entities@0.0/manual/transform_system.html

    glm::mat4 parent_transform = glm::mat4(1.0);
    if (parent != ecs::no_entity) {
        parent_transform = local_to_world(parent, ecs, lookup);
    }

    // Now that we have the parent to world matrix (or an identity matrix if this entity has no parent),
    // we simply apply this entity's transformation.
    auto const& transform = ecs->get_component<Transform>(ent);
    glm::mat4 local_transform = glm::translate(glm::mat4(1.0), transform.position);
    local_transform = glm::rotate(local_transform, glm::radians(transform.rotation));
    local_transform = glm::scale(local_transform, transform.scale);

    glm::mat4 local_to_world = parent_transform * local_transform;
    // Store the transform in the lookup table.
    lookup[ent] = local_to_world;

    return local_to_world;
}

glm::vec3 euler_to_direction(glm::vec3 const& euler) {
    // We will convert the set of euler angles to a direction vector by rotating
    // the forward vector (0, 0, -1) around these angles.
    static glm::vec4 forward = glm::vec4(0, 0, -1, 0); // Note that the w component is zero to indicate a direction vector
    // Construct rotation matrix
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(euler));
    // Apply it and return the result
    glm::vec4 direction = rotation * forward;
    return glm::vec3(direction.x, direction.y, direction.z);
}

glm::vec3 matrix_to_euler(glm::mat4 const& matrix) {
    glm::vec3 _v3;
    glm::vec4 _v4;
    glm::quat _q;
    glm::vec3 rotation;
    glm::decompose(matrix, _v3, _q, _v3, rotation, _v4);
    return glm::degrees(rotation);
}

}