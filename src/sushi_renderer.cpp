#include "sushi_renderer.hpp"

#include "tracing.hpp"

#include <glm/gtc/matrix_inverse.hpp>

sushi_renderer::sushi_renderer(const glm::vec2& display_area, basic_shader_program& program, msdf_shader_program& program_msdf, cache<msdf_font>& font_cache, cache<sushi::texture_2d>& texture_cache) :
    display_area(display_area),
    program(&program),
    program_msdf(&program_msdf),
    font_cache(&font_cache),
    texture_cache(&texture_cache)
{
    rectangle_mesh = sushi::load_static_mesh_data(
        {{0.f, 1.f, 0.f},{0.f, 0.f, 0.f},{1.f, 0.f, 0.f},{1.f, 1.f, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

void sushi_renderer::begin() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void sushi_renderer::end() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void sushi_renderer::draw_rectangle(const std::string& texture, const glm::vec4& color, glm::vec2 position, glm::vec2 size) {
    auto proj = glm::ortho(0.f, display_area.x, 0.f, display_area.y, -1.f, 1.f);
    auto model_mat = glm::mat4(1.f);

    model_mat = glm::translate(model_mat, glm::vec3(position, 0.f));
    model_mat = glm::scale(model_mat, glm::vec3(size, 1.f));

    program->bind();
    program->set_cam_forward({0.0, 0.0, -1.0});
    program->set_normal_mat(glm::inverseTranspose(model_mat));
    program->set_texcoord_mat(glm::mat3(1));
    program->set_MVP(proj * model_mat);
    program->set_tint(color);

    sushi::set_texture(0, *texture_cache->get(texture));
    sushi::draw_mesh(rectangle_mesh);
}

float sushi_renderer::get_text_width(const std::string& text, const std::string& fontname) {
    auto font = font_cache->get(fontname);
    auto width = 0.f;

    for (auto c : text) {
        if (auto glyph = font->get_glyph(c)) {
            width += glyph->advance;
        }
    }

    return width;
}

void sushi_renderer::draw_text(const std::string& text, const std::string& fontname, const glm::vec4& color, glm::vec2 position, float size) {
    auto _trace = tracing::push("sushi_renderer::draw_text");
    auto font = font_cache->get(fontname);
    auto proj = glm::ortho(0.f, display_area.x, 0.f, display_area.y, -1.f, 1.f);
    auto model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(position, 0.f)), glm::vec3{size, size, 1.f});

    program_msdf->bind();
    program_msdf->set_msdf(0);
    program_msdf->set_pxRange(4.f);
    program_msdf->set_fgColor(color);
    program_msdf->set_MVP(proj * model);
    program_msdf->set_texelSize(msdf_font::TEX_SIZE);

    for (const auto& chunk : font->get_text(text).chunks) {
        sushi::set_texture(0, font->get_texture(chunk.texture_index));
        sushi::draw_mesh(chunk.mesh);
    }
}

void sushi_renderer::collect_garbage() const {
    for (const auto& [key, val] : font_cache->get_cache()) {
        val->clear_unused();
    }
}
