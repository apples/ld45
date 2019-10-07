#include "font.hpp"

#include "tracing.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // static

msdfgen::FreetypeHandle* font_init() {
    static const auto ft = msdfgen::initializeFreetype();
    return ft;
}

} // static

msdf_font::msdf_font(const std::string& fontname) {
    auto ft = font_init();

    if (!ft) {
        throw std::runtime_error("Failed to initialize FreeType.");
    }

    font = decltype(font)(msdfgen::loadFont(ft, fontname.c_str()));

    if (!font) {
        throw std::runtime_error("Failed to load font "+fontname+".");
    }
}

const msdf_font::text_def& msdf_font::get_text(const std::string& str) const {
    auto _trace = tracing::push("msdf_font::get_text");

    auto iter = texts.find(str);

    if (iter == end(texts)) {
        auto _trace = tracing::push("msdf_font::get_text[generate]");

        struct data_t {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec2> texcoords;
            std::vector<glm::vec3> normals = {{0.f, 0.f, 1.f}};
            std::vector<sushi::Tri> tris;
        };

        std::unordered_map<int, data_t> tex2data;

        auto push_quad = [](data_t& data, std::array<glm::vec3, 4> pos, std::array<glm::vec2, 4> uv) {
            data.positions.insert(end(data.positions), begin(pos), end(pos));
            data.texcoords.insert(end(data.texcoords), begin(uv), end(uv));

            const auto pos_idx = data.positions.size() - 4;
            const auto uv_idx = data.positions.size() - 4;

            data.tris.push_back({{
                {pos_idx, 0, uv_idx},
                {pos_idx + 1, 0, uv_idx + 1},
                {pos_idx + 2, 0, uv_idx + 2},
            }});

            data.tris.push_back({{
                {pos_idx + 2, 0, uv_idx + 2},
                {pos_idx + 3, 0, uv_idx + 3},
                {pos_idx, 0, uv_idx},
            }});
        };

        {
            float advance = 0;

            for (const auto& c : str) {
                auto codepoint = int(c); // not proper decoding :(

                auto* glyph = get_glyph(codepoint);

                if (!glyph) continue;

                auto& data = tex2data[glyph->texture_index];

                push_quad(
                    data,
                    {{
                        { glyph->pos[0].x + advance, glyph->pos[0].y, 0.f },
                        { glyph->pos[0].x + advance, glyph->pos[1].y, 0.f },
                        { glyph->pos[1].x + advance, glyph->pos[1].y, 0.f },
                        { glyph->pos[1].x + advance, glyph->pos[0].y, 0.f },
                    }},
                    {{
                        glyph->uv[0],
                        { glyph->uv[0].x, glyph->uv[1].y },
                        glyph->uv[1],
                        { glyph->uv[1].x, glyph->uv[0].y },
                    }});

                advance += glyph->advance;
            }
        }

        auto text = text_def{};

        text.chunks.reserve(tex2data.size());

        for (const auto& [texture_index, data] : tex2data) {
            auto g = text_def_chunk{};

            g.mesh = sushi::load_static_mesh_data(data.positions, data.normals, data.texcoords, data.tris);
            g.texture_index = texture_index;

            text.chunks.push_back(std::move(g));
        }

        iter = texts.insert_or_assign(str, std::move(text)).first;
    }

    iter->second.hits += 1;

    return iter->second;
}

const sushi::texture_2d& msdf_font::get_texture(int i) const {
    return textures.at(i);
}

const msdf_font::glyph_def* msdf_font::get_glyph(int unicode) const {
    if (!glyphs.count(unicode)) {
        msdfgen::Shape shape;
        double advance;

        if (!msdfgen::loadGlyph(shape, font.get(), unicode, &advance)) {
            return nullptr;
        }

        shape.normalize();
        msdfgen::edgeColoringSimple(shape, 3.0);

        double left = 0, bottom = 0, right = 0, top = 0;
        shape.bounds(left, bottom, right, top);

        left -= 2;
        bottom -= 2;
        right += 2;
        top += 2;

        msdfgen::Bitmap<msdfgen::FloatRGB> msdf(right - left, top - bottom);
        msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(-left, -bottom));

        std::vector<unsigned char> pixels;
        pixels.reserve(4 * msdf.width() * msdf.height());
        for (int y = 0; y < msdf.height(); ++y) {
            for (int x = 0; x < msdf.width(); ++x) {
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).r * 0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).g * 0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).b * 0x100), 0xff));
                pixels.push_back(255);
            }
        }

        double em;
        msdfgen::getFontScale(em, font.get());

        left /= em;
        right /= em;
        bottom /= em;
        top /= em;
        advance /= em;

        auto g = glyph_def{};

        if (current_u + msdf.width() > TEX_SIZE) {
            current_u = 0;
            current_v += advance_v + 1;
            advance_v = 0;
        }

        if (textures.empty() || current_v + msdf.height() > TEX_SIZE) {
            current_u = 0;
            current_v = 0;
            advance_v = 0;

            auto texture = sushi::texture_2d{};
            texture.handle = sushi::make_unique_texture();
            texture.width = TEX_SIZE;
            texture.height = TEX_SIZE;

            glBindTexture(GL_TEXTURE_2D, texture.handle.get());
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            const auto data = std::vector<GLubyte>(TEX_SIZE * TEX_SIZE * 4, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

            g.texture_index = textures.size();
            textures.push_back(std::move(texture));
        } else {
            g.texture_index = textures.size() - 1;
            glBindTexture(GL_TEXTURE_2D, textures.back().handle.get());
        }

        glTexSubImage2D(
            GL_TEXTURE_2D, 0, current_u, current_v, msdf.width(), msdf.height(), GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
        
        const auto u = double(current_u) / double(TEX_SIZE);
        const auto v = double(current_v) / double(TEX_SIZE);
        const auto u2 = double(current_u + msdf.width()) / double(TEX_SIZE);
        const auto v2 = double(current_v + msdf.height()) / double(TEX_SIZE);

        g.pos[0] = {left, bottom};
        g.pos[1] = {right, top};
        g.uv[0] = {u, v};
        g.uv[1] = {u2, v2};
        g.advance = advance;

        advance_v = std::max(advance_v, msdf.height());
        current_u += msdf.width() + 1;

        glyphs[unicode] = std::move(g);
    }

    return &glyphs[unicode];
}

void msdf_font::clear_unused() const {
    for (auto iter = begin(texts); iter != end(texts);) {
        if (iter->second.hits == 0) {
            iter = texts.erase(iter);
        } else {
            iter->second.hits = 0;
            ++iter;
        }
    }
}
