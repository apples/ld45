#pragma once

#include "sdl.hpp"

#include "entities.hpp"
#include "components.hpp"
#include "font.hpp"
#include "gui.hpp"
#include "lua_gui.hpp"
#include "sushi_renderer.hpp"
#include "shaders.hpp"
#include "utility.hpp"
#include "resource_cache.hpp"
#include "json_lua.hpp"
#include "tracing.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include <sol.hpp>

#include <sol.hpp>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>
#include <memory>
#include <random>

class engine {
public:
    engine();

    ~engine();

    void register_engine_module();

    auto handle_game_input(const SDL_Event& event) -> bool;

    auto handle_gui_input(SDL_Event& e) -> bool;

    void tick();

private:
    using clock = std::chrono::steady_clock;

    sol::state lua;
    nlohmann::json config;
    int display_width;
    int display_height;
    float aspect_ratio;
    SDL_Window* g_window;
    SDL_GLContext glcontext;
    basic_shader_program program_basic;
    msdf_shader_program program_msdf;
    resource_cache<sushi::static_mesh, std::string> mesh_cache;
    resource_cache<sushi::texture_2d, std::string> texture_cache;
    resource_cache<msdf_font, std::string> font_cache;
    resource_cache<SoLoud::Wav, std::string> sfx_cache;
    resource_cache<SoLoud::Wav, std::string> bgm_cache;
    ember_database entities;
    sushi_renderer renderer;
    std::shared_ptr<gui::widget> root_widget;
    std::weak_ptr<gui::widget> focused_widget;
    clock::time_point prev_time;
    std::vector<std::chrono::nanoseconds> framerate_buffer;
    sol::table gui_state;
    sol::protected_function update_gui_state;
    sol::table luakeys;
    sushi::texture_2d sprite_tex;
    sushi::static_mesh sprite_mesh;
    clock::duration next_tick;
    SoLoud::Soloud soloud;
    tracing::context tracing_context;
    bool enable_tracing;
    clock::duration next_renderer_gc;
};