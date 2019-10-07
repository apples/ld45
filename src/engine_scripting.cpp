#include "engine.hpp"

void engine::register_engine_module() {
    auto global_table = sol::table(lua.globals());

    scripting::register_type<ember_database>(global_table);

    auto engine_table = lua.create_table();

    engine_table["entities"] = std::ref(entities);
    engine_table["get_texture"] = [this](const std::string& name) {
        return texture_cache.get(name);
    };
    engine_table["get_mesh"] = [this](const std::string& name) {
        return mesh_cache.get(name);
    };
    lua["package"]["loaded"]["engine"] = engine_table;
}

