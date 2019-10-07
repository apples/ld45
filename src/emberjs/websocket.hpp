#pragma once

#include <functional>
#include <string>
#include <utility>

namespace emberjs {

    using handle = void*;
    using onopen_callback = void(void* self);
    using onmessage_callback = void(void* self, const char* msg);
    using onclose_callback = void(void* self, int code, const char* reason);

    enum state {
        INVALID = -1,
        CONNECTING = 0,
        OPEN = 1,
        CLOSING = 2,
        CLOSED = 3
    };

    extern "C" {
        extern handle ember_ws_create(const char* addr);
        extern int ember_ws_destroy(handle hdl);
        extern int ember_ws_onopen(handle hdl, onopen_callback* cb, void* data);
        extern int ember_ws_onmessage(handle hdl, onmessage_callback* cb, void* data);
        extern int ember_ws_onclose(handle hdl, onclose_callback* cb, void* data);
        extern int ember_ws_send(handle hdl, const char* msg);
        extern state ember_ws_get_state(handle hdl);
    }

    class websocket {
    public:
        websocket() = default;

        websocket(const websocket&) = delete;
        websocket(websocket&& other);
        websocket& operator=(const websocket&) = delete;
        websocket& operator=(websocket&& other);

        ~websocket();

        void connect(const std::string& address);
        void send(const std::string& msg);

        state get_state() const;

        std::function<void()> on_open;
        std::function<void(const std::string&)> on_message;
        std::function<void(int, const std::string&)> on_close;

    private:

        handle hdl = nullptr;
    };

} //namespace emberjs
