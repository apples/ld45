#include "gui.hpp"

#include "utility.hpp"

#include <iostream>
#include <exception>

namespace { // static

template <typename PreFunc, typename PostFunc>
void visit_in_order(gui::widget& root, const PreFunc& prefunc, const PostFunc& postfunc) {
    prefunc(root);

    if (const auto& children = root.get_children(); !children.empty()) {
        return visit_in_order(*children.front(), prefunc, postfunc);
    } else {
        for (auto ancestor = &root; ancestor; ancestor = ancestor->get_parent()) {
            postfunc(*ancestor);
            if (auto ns = ancestor->get_next_sibling()) {
                return visit_in_order(*ns, prefunc, postfunc);
            }
        }
    }
}

template <typename F>
void visit_in_order(gui::widget& root, const F& func) {
    return visit_in_order(root, func, [](auto&){});
}

} // static

namespace gui {

widget::widget(render_context& renderer) : renderer(&renderer) {}

widget* widget::get_parent() const { return parent; }

widget* widget:: get_next_sibling() const { return next_sibling; }

void widget::draw_self() const {}

render_context* widget::get_renderer() const { return renderer; }

std::string widget::get_type() const { return "widget"; }

bool widget::pointer_opaque() const {
    return on_click || has_attribute("pointer_opaque");
}

std::shared_ptr<widget> widget::create_widget(const std::string& type) const {
    if (type == "widget") {
        return std::make_shared<widget>(*renderer);
    } else if (type == "label") {
        return std::make_shared<label>(*renderer);
    } else if (type == "panel") {
        return std::make_shared<panel>(*renderer);
    } else {
        throw std::logic_error("Invalid widget type: " + type);
    }
}

const std::vector<std::shared_ptr<widget>>& widget::get_children() const { return children; }

void widget::add_child(std::shared_ptr<widget> child) {
    if (!child) {
        throw std::logic_error("add_child(): cannot add null child");
    }

    if (auto old_parent = child->parent) {
        old_parent->remove_child(child.get());
    }

    if (children.size() > 0) {
        children.back()->next_sibling = child.get();
    }

    child->parent = this;
    children.push_back(std::move(child));
}

void widget::remove_child(widget* child) {
    if (!child) {
        throw std::logic_error("remove_child(): cannot remove null child");
    }

    if (child->parent != this) {
        throw std::logic_error("remove_child(): child does not belong to this");
    }

    auto iter = std::find_if(children.begin(), children.end(), [&](auto& c){ return c.get() == child; });

    if (iter != children.end()) {
        if (iter > children.begin()) {
            (*std::prev(iter))->next_sibling = (*iter)->next_sibling;
        }

        (*iter)->parent = nullptr;
        (*iter)->next_sibling = nullptr;

        children.erase(iter);
    }
}

void widget::replace_child(widget* child, std::shared_ptr<widget> new_child) {
    if (!child) {
        throw std::logic_error("remove_child(): cannot replace null child");
    }

    if (child->parent != this) {
        throw std::logic_error("remove_child(): child does not belong to this");
    }

    if (!new_child) {
        throw std::logic_error("replace_child(): cannot replace with null child");
    }

    if (auto old_parent = new_child->parent) {
        old_parent->remove_child(new_child.get());
    }

    auto iter = std::find_if(children.begin(), children.end(), [&](auto& c){ return c.get() == child; });

    if (iter != children.end()) {
        if (iter > children.begin()) {
            (*std::prev(iter))->next_sibling = new_child.get();
        }

        new_child->parent = this;
        new_child->next_sibling = (*iter)->next_sibling;

        (*iter)->parent = nullptr;
        (*iter)->next_sibling = nullptr;

        *iter = std::move(new_child);
    }
}

void widget::clear_children() {
    for (auto& child : children) {
        child->parent = nullptr;
        child->next_sibling = nullptr;
    }

    children.clear();
}

void widget::set_attribute(const std::string& name, sol::optional<std::string> value) {
    if (value) {
        attributes.insert_or_assign(name, *value);
    } else {
        attributes.erase(name);
    }
}

sol::optional<std::string> widget::get_attribute(const std::string& name) const {
    auto iter = attributes.find(name);
    if (iter != attributes.end()) {
        return iter->second;
    } else {
        return sol::nullopt;
    }
}

bool widget::has_attribute(const std::string& name) const {
    return attributes.find(name) != attributes.end();
}

const std::unordered_map<std::string, std::string>& widget::get_all_attributes() const {
    return attributes;
}

void widget::calculate_layout() {
    auto layout = get_layout();

    layout.size = {0, 0};
    layout.position = {0, 0};
    layout.visible = true;

    if (auto attr_width = get_attribute("width")) {
        std::size_t len;

        const auto& str_width = *attr_width;
        auto value = std::stof(str_width, &len);
        auto unit = str_width.substr(len);

        if (unit == "" || unit == "px") {
            layout.size.x = value;
        } else if (unit == "%" || unit == "%w") {
            if (auto parent = get_parent()) {
                const auto& parent_layout = parent->get_layout();

                layout.size.x = (value / 100.f) * parent_layout.size.x;
            }
        } else if (unit == "%h") {
            if (auto parent = get_parent()) {
                const auto& parent_layout = parent->get_layout();

                layout.size.x = (value / 100.f) * parent_layout.size.y;
            }
        }
    }

    if (auto attr_height = get_attribute("height")) {
        std::size_t len;

        const auto& str_height = *attr_height;
        auto value = std::stof(str_height, &len);
        auto unit = str_height.substr(len);

        if (unit == "" || unit == "px") {
            layout.size.y = value;
        } else if (unit == "%" || unit == "%h") {
            if (auto parent = get_parent()) {
                const auto& parent_layout = parent->get_layout();

                layout.size.y = (value / 100.f) * parent_layout.size.y;
            }
        } else if (unit == "%w") {
            if (auto parent = get_parent()) {
                const auto& parent_layout = parent->get_layout();

                layout.size.y = (value / 100.f) * parent_layout.size.x;
            }
        }
    }

    if (auto attr_left = get_attribute("left")) {
        layout.position.x = std::stoi(*attr_left);
    }

    if (auto attr_right = get_attribute("right")) {
        layout.position.x = std::stoi(*attr_right) - layout.size.x;
    }

    if (auto attr_bottom = get_attribute("bottom")) {
        layout.position.y = std::stoi(*attr_bottom);
    }

    if (auto attr_top = get_attribute("top")) {
        layout.position.y = std::stoi(*attr_top) - layout.size.y;
    }

    if (auto attr_visible = get_attribute("visible")) {
        if (*attr_visible == "false") {
            layout.visible = false;
        }
    }

    if (auto parent = get_parent()) {
        const auto& parent_layout = parent->get_layout();

        auto align = [&](float glm::vec2::* d, std::string name, std::string normal, std::string opposite) {
            auto attr_align = get_attribute(name).value_or(normal);

            if (attr_align == normal) {
                layout.position.*d += parent_layout.position.*d;
            } else if (attr_align == opposite) {
                layout.position.*d =
                    parent_layout.position.*d
                    + parent_layout.size.*d
                    - layout.position.*d
                    - layout.size.*d * 2;
            } else if (attr_align == "center") {
                layout.position.*d =
                    parent_layout.position.*d
                    + parent_layout.size.*d / 2
                    - layout.size.*d / 2;
            }
        };

        align(&glm::vec2::x, "halign", "left", "right");
        align(&glm::vec2::y, "valign", "bottom", "top");
    }

    set_layout(layout);
}

const block_layout& widget::get_layout() const { return layout; }

void widget::set_layout(const block_layout& lo) { layout = lo; }

std::vector<widget*> get_descendent_stack(const widget& root, const glm::vec2& position) {
    auto result = std::vector<widget*>();

    for (const auto& child : root.get_children()) {
        const auto& child_layout = child->get_layout();

        if (child_layout.visible) {
            auto left = child_layout.position.x;
            auto bottom = child_layout.position.y;
            auto right = left + child_layout.size.x;
            auto top = bottom + child_layout.size.y;

            if (position.x >= left && position.x < right && position.y >= bottom && position.y < top) {
                result.push_back(child.get());

                auto child_stack = get_descendent_stack(*child, position);

                result.insert(end(result), begin(child_stack), end(child_stack));
            }
        }
    }

    return result;
}

void calculate_all_layouts(widget& root) try {

    visit_in_order(root, [](widget& w) { w.calculate_layout(); });

} catch (const std::exception& e) {
    std::cerr << "calculate_all_layouts() exception: "
              << " (type: " << typeid(root).name()
              << ", attrs: {";
    for (auto& attr : root.get_all_attributes()) {
        std::cerr << attr.first << "={" << attr.second << "} ";
    }
    std::cerr << "})" << std::endl;
    throw;
}

void draw_all(widget& root) {
    visit_in_order(root, [](widget& w){ w.draw_self(); });
}

// label

label::label(render_context& renderer) : widget(renderer) {}

std::string label::get_type() const { return "label"; }

void label::draw_self() const {
    const auto& layout = get_layout();
    get_renderer()->draw_text(text, font, color, layout.position, layout.size.y);
}

void label::calculate_layout() {
    widget::calculate_layout();

    text = get_attribute("text").value_or("");

    auto attr_font = get_attribute("font");

    if (!attr_font) {
        auto parent = get_parent();
        while (!attr_font && parent) {
            attr_font = parent->get_attribute("font");
            parent = parent->get_parent();
        }
    }

    font = attr_font.value_or("LiberationSans-Regular");

    auto attr_color = get_attribute("color").value_or("#000");

    color = {0, 0, 0, 1};

    if (attr_color[0] == '#') {
        switch (attr_color.size()) {
        case 4:
            color[0] = float(std::stoi(attr_color.substr(1, 1), nullptr, 16)) / 15.f;
            color[1] = float(std::stoi(attr_color.substr(2, 1), nullptr, 16)) / 15.f;
            color[2] = float(std::stoi(attr_color.substr(3, 1), nullptr, 16)) / 15.f;
            color[3] = 1.f;
            break;
        case 5:
            color[0] = float(std::stoi(attr_color.substr(1, 1), nullptr, 16)) / 15.f;
            color[1] = float(std::stoi(attr_color.substr(2, 1), nullptr, 16)) / 15.f;
            color[2] = float(std::stoi(attr_color.substr(3, 1), nullptr, 16)) / 15.f;
            color[3] = float(std::stoi(attr_color.substr(4, 1), nullptr, 16)) / 15.f;
            break;
        case 7:
            color[0] = float(std::stoi(attr_color.substr(1, 2), nullptr, 16)) / 255.f;
            color[1] = float(std::stoi(attr_color.substr(3, 2), nullptr, 16)) / 255.f;
            color[2] = float(std::stoi(attr_color.substr(5, 2), nullptr, 16)) / 255.f;
            color[3] = 1.f;
            break;
        case 9:
            color[0] = float(std::stoi(attr_color.substr(1, 2), nullptr, 16)) / 255.f;
            color[1] = float(std::stoi(attr_color.substr(3, 2), nullptr, 16)) / 255.f;
            color[2] = float(std::stoi(attr_color.substr(5, 2), nullptr, 16)) / 255.f;
            color[3] = float(std::stoi(attr_color.substr(7, 2), nullptr, 16)) / 255.f;
            break;
        }
    } else {
        if (attr_color == "black") {
            color = {0, 0, 0, 1};
        } else if (attr_color == "white") {
            color = {1, 1, 1, 1};
        }
    }

    auto layout = get_layout();

    layout.position.x = 0;
    layout.size.x = layout.size.y * get_renderer()->get_text_width(text, font);

    if (auto attr_left = get_attribute("left")) {
        layout.position.x = std::stoi(*attr_left);
    }

    if (auto attr_right = get_attribute("right")) {
        layout.position.x = std::stoi(*attr_right) - layout.size.x;
    }

    if (auto parent = get_parent()) {
        const auto& parent_layout = get_parent()->get_layout();

        auto attr_halign = get_attribute("halign").value_or("left");

        if (attr_halign == "left") {
            layout.position.x += parent_layout.position.x;
        } else if (attr_halign == "right") {
            layout.position.x =
                parent_layout.position.x
                + parent_layout.size.x
                - layout.position.x
                - layout.size.x;
        } else if (attr_halign == "center") {
            layout.position.x =
                parent_layout.position.x
                + parent_layout.size.x / 2
                - layout.size.x / 2;
        }
    }

    set_layout(layout);
}

// Panel

panel::panel(render_context& renderer) : widget(renderer) {}

std::string panel::get_type() const { return "panel"; }

bool panel::pointer_opaque() const {
    return true;
}

void panel::draw_self() const {
    const auto& layout = get_layout();
    get_renderer()->draw_rectangle(texture, color, layout.position, layout.size);
}

void panel::calculate_layout() {
    widget::calculate_layout();

    texture = get_attribute("texture").value_or(":white");

    auto attr_color = get_attribute("color").value_or("white");

    if (attr_color[0] == '#') {
        switch (attr_color.size()) {
        case 4:
            color[0] = float(std::stoi(attr_color.substr(1, 1), nullptr, 16)) / 15.f;
            color[1] = float(std::stoi(attr_color.substr(2, 1), nullptr, 16)) / 15.f;
            color[2] = float(std::stoi(attr_color.substr(3, 1), nullptr, 16)) / 15.f;
            color[3] = 1.f;
            break;
        case 5:
            color[0] = float(std::stoi(attr_color.substr(1, 1), nullptr, 16)) / 15.f;
            color[1] = float(std::stoi(attr_color.substr(2, 1), nullptr, 16)) / 15.f;
            color[2] = float(std::stoi(attr_color.substr(3, 1), nullptr, 16)) / 15.f;
            color[3] = float(std::stoi(attr_color.substr(4, 1), nullptr, 16)) / 15.f;
            break;
        case 7:
            color[0] = float(std::stoi(attr_color.substr(1, 2), nullptr, 16)) / 255.f;
            color[1] = float(std::stoi(attr_color.substr(3, 2), nullptr, 16)) / 255.f;
            color[2] = float(std::stoi(attr_color.substr(5, 2), nullptr, 16)) / 255.f;
            color[3] = 1.f;
            break;
        case 9:
            color[0] = float(std::stoi(attr_color.substr(1, 2), nullptr, 16)) / 255.f;
            color[1] = float(std::stoi(attr_color.substr(3, 2), nullptr, 16)) / 255.f;
            color[2] = float(std::stoi(attr_color.substr(5, 2), nullptr, 16)) / 255.f;
            color[3] = float(std::stoi(attr_color.substr(7, 2), nullptr, 16)) / 255.f;
            break;
        }
    } else {
        if (attr_color == "white") {
            color = {1, 1, 1, 1};
        } else if (attr_color == "black") {
            color = {0, 0, 0, 1};
        }
    }
}

} //namespace gui
