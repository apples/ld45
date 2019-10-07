#include "engine.hpp"

#include "emberjs/config.hpp"

using namespace std::literals;

namespace {
auto imod(double x, double y) -> int {
    return x - y * std::floor(x / y);
}
}

engine::engine() {
    config = emberjs::get_config();

    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::package);

    lua["package"]["path"] = "data/scripts/?.lua;data/scripts/?/init.lua";
    lua["package"]["cpath"] = "";

    lua.new_usertype<glm::vec2>(
        "vec2", sol::constructors<glm::vec2(), glm::vec2(const glm::vec2 &)>{},
        "x", &glm::vec2::x,
        "y", &glm::vec2::y);

    lua_gui::register_types(lua.globals());

    register_engine_module();

    soloud.init();

    {
        auto json_table = lua.create_table();
        json_table["json_to_lua"] = json_lua::json_to_lua;
        json_table["lua_to_json"] = json_lua::lua_to_json;
        json_table["load_file"] = json_lua::load_file;
        json_table["dump_json"] = [](const nlohmann::json &json) { return json.dump(); };
        lua["package"]["loaded"]["json"] = json_table;
    }

    {
        auto component_table = lua.create_named_table("component");
        component::register_all_components(component_table);
    }

    {
        auto sushi_table = lua.create_named_table("sushi");
        sushi_table.new_usertype<sushi::texture_2d>("texture_2d");
        sushi_table.new_usertype<sushi::static_mesh>("static_mesh");
    }

    lua["trace_push"] = +[](const std::string& name){ tracing::push(name).discard(); };
    lua["trace_pop"] = +[](const std::string& name){ tracing::pop(name); };

    display_width = config["display"]["width"];
    display_height = config["display"]["height"];
    aspect_ratio = float(display_width) / float(display_height);

    g_window = SDL_CreateWindow(
        "LD44",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        display_width,
        display_height,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    glcontext = SDL_GL_CreateContext(g_window);

    SDL_StartTextInput();

    glEnable(GL_DEPTH_TEST);

    program_basic = basic_shader_program("data/shaders/basic.vert", "data/shaders/basic.frag");
    program_msdf = msdf_shader_program("data/shaders/msdf.vert", "data/shaders/msdf.frag");

    program_basic.bind();
    program_basic.set_s_texture(0);
    glBindAttribLocation(program_basic.get_program().get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_basic.get_program().get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_basic.get_program().get(), sushi::attrib_location::NORMAL, "normal");

    program_msdf.bind();
    glBindAttribLocation(program_msdf.get_program().get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_msdf.get_program().get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_msdf.get_program().get(), sushi::attrib_location::NORMAL, "normal");
    glBindAttribLocation(program_msdf.get_program().get(), 3, "texSize");

    mesh_cache = [](const std::string& name) {
        return sushi::load_static_mesh_file("data/models/" + name + ".obj");
    };

    texture_cache = [](const std::string& name) {
        if (name == ":white") {
            unsigned char white[4] = { 0xff, 0xff, 0xff, 0xff };
            auto tex = sushi::create_uninitialized_texture_2d(1, 1, sushi::TexType::COLORA);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
            return tex;
        } else {
            return sushi::load_texture_2d("data/textures/" + name + ".png", false, false, false, false);
        }
    };

    font_cache = [](const std::string& fontname) {
        return msdf_font("data/fonts/"+fontname+".ttf");
    };

    sfx_cache = resource_cache<SoLoud::Wav, std::string>{[](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sfx/"+name+".wav").c_str());
        return wav;
    }};

    bgm_cache = resource_cache<SoLoud::Wav, std::string>{[volume = double(config["volume"])](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/bgm/"+name+".ogg").c_str());
        wav->setLooping(1);
        wav->setVolume(volume);
        return wav;
    }};

    auto play_sfx = [this](const std::string& name) {
        auto wav_ptr = sfx_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };

    auto play_bgm = [this](const std::string& name) {
        auto wav_ptr = bgm_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };

    lua["play_sfx"] = play_sfx;
    lua["play_bgm"] = play_bgm;

    renderer = sushi_renderer(
        {display_width, display_height},
        program_basic,
        program_msdf,
        font_cache,
        texture_cache);

    root_widget = std::make_shared<gui::widget>(renderer);
    root_widget->set_attribute("width", std::to_string(display_width));
    root_widget->set_attribute("height", std::to_string(display_height));

    lua["root_widget"] = root_widget;
    lua["focus_widget"] = [this](gui::widget* widget) {
        focused_widget = widget->weak_from_this();
    };

    luakeys = lua.create_named_table("keys");
    luakeys["left"] = false;
    luakeys["right"] = false;
    luakeys["up"] = false;
    luakeys["down"] = false;
    luakeys["jump"] = false;
    luakeys["action"] = false;

    prev_time = clock::now();

    framerate_buffer.reserve(10);

    std::cout << "initting game state" << std::endl;

    auto init_result = lua.do_file("data/scripts/init.lua");
    if (!init_result.valid()) {
        sol::error err = init_result;
        std::cerr << err.what() << std::endl;
        throw std::runtime_error("Init error: "s + err.what());
    }

    gui_state = lua["gui_state"];

    std::cout << "initting gui state" << std::endl;

    auto init_gui_result = lua.do_file("data/scripts/init_gui.lua");
    if (!init_gui_result.valid()) {
        sol::error err = init_gui_result;
        std::cerr << err.what() << std::endl;
        throw std::runtime_error("init_gui(): "s + err.what());
    }

    update_gui_state = lua["update_gui_state"];

    std::cout << "initting static resources" << std::endl;

    sprite_tex = sushi::load_texture_2d("data/textures/sprites.png", false, false, false, false);

    sprite_mesh = sushi::load_static_mesh_data(
        {{-.5f, .5f, 0.f},{-.5f, -.5f, 0.f},{.5f, -.5f, 0.f},{.5f, .5f, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f / 16.f},{1.f / 16.f, 1.f / 16.f},{1.f / 16.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});

    next_tick = std::chrono::duration_cast<clock::duration>(std::chrono::duration<std::int64_t, std::ratio<1, 30>>(1));

    enable_tracing = false;

    next_renderer_gc = clock::duration{0};
}

engine::~engine() {
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
}
