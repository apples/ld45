#include "engine.hpp"

using namespace std::literals;

namespace {
auto imod(double x, double y) -> int {
    return x - y * std::floor(x / y);
}
}

auto engine::handle_game_input(const SDL_Event& event) -> bool {
    switch (event.type) {
        case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
                case SDLK_t:
                    enable_tracing = !enable_tracing;
                    return true;
                case SDLK_y:
                    tracing_context.clear();
                    return true;
            }
            break;
        }
        case SDL_QUIT:
            std::cout << "Goodbye!" << std::endl;
            return true;
    }

    return false;
}

auto engine::handle_gui_input(SDL_Event& e) -> bool {
    switch (e.type) {
        case SDL_TEXTINPUT: {
            if (auto widget = focused_widget.lock()) {
                if (widget->on_textinput) {
                    widget->on_textinput(widget, e.text.text);
                    return true;
                }
            }
            break;
        }
        case SDL_KEYDOWN: {
            if (auto widget = focused_widget.lock()) {
                if (widget->on_keydown) {
                    return widget->on_keydown(widget, SDL_GetKeyName(e.key.keysym.sym));
                }
            }
        }
        case SDL_MOUSEBUTTONDOWN: {
            switch (e.button.button) {
                case SDL_BUTTON_LEFT: {
                    auto abs_click_pos = glm::vec2{e.button.x, display_height - e.button.y + 1};
                    auto widget_stack = get_descendent_stack(*root_widget, abs_click_pos);
                    while (!widget_stack.empty()) {
                        auto cur_widget = widget_stack.back();
                        auto widget_pos = cur_widget->get_layout().position;
                        auto rel_click_pos = abs_click_pos - widget_pos;
                        if (cur_widget->on_click) {
                            cur_widget->on_click(cur_widget->shared_from_this(), rel_click_pos);
                            return true;
                        } else if (cur_widget->pointer_opaque()) {
                            return true;
                        } else {
                            widget_stack.pop_back();
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return false;
}

void engine::tick() {
    if (!enable_tracing) {
        tracing::set_context(nullptr);

        gui_state["debug_strings"] = lua.create_table();
        gui_state["debug_vals"] = lua.create_table();
    } else {
        const auto& entries = tracing_context.get_entries();

        std::vector<tracing::entry> sorted_entries;
        sorted_entries.reserve(entries.size());

        for (auto& p : entries) {
            sorted_entries.push_back(p.second);
        }

        std::sort(begin(sorted_entries), end(sorted_entries), [](auto& a, auto& b) {
            return a.self_time > b.self_time;
        });

        std::vector<std::string> debug_strings;
        std::vector<std::string> debug_vals;

        auto total = 1ll;
        auto count = 1ll;

        if (entries.count("TICK")) {
            auto& tickentry = entries.at("TICK");
            total = std::chrono::duration_cast<std::chrono::microseconds>(tickentry.total_time).count();
            count = tickentry.calls;

            auto dur = std::to_string(total / count) + "us";

            debug_strings.push_back("TICK");
            debug_vals.push_back(dur);
        }

        for (int i = 0; i < 30 && i < sorted_entries.size(); ++i) {
            const auto& entry = sorted_entries[i];

            auto us = std::chrono::duration_cast<std::chrono::microseconds>(entry.self_time).count();

            auto dur = std::to_string(us / count) + "us";

            debug_strings.push_back(entry.name);
            debug_vals.push_back(dur);
        }

        gui_state["debug_strings"] = debug_strings;
        gui_state["debug_vals"] = debug_vals;

        tracing::set_context(&tracing_context);
    }

    auto tt = tracing::push("TICK");

    auto now = clock::now();
    auto delta_time = now - prev_time;
    prev_time = now;

    const auto delta = std::chrono::duration<double>(delta_time).count();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (handle_gui_input(event)) break;
        if (handle_game_input(event)) break;
    }

    framerate_buffer.push_back(delta_time);

    if (framerate_buffer.size() >= 10) {
        auto avg_frame_dur = std::accumulate(begin(framerate_buffer), end(framerate_buffer), 0ns) / framerate_buffer.size();
        auto framerate = 1.0 / std::chrono::duration<double>(avg_frame_dur).count();

        gui_state["fps"] = std::to_string(std::lround(framerate)) + "fps";

        framerate_buffer.clear();
    }

    auto run_systems = [this](const auto&... args) {
        auto module_name = "systems";
        auto file_name = "data/scripts/systems/init.lua";
        auto import = lua.require_file(module_name, file_name);
        if (import.get_type() != sol::type::table) {
            throw std::runtime_error("systems/init.lua did not return a table");
        }
        auto scripting = sol::table(import);
        sol::protected_function visit = scripting["visit"];
        if (!visit) {
            throw std::runtime_error("systems/init.lua does not have a 'visit' function");
        }

        sol::protected_function_result visit_result;

        TRACE("SYSTEMS") {
            visit_result = visit(args...);
        }

        if (!visit_result.valid()) {
            sol::error err = visit_result;
            throw std::runtime_error(std::string("system/init.lua failure: ") + err.what());
        }
    };

    next_tick -= delta_time;

    if (next_tick <= 0s) {
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);

        auto set_key = [&](const std::string &name, int scancode) {
            auto down = bool(keys[scancode]);
            luakeys[name + "_pressed"] = down && !bool(luakeys[name]);
            luakeys[name] = down;
        };

        set_key("left", SDL_SCANCODE_LEFT);
        set_key("right", SDL_SCANCODE_RIGHT);
        set_key("up", SDL_SCANCODE_UP);
        set_key("down", SDL_SCANCODE_DOWN);
        set_key("jump", SDL_SCANCODE_Z);
        set_key("action", SDL_SCANCODE_X);

        constexpr auto rate = 60;

        run_systems(1.0 / rate);

        next_tick += std::chrono::duration_cast<clock::duration>(std::chrono::duration<std::int64_t, std::ratio<1, rate>>(1));
    }

    TRACE("RENDER SCENE") {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto scrw = 25.f / 2.f;
        const auto scrh = 18.75f / 2.f;

        auto proj = glm::ortho(-scrw, scrw, -scrh - .75f, scrh - .75f, -5.f, 5.f);

        program_basic.bind();
        program_basic.set_cam_forward({0.0, 0.0, -1.0});
        program_basic.set_tint({1, 1, 1, 1});
        program_basic.set_hue(0);
        program_basic.set_saturation(1);
        program_basic.set_normal_mat(glm::mat4(1));


        {
            auto tex = texture_cache.get("background");

            auto l = -scrw;
            auto r = scrw;
            auto b = -scrh - .75f;
            auto t = scrh - .75f;

            auto mesh = sushi::load_static_mesh_data(
                {{l, t, 0.f},{l, b, 0.f},{r, b, 0.f},{r, t, 0.f}},
                {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
                {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
                {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});

            sushi::set_texture(0, *tex);
            program_basic.set_texcoord_mat(glm::mat4(1.f));
            program_basic.set_MVP(proj * glm::translate(glm::mat4(1.f), glm::vec3{0, 0, -4}));
            sushi::draw_mesh(mesh);
        }

        sushi::set_texture(0, sprite_tex);

        entities.visit([&](const component::position& position, const component::sprite& sprite) {
            auto pos = glm::vec3(position.pos, position.z);
            pos.x = std::round(pos.x * 25.f) / 25.f;
            pos.y = std::round(pos.y * 18.75f) / 18.75f;
            auto modelmat = glm::translate(glm::mat4(1.f), pos);
            if (sprite.flip) {
                modelmat = glm::scale(modelmat, {-1.f, 1.f, 1.f});
            }
            const auto& rc = sprite.cur_frame < sprite.frames.size() ? sprite.frames[sprite.cur_frame] : component::sprite::rowcol(0, 0);
            auto offset = glm::translate(glm::mat3(1), glm::vec2{rc.c / 16.f, rc.r / 16.f});
            program_basic.set_texcoord_mat(offset);
            program_basic.set_MVP(proj * modelmat);
            sushi::draw_mesh(sprite_mesh);
        });
    }

    TRACE("GUI UPDATE") {
        auto result = update_gui_state();
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << err.what() << std::endl;
            throw std::runtime_error("Init error: "s + err.what());
        }
    }

    TRACE("GUI LAYOUT") {
        gui::calculate_all_layouts(*root_widget);
    }

    TRACE("GUI RENDER") {
        renderer.begin();
        gui::draw_all(*root_widget);
        renderer.end();
    }

    TRACE("GUI GC") {
        next_renderer_gc -= delta_time;
        if (next_renderer_gc <= clock::duration{0}) {
            renderer.collect_garbage();
            next_renderer_gc = std::chrono::seconds{1};
        }
    }

    TRACE("LUA GC") {
        lua.collect_garbage();
    }

    SDL_GL_SwapWindow(g_window);
}
