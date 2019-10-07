#include "components.hpp"

#include "component_scripting.hpp"

namespace component {

void register_all_components(sol::table& table) {
    using scripting::register_type;

    {
        using rowcol = sprite::rowcol;
        auto usertype = table.create_simple_usertype<rowcol>();
        usertype.set("new", sol::constructors<rowcol(), rowcol(const rowcol&), rowcol(int, int)>{});
        usertype.set("r", &rowcol::r);
        usertype.set("c", &rowcol::c);
        table.set_usertype("rowcol", usertype);
    }

    register_type<net_id>(table);
    register_type<position>(table);
    register_type<velocity>(table);
    register_type<script>(table);
    register_type<sprite>(table);
    register_type<controller>(table);
    register_type<body>(table);
    register_type<food>(table);
}

} //namespace component
