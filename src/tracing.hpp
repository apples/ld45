#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#define TRACE(name) if (auto tt = tracing::push(name); true)

namespace tracing {

namespace _detail {

struct source_location {
    int line;
    std::string file;
    std::string function;

    source_location(
        int line = __builtin_LINE(),
        std::string file = __builtin_FILE(),
        std::string func = __builtin_FUNCTION());
    
    std::string get_name() const;
};

} // namespace _detail

using clock = std::chrono::steady_clock;
using duration = clock::duration;
using time_point = clock::time_point;

struct entry {
    std::string name;
    duration self_time = duration(0);
    duration total_time = duration(0);
    int calls = 0;
};

struct stack_frame {
    std::string name;
    time_point start;
    duration child_time;
};

class context;

class stack_token {
    friend class context;
public:
    stack_token(stack_token&& other);
    stack_token(const stack_token& other) = delete;
    stack_token& operator=(stack_token&& other) = delete;
    stack_token& operator=(const stack_token& other) = delete;
    ~stack_token();

    explicit stack_token(std::nullptr_t);

    void discard();

private:
    stack_token(context* ctx, std::string name);

    context* ctx;
    std::string name;
};

class context {
public:
    stack_token push(const std::string& name);
    void pop(const std::string& name);

    const std::unordered_map<std::string, entry>& get_entries() const;

    void clear();

private:
    std::unordered_map<std::string, entry> all_entries;
    std::vector<stack_frame> stack;
};

void set_context(context* ctx);

stack_token push(const std::string& name = _detail::source_location().get_name());

void pop(const std::string& name);

} // namespace tracing
