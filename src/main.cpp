
#include "engine.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <boost/asio.hpp>
#endif

extern "C" void EMSCRIPTEN_KEEPALIVE what_to_stderr(intptr_t pointer) {
    auto error = reinterpret_cast<const std::exception *>(pointer);
    std::cerr << error->what() << std::endl;
}

std::function<void()> loop;
void main_loop() try {
    loop();
} catch (const std::exception& e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) try {
    std::cout << "Init..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    auto e = std::make_unique<engine>();

    std::cout << "Success." << std::endl;

    #ifdef __EMSCRIPTEN__
    loop = [&e]{ e->tick(); };

    emscripten_set_main_loop(main_loop, 0, 1);
    #else
    auto io_service = boost::asio::io_service{};
    auto timer = boost::asio::deadline_timer{io_service};
    auto tickrate = boost::posix_time::microseconds{16667};

    auto timer_tick = [&](const boost::system::error_code &ec) {
        if (ec == boost::asio::error::operation_aborted) {
            std::clog << "Main loop aborted." << std::endl;
            return;
        }
        loop();
    };

    timer.expires_at(timer.expires_at() + tickrate);
    timer.async_wait(timer_tick);

    loop = [&] {
        e->tick();
        timer.expires_at(timer.expires_at() + tickrate);
        timer.async_wait(timer_tick);
    };

    io_service.run();
    #endif

    SDL_Quit();

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
