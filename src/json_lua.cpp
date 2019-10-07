#include "json_lua.hpp"

#include <fstream>

namespace json_lua {

auto json_to_lua(const nlohmann::json& json, sol::this_state state) -> sol::object {
    using value_t = nlohmann::json::value_t;
    auto lua = sol::state_view(state);

    switch (json.type()) {
    case value_t::null:
        return sol::make_object(lua, sol::nil);
    case value_t::object: {
        auto obj = lua.create_table();
        for (auto it = json.begin(); it != json.end(); ++it) {
            obj[it.key()] = json_to_lua(it.value(), state);
        }
        return obj;
    }
    case value_t::array: {
        auto obj = lua.create_table();
        for (auto i = 0; i < json.size(); ++i) {
            obj[i+1] = json_to_lua(json[i], state);
        }
        return obj;
    }
    case value_t::string:
        return sol::make_object(lua, json.get<std::string>());
    case value_t::boolean:
        return sol::make_object(lua, json.get<bool>());
    case value_t::number_integer:
        return sol::make_object(lua, json.get<int>());
    case value_t::number_unsigned:
        return sol::make_object(lua, json.get<unsigned>());
    case value_t::number_float:
        return sol::make_object(lua, json.get<double>());
    default:
        return sol::make_object(lua, sol::nil);
    }
}

auto lua_to_json(const sol::object& obj) -> nlohmann::json {
    switch (obj.get_type()) {
    case sol::type::table: {
        auto table = sol::table(obj);
        auto is_array = true;
        auto array_max = 0;

        for (auto& kvp : table) {
            if (kvp.first.get_type() != sol::type::number) {
                is_array = false;
                break;
            } else {
                array_max = std::max(array_max, kvp.first.as<int>());
            }
        }

        if (is_array) {
            auto json = nlohmann::json::array();

            for (auto i = 1; i <= array_max; ++i) {
                json.push_back(lua_to_json(table[i]));
            }

            return json;
        } else {
            auto json = nlohmann::json::object();

            for (auto& kvp : table) {
                auto val = lua_to_json(kvp.second);
                json[kvp.first.as<std::string>()] = val;
            }

            return json;
        }
    }
    case sol::type::string: return obj.as<std::string>();
    case sol::type::boolean: return obj.as<bool>();
    case sol::type::nil: return nullptr;
    case sol::type::number: {
        auto int_val = obj.as<std::int64_t>();
        auto flt_val = obj.as<double>();
        if (int_val == flt_val) {
            return int_val;
        } else {
            return flt_val;
        }
    }
    default: return nullptr;
    }
}

auto load_file(const std::string& filename, sol::this_state state) -> sol::object {
    auto file = std::ifstream(filename);
    auto json = nlohmann::json{};
    file >> json;
    return json_to_lua(json, state);
}

} // namespace json_lua
