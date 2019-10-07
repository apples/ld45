#pragma once

#include <Meta.h>
#include <sol.hpp>

#include <tuple>

namespace scripting {

template <typename T>
struct token {};

template <typename T, typename = void>
struct has_register : std::false_type {};

template <typename T>
struct has_register<T, std::void_t<decltype(
    register_usertype(token<T>{}, std::declval<sol::simple_usertype<T>&>())
)>> : std::true_type {};

template <typename T>
void register_type(sol::table& lua) {
    std::apply([&](auto&&... members) {
        auto usertype = lua.create_simple_usertype<T>();

        if constexpr (has_register<T>::value) {
            register_usertype(token<T>{}, usertype);
        } else {
            usertype.set("new", sol::constructors<T(), T(const T&)>{});
        }

        (usertype.set(members.getName(), members.getPtr()), ...);

        lua.set_usertype(meta::getName<T>(), usertype);
    }, meta::getMembers<T>());
}

} //namespace scripting
