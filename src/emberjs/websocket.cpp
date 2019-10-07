#include "websocket.hpp"

#ifndef __EMSCRIPTEN__
namespace emberjs {
extern "C" {
    handle ember_ws_create(const char* addr) { return nullptr; }
    int ember_ws_destroy(handle hdl) { return 0; }
    int ember_ws_onopen(handle hdl, onopen_callback* cb, void* data) { return 0; }
    int ember_ws_onmessage(handle hdl, onmessage_callback* cb, void* data) { return 0; }
    int ember_ws_onclose(handle hdl, onclose_callback* cb, void* data) { return 0; }
    int ember_ws_send(handle hdl, const char* msg) { return 0; }
    state ember_ws_get_state(handle hdl) { return INVALID; }
}
}
#endif

namespace emberjs {
    namespace { // static

        void onopen_trampoline(void* self) {
            auto& ws = *static_cast<websocket*>(self);
            if (ws.on_open) {
                ws.on_open();
            }
        }

        void onmessage_trampoline(void* self, const char* message) {
            auto& ws = *static_cast<websocket*>(self);
            if (ws.on_message) {
                ws.on_message(message);
            }
        }

        void onclose_trampoline(void* self, int code, const char* reason) {
            auto& ws = *static_cast<websocket*>(self);
            if (ws.on_close) {
                ws.on_close(code, reason);
            }
        }

        void set_handlers(handle hdl, websocket* self) {
            if (!ember_ws_onopen(hdl, &onopen_trampoline, self)) {
                throw std::runtime_error("Failed to set websocket onopen callback.");
            }
            if (!ember_ws_onmessage(hdl, &onmessage_trampoline, self)) {
                throw std::runtime_error("Failed to set websocket onmessage callback.");
            }
            if (!ember_ws_onclose(hdl, &onclose_trampoline, self)) {
                throw std::runtime_error("Failed to set websocket onclose callback.");
            }
        }

        void unset_handlers(handle hdl) {
            if (!ember_ws_onopen(hdl, nullptr, nullptr)) {
                throw std::runtime_error("Failed to unset websocket onopen callback.");
            }
            if (!ember_ws_onmessage(hdl, nullptr, nullptr)) {
                throw std::runtime_error("Failed to unset websocket onmessage callback.");
            }
            if (!ember_ws_onclose(hdl, nullptr, nullptr)) {
                throw std::runtime_error("Failed to unset websocket onclose callback.");
            }
        }

    } // static

    websocket::websocket(websocket&& other) :
        on_open(std::move(other.on_open)),
        on_message(std::move(other.on_message)),
        on_close(std::move(other.on_close)),
        hdl(std::exchange(other.hdl, nullptr))
    {
        if (hdl) {
            set_handlers(hdl, this);
        }
    }

    websocket& websocket::operator=(websocket&& other) {
        if (hdl) {
            unset_handlers(hdl);
            if (!ember_ws_destroy(hdl)) {
                throw std::runtime_error("Failed to close websocket.");
            }
        }

        on_open = std::move(other.on_open);
        on_message = std::move(other.on_message);
        on_close = std::move(other.on_close);
        hdl = std::exchange(other.hdl, nullptr);

        if (hdl) {
            set_handlers(hdl, this);
        }

        return *this;
    }

    websocket::~websocket() {
        if (hdl) {
            unset_handlers(hdl);
            if (!ember_ws_destroy(hdl)) {
                throw std::runtime_error("Failed to close websocket.");
            }
        }
    }

    void websocket::connect(const std::string& address) {
        hdl = ember_ws_create(address.c_str());
        set_handlers(hdl, this);
    }

    void websocket::send(const std::string& msg) {
        if (!ember_ws_send(hdl, msg.c_str())) {
            throw std::runtime_error("Failed to send message to websocket.");
        }
    }

    state websocket::get_state() const {
        return ember_ws_get_state(hdl);
    }

} //namespace emberjs
