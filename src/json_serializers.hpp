#pragma once

#include "json.hpp"

#include <Meta.h>
#include <glm/glm.hpp>

#include <type_traits>

namespace nlohmann {
    template <typename T>
    struct adl_serializer<glm::tvec2<T>> {
        static void to_json(json& j, const glm::tvec2<T>& vec) {
            j[0] = vec.x;
            j[1] = vec.y;
        }

        static void from_json(const json& j, glm::tvec2<T>& vec) {
            vec.x = j[0];
            vec.y = j[1];
        }
    };
}

namespace json_serializers {

namespace basic {

template <typename T>
constexpr bool check() {
    using type = std::decay_t<T>;
    return
        std::tuple_size_v<decltype(meta::registerMembers<type>())> > 0;
}

template <typename T>
auto from_json(const nlohmann::json& json, T& msg) -> std::enable_if_t<check<T>()> {
    using type = std::decay_t<T>;
    meta::doForAllMembers<type>([&](auto& member) {
        using member_type = meta::get_member_type<decltype(member)>;
        member.set(msg, json[member.getName()].template get<member_type>());
    });
}

template <typename T>
auto to_json(nlohmann::json& json, const T& msg) -> std::enable_if_t<check<T>()> {
    using type = std::decay_t<T>;
    meta::doForAllMembers<type>([&](auto& member) {
        json[member.getName()] = member.template get(msg);
    });
}

} // namespace basic

namespace with_type {

template <typename T>
constexpr bool check() {
    using type = std::decay_t<T>;
    return
        std::tuple_size_v<decltype(meta::registerMembers<type>())> > 0 &&
        meta::registerName<type>()[0] != '\0';
}

template <typename T>
auto from_json(const nlohmann::json& json, T& msg) -> std::enable_if_t<check<T>()> {
    using type = std::decay_t<T>;
    meta::doForAllMembers<type>([&](auto& member) {
        using member_type = meta::get_member_type<decltype(member)>;
        member.set(msg, json[member.getName()].template get<member_type>());
    });
}

template <typename T>
auto to_json(nlohmann::json& json, const T& msg) -> std::enable_if_t<check<T>()> {
    using type = std::decay_t<T>;
    json["type"] = meta::getName<type>();
    meta::doForAllMembers<type>([&](auto& member) {
        json[member.getName()] = member.template get(msg);
    });
}

} // namespace with_type

} // namespace json_serializers
