#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <map>

template <typename T, typename... S>
class resource_cache {
public:
    using factory_function = std::function<std::shared_ptr<T>(const S&...)>;
    using cache_type = std::map<std::tuple<S...>, std::shared_ptr<T>>;

    template <typename F>
    using require_is_not_factory = std::enable_if_t<!std::is_convertible<std::decay_t<F>, factory_function>::value>*;

    resource_cache() = default;
    resource_cache(const resource_cache&) = delete;
    resource_cache(resource_cache&&) = default;
    resource_cache& operator=(const resource_cache&) = delete;
    resource_cache& operator=(resource_cache&&) = default;

    resource_cache(factory_function f) : factory(std::move(f)) {}

    template <typename F>
    resource_cache(F&& f, require_is_not_factory<F> = {}) : factory(make_factory(std::forward<F>(f))) {}

    std::shared_ptr<T> get(const S&... s) {
        auto& ptr = cache[std::tie(s...)];
        if (!ptr) {
            ptr = factory(s...);
        }
        return ptr;
    }

    void clear() {
        cache.clear();
    }

    std::shared_ptr<T> reload(const S&... s) {
        auto& ptr = cache[std::tie(s...)];
        ptr = factory(s...);
        return ptr;
    }

    const cache_type& get_cache() const {
        return cache;
    }

private:
    template <typename F>
    static factory_function make_factory(F&& f) {
        return [f=std::forward<F>(f)](const S&... s) {
            return std::make_shared<T>(f(s...));
        };
    }

    factory_function factory;
    cache_type cache;
};
