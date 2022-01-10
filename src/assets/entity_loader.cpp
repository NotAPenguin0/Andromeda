#include <andromeda/assets/loaders.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/graphics/context.hpp>

#include <reflect/reflection.hpp>

#include <plib/stream.hpp>
#include <json/json.hpp>

namespace andromeda::impl {

using namespace ::andromeda; // So we don't need to prefix our component names constantly.

namespace {

template<typename T>
struct json_convert {
    static T from_json(json::JSON const& json) {
        assert(false && "from_json not implemented");
        return {};
    }

    static json::JSON to_json(T const& value) {
        assert(false && "to_json not implemented");
        return {};
    }
};

template<>
struct json_convert<std::string> {
    [[nodiscard]] static std::string from_json(json::JSON const& json) {
        return json.ToString();
    }

    [[nodiscard]] static json::JSON to_json(std::string const& value) {
        return {value};
    }
};

template<>
struct json_convert<bool> {
    [[nodiscard]] static bool from_json(json::JSON const& json) {
        return json.ToBool();
    }

    [[nodiscard]] static json::JSON from_json(bool const& value) {
        return {value};
    }
};

template<typename A>
struct json_convert<Handle<A>> {
    [[nodiscard]] static Handle<A> from_json(json::JSON const& json) {
        return assets::load<A>(json.ToString());
    }

    [[nodiscard]] static json::JSON to_json(Handle<A> const& value) {
        auto path = assets::get_path(value);
        if (path) {
            return {path->generic_string()};
        } else {
            // Invalid asset, we leave this empty
            return {};
        }
    }
};

template<>
struct json_convert<float> {
    [[nodiscard]] static float from_json(json::JSON const& json) {
        return static_cast<float>(json.ToFloat());
    }

    [[nodiscard]] static json::JSON to_json(float value) {
        return {value};
    }
};

template<>
struct json_convert<glm::vec3> {
    [[nodiscard]] static glm::vec3 from_json(json::JSON const& json) {
        assert(json.length() == 3 && "Invalid array size for 3-component vector in json data");
        auto result = glm::vec3(0.0f);
        result.x = static_cast<float>(json.at(0).ToFloat());
        result.y = static_cast<float>(json.at(1).ToFloat());
        result.z = static_cast<float>(json.at(2).ToFloat());
        return result;
    }

    [[nodiscard]] static json::JSON to_json(glm::vec3 const& value) {
        json::JSON json;
        json[0] = value.x;
        json[1] = value.y;
        json[2] = value.z;
        return json;
    }
};

struct load_field_json {
    template<typename F>
    void operator()(F& value, json::JSON const& json) {
        value = json_convert<F>::from_json(json);
    }
};

template<typename C>
struct load_component_json {
    // Note that this JSON is the json data of the entire entity.
    void operator()(ecs::entity_t entity, World& world, thread::LockedValue<ecs::registry>& blueprints, json::JSON const& json) const {
        meta::reflection_info<C> const& refl = meta::reflect<C>();
        // Check if JSON data has matching key for this component. If not, we can return early and skip importing fields.
        if (!json.hasKey(refl.name())) { return; }
        // Only add if not yet present
        if (!blueprints->has_component<C>(entity)) {
            blueprints->add_component<C>(entity);
        }
        C& component = blueprints->template get_component<C>(entity);
        json::JSON const& component_json = json.at(refl.name());

        // Similarly to the old looping over each component, we'll now try looping over each field and finding it in the JSON object
        for (meta::field<C> field: refl.fields()) {
            // If key was found in the component's json, dispatch a call to load_field_json to load its data.
            if (component_json.hasKey(field.name())) {
                meta::dispatch(field, component, load_field_json{}, component_json.at(field.name()));
            }
        }
    }
};
}

static void load_entity_json(ecs::entity_t entity, World& world, thread::LockedValue<ecs::registry>& blueprints, json::JSON const& json) {
    // Instead of looping over each entry in the JSON, we'll loop over each component type and check whether it's present in the JSON data.
    // This way we avoid ever having to manually map strings to component types.
    meta::for_each_component<load_component_json>(entity, world, blueprints, json);
}

static ecs::entity_t load_entity_and_children(World& world, thread::LockedValue<ecs::registry>& blueprints, ecs::entity_t parent, json::JSON const& json) {
    ecs::entity_t entity = world.create_blueprint(blueprints, parent);
    // Read JSON information of this entity
    load_entity_json(entity, world, blueprints, json);
    // Load child entities
    if (json.hasKey("children")) {
        auto children = json.at("children").ArrayRange();
        for (auto const& child_json: children) {
            ecs::entity_t child = load_entity_and_children(world, blueprints, entity, child_json);
        }
    }
    return entity;
}

void load_entity(gfx::Context& ctx, World& world, Handle<ecs::entity_t> handle, std::string_view path) {
    // Read JSON from file stream.
    plib::binary_input_stream file = plib::binary_input_stream::from_file(path.data());

    std::string json_string{};
    json_string.resize(file.size());
    file.read<char>(json_string.data(), file.size());
    json::JSON json = json::JSON::Load(json_string);

    auto bp_lock = world.blueprints();
    ecs::entity_t entity = load_entity_and_children(world, bp_lock, 0, json);

    // Insert into asset system
    assets::impl::make_ready(handle, entity);
}

}