#include "entities.hpp"

#include "components.hpp"
#include "component_scripting.hpp"

#include <algorithm>
#include <iostream>

void ember_database::on_destroy_entity(std::function<void(net_id id)> func) {
    destroy_entity_callback = std::move(func);
}

namespace scripting {

namespace { // static

auto get_dispatch(const sol::table& table) -> const component::dispatch_t& {
    return table["_dispatch"].get<sol::light<component::dispatch_t>>();
}

} // static

template <>
void register_type<ember_database>(sol::table& lua) {
    lua.new_usertype<ember_database::ent_id>("ent_id",
        sol::constructors<ember_database::ent_id(), ember_database::ent_id(const ember_database::ent_id&)>{},
        "get_index", &ember_database::ent_id::get_index);

    lua.new_usertype<ember_database>("ember_database",
        "create_entity", &ember_database::create_entity,
        "destroy_entity", &ember_database::destroy_entity,
        "add_component", [](ember_database& db, ember_database::ent_id eid, sol::userdata com){
            return com["_add_component"](db, eid, com);
        },
        "remove_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type){
            return com_type["_remove_component"](db, eid);
        },
        "get_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type, sol::this_state s){
            auto lua = sol::state_view{s};
            const auto& dispatch = get_dispatch(com_type);

            return dispatch.get_component(db, eid, lua);
        },
        "has_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type){
            const auto& dispatch = get_dispatch(com_type);

            return dispatch.has_component(db, eid);
        },
        "has_components", [](ember_database& db, ember_database::ent_id eid, sol::variadic_args args){
            for (const auto& arg : args) {
                const auto& dispatch = get_dispatch(arg.as<sol::table>());

                if (!dispatch.has_component(db, eid)) {
                    return false;
                }
            }

            return true;
        },
        "visit", [](ember_database& db, sol::protected_function func){
            db.visit([&func](ember_database::ent_id eid) {
                auto result = func(std::move(eid));
                if (!result.valid()) {
                    sol::error error = result;
                    throw std::runtime_error(std::string("ember_database.visit(): ") + error.what());
                }
            });
        });
}

} //namespace scripting
