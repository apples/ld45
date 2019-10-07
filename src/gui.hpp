#pragma once

#include <glm/glm.hpp>
#include <sol.hpp>

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <unordered_map>

namespace gui {

class render_context {
public:
    virtual ~render_context() = 0;

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void draw_rectangle(const std::string& texture, const glm::vec4& color, glm::vec2 position, glm::vec2 size) = 0;
    virtual void draw_text(const std::string& text, const std::string& font, const glm::vec4& color, glm::vec2 position, float size) = 0;
    virtual float get_text_width(const std::string& text, const std::string& font) = 0;
};

inline render_context::~render_context() = default;

struct block_layout {
    glm::vec2 position = {0, 0};
    glm::vec2 size = {0, 0};
    bool visible = true;
};

class widget : public std::enable_shared_from_this<widget> {
public:
    widget() = default;
    virtual ~widget() = default;

    widget(const widget&) = delete;
    widget(widget&&) = delete;

    widget& operator=(const widget&) = delete;
    widget& operator=(widget&&) = delete;

    widget(render_context& renderer);

    widget* get_parent() const;

    widget* get_next_sibling() const;

    virtual void draw_self() const;

    render_context* get_renderer() const;

    virtual std::string get_type() const;

    virtual bool pointer_opaque() const;

    // Factories

    std::shared_ptr<widget> create_widget(const std::string& type) const;

    // Tree

    const std::vector<std::shared_ptr<widget>>& get_children() const;

    void add_child(std::shared_ptr<widget> child);

    void remove_child(widget* child);

    void replace_child(widget* child, std::shared_ptr<widget> new_child);

    void clear_children();

    // Attrs

    void set_attribute(const std::string& name, sol::optional<std::string> value);

    sol::optional<std::string> get_attribute(const std::string& name) const;

    bool has_attribute(const std::string& name) const;

    const std::unordered_map<std::string, std::string>& get_all_attributes() const;

    // Events

    std::function<void(std::shared_ptr<widget> self, glm::vec2 click_pos)> on_click;

    std::function<void(std::shared_ptr<widget> self, const std::string& text)> on_textinput;

    std::function<bool(std::shared_ptr<widget> self, const std::string& keyname)> on_keydown;

    // Layout

    virtual void calculate_layout();

    const block_layout& get_layout() const;

protected:

    void set_layout(const block_layout& lo);

private:
    std::vector<std::shared_ptr<widget>> children = {};
    std::unordered_map<std::string, std::string> attributes = {};
    widget* parent = nullptr;
    widget* next_sibling = nullptr;
    render_context* renderer = nullptr;
    block_layout layout;
};

std::vector<widget*> get_descendent_stack(const widget& widget, const glm::vec2& position);

void calculate_all_layouts(widget& root);

void draw_all(widget& root);

class label final : public widget {
public:
    label() = default;

    label(render_context& renderer);

    virtual std::string get_type() const override;

    virtual void draw_self() const override;

    virtual void calculate_layout() override;

private:
    std::string text;
    std::string font;
    glm::vec4 color;
};

class panel final : public widget {
public:
    panel() = default;

    panel(render_context& renderer);

    virtual std::string get_type() const override;

    virtual bool pointer_opaque() const override;

    virtual void draw_self() const override;

    virtual void calculate_layout() override;

private:
    std::string texture;
    glm::vec4 color;
};

} //namespace gui
