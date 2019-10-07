#pragma once

#include <sushi/mesh.hpp>
#include <sushi/shader.hpp>
#include <sushi/texture.hpp>

#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <string>
#include <unordered_map>
#include <memory>

struct FontDeleter {
    void operator()(msdfgen::FontHandle* ptr) {
        msdfgen::destroyFont(ptr);
    }
};

class msdf_font {
public:
    static constexpr int TEX_SIZE = 512;

    struct text_def_chunk {
        sushi::static_mesh mesh;
        int texture_index = 0;
    };

    struct text_def {
        std::vector<text_def_chunk> chunks;
        int hits = 0;
    };

    struct glyph_def {
        glm::vec2 pos[2] = {};
        glm::vec2 uv[2] = {};
        int texture_index = 0;
        float advance = 0;
    };

    msdf_font() = default;
    msdf_font(const std::string& filename);

    const text_def& get_text(const std::string& str) const;

    const sushi::texture_2d& get_texture(int i) const;

    const glyph_def* get_glyph(int unicode) const;

    void clear_unused() const;

private:
    std::unique_ptr<msdfgen::FontHandle, FontDeleter> font;
    mutable std::unordered_map<int, glyph_def> glyphs;
    mutable std::vector<sushi::texture_2d> textures;
    mutable std::unordered_map<std::string, text_def> texts;
    mutable int current_u = 0;
    mutable int current_v = 0;
    mutable int advance_v = 0;
};
