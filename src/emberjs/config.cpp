#include "config.hpp"

#include "../utility.hpp"

#include <cstring>

#ifndef __EMSCRIPTEN__
namespace emberjs {
extern "C" {
    char* ember_config_get() {
        const char* str = R"({
            "display": {
                "width": 640,
                "height": 480
            },
            "volume": 1.0
        })";

        auto sz = std::strlen(str) + 1;
        auto r = static_cast<char*>(std::malloc(sz));
        memcpy(r, str, sz);
        return r;
    }
}
} // namespace emberjs
#endif

namespace emberjs {

    nlohmann::json get_config() {
        auto config = ember_config_get();
        EMBER_DEFER { free(config); };
        return nlohmann::json::parse(config);
    }

} //namespace emberjs
