#pragma once

#include "json.hpp"
#include "scripting.hpp"
#include "entities.hpp"

#include <type_traits>
#include <utility>
#include <iostream>
#include <stdexcept>

namespace component {
namespace _detail {

using ent_id = ember_database::ent_id;
using com_id = ember_database::com_id;

template <typename T>
struct is_tag : std::false_type {};

template <typename T>
struct is_tag<ginseng::tag<T>> : std::true_type {};

template <typename T>
void add_component(ember_database& db, ent_id eid, T com) {
    db.add_component(eid, std::move(com));
}

template <typename T>
void remove_component(ember_database& db, ent_id eid) {
    if (!db.has_component<T>(eid)) {
        auto name = std::string(meta::getName<T>());
        auto id = std::to_string(eid.get_index());

        throw std::runtime_error("Bad component removal: " + name + " from entity " + id);
    }

    db.remove_component<T>(eid);
}

template <typename T>
auto get_component(ember_database& db, ent_id eid) -> T& {
    if (!db.has_component<T>(eid)) {
        auto name = std::string(meta::getName<T>());
        auto id = std::to_string(eid.get_index());

        throw std::runtime_error("Bad component access: " + name + " from entity " + id);
    }

    return db.get_component<T>(eid);
}

template <typename T>
auto get_component_obj(ember_database& db, ent_id eid, sol::state_view lua) -> sol::object {
    return sol::make_object(lua, std::ref(get_component<T>(db, eid)));
}

template <typename T>
auto has_component(ember_database& db, ember_database::ent_id eid) -> bool {
    return db.has_component<T>(eid);
}

template <typename T>
void visit(ember_database& db, const std::function<void(ember_database::ent_id, T&)>& visitor) {
    db.visit(visitor);
}

template <typename T>
void visit_tag(ember_database& db, const std::function<void(ember_database::ent_id, T)>& visitor) {
    db.visit(visitor);
}

} //namespace _detail

struct dispatch_t {
    auto (*has_component)(ember_database& db, ember_database::ent_id eid) -> bool;
    auto (*get_component)(ember_database& db, ember_database::ent_id eid, sol::state_view lua) -> sol::object;
};

template <typename T>
void register_usertype(scripting::token<T>, sol::simple_usertype<T>& usertype) {
    using namespace _detail;

    if constexpr (is_tag<T>::value) {
        usertype.set("new", sol::constructors<T()>{});
        usertype.set("_visit", &_detail::visit_tag<T>);
    } else {
        usertype.set("new", sol::constructors<T(), T(const T&)>{});
        usertype.set("_get_component", &_detail::get_component<T>);
        usertype.set("_visit", &_detail::visit<T>);
    }

    usertype.set("_add_component", &_detail::add_component<T>);
    usertype.set("_remove_component", &_detail::remove_component<T>);
    usertype.set("_has_component", &_detail::has_component<T>);

    static auto dispatch = dispatch_t{
        &_detail::has_component<T>,
        &_detail::get_component_obj<T>,
    };

    usertype.set("_dispatch", sol::make_light(dispatch));
}

} //namespace component
