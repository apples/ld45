#include "lua_gui.hpp"

#include "gui.hpp"

namespace lua_gui {

void register_types(sol::table lua) {
    using namespace gui;

    lua.new_usertype<block_layout>("block_layout",
        sol::constructors<>{},
        "position", &block_layout::position,
        "size", &block_layout::size,
        "visible", &block_layout::visible);

    lua.new_usertype<widget>("widget",
        sol::constructors<>{},
        "get_type", &widget::get_type,
        "create_widget", &widget::create_widget,
        "get_parent", &widget::get_parent,
        "get_children", &widget::get_children,
        "add_child", &widget::add_child,
        "remove_child", &widget::remove_child,
        "replace_child", &widget::replace_child,
        "clear_children", &widget::clear_children,
        "set_attribute", &widget::set_attribute,
        "get_attribute", &widget::get_attribute,
        "get_all_attributes", &widget::get_all_attributes,
        "on_click", &widget::on_click,
        "on_textinput", &widget::on_textinput,
        "on_keydown", &widget::on_keydown,
        "get_layout", &widget::get_layout);
}

} // namespace lua_gui
