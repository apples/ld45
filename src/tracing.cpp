#include "tracing.hpp"

namespace { // static

tracing::context* current_context = nullptr;

} // static

namespace tracing {

namespace _detail {

source_location::source_location(int line, std::string file, std::string func):
    line(line),
    file(std::move(file)),
    function(std::move(func))
{}

std::string source_location::get_name() const {
    return function + "[" + file + ":" + std::to_string(line) + "]";
}

} // namespace _detail

stack_token::stack_token(context* ctx, std::string name) : ctx(ctx), name(std::move(name)) {}
stack_token::stack_token(stack_token&& other) : ctx(std::exchange(other.ctx, nullptr)), name(std::move(other.name)) {}

stack_token::~stack_token() {
    if (ctx) {
        ctx->pop(name);
    }
}

stack_token::stack_token(std::nullptr_t) : ctx(nullptr), name() {}

void stack_token::discard() {
    ctx = nullptr;
}

stack_token context::push(const std::string& name) {
    stack.push_back({name, clock::now(), duration(0)});
    return stack_token(this, name);
}

void context::pop(const std::string& name) {
    if (stack.empty() || stack.back().name != name) {
        throw std::runtime_error("context::pop(): Invalid pop (" + name + ")");
    }

    const auto& frame = stack.back();
    auto time = clock::now() - frame.start;
    auto& entry = all_entries[name];

    if (entry.name.empty()) {
        entry.name = name;
    }

    entry.total_time += time;
    entry.self_time += time - frame.child_time;
    ++entry.calls;

    stack.pop_back();

    if (!stack.empty()) {
        auto& parent_frame = stack.back();
        parent_frame.child_time += time;
    }
}

auto context::get_entries() const -> const std::unordered_map<std::string, entry>& {
    return all_entries;
}

void context::clear() {
    all_entries.clear();
}

void set_context(context* ctx) {
    current_context = ctx;
}

stack_token push(const std::string& name) {
    if (current_context) {
        return current_context->push(name);
    } else {
        return stack_token(nullptr);
    }
}

void pop(const std::string& name) {
    if (current_context) {
        current_context->pop(name);
    }
}

} // namespace tracing
