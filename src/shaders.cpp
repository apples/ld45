#include "shaders.hpp"

basic_shader_program::basic_shader_program(const std::string& vert, const std::string& frag) :
    sushi::shader_base({
        {sushi::shader_type::VERTEX, vert},
        {sushi::shader_type::FRAGMENT, frag},
    })
{
    bind();

    uniforms.MVP = get_uniform_location("MVP");
    uniforms.normal_mat = get_uniform_location("normal_mat");
    uniforms.texcoord_mat = get_uniform_location("texcoord_mat");
    uniforms.s_texture = get_uniform_location("s_texture");
    uniforms.cam_forward = get_uniform_location("cam_forward");
    uniforms.tint = get_uniform_location("tint");
    uniforms.hue = get_uniform_location("hue");
    uniforms.saturation = get_uniform_location("saturation");
}

void basic_shader_program::set_MVP(const glm::mat4& mat) {
    sushi::set_current_program_uniform(uniforms.MVP, mat);
}

void basic_shader_program::set_normal_mat(const glm::mat4& mat) {
    sushi::set_current_program_uniform(uniforms.normal_mat, mat);
}

void basic_shader_program::set_texcoord_mat(const glm::mat3& mat) {
    sushi::set_current_program_uniform(uniforms.texcoord_mat, mat);
}

void basic_shader_program::set_s_texture(GLint i) {
    sushi::set_current_program_uniform(uniforms.s_texture, i);
}

void basic_shader_program::set_cam_forward(const glm::vec3& vec) {
    sushi::set_current_program_uniform(uniforms.cam_forward, vec);
}

void basic_shader_program::set_tint(const glm::vec4& v) {
    sushi::set_current_program_uniform(uniforms.tint, v);
}

void basic_shader_program::set_hue(float f) {
    sushi::set_current_program_uniform(uniforms.hue, f);
}

void basic_shader_program::set_saturation(float f) {
    sushi::set_current_program_uniform(uniforms.saturation, f);
}

msdf_shader_program::msdf_shader_program(const std::string& vertfile, const std::string& fragfile) :
    sushi::shader_base({
        {sushi::shader_type::VERTEX, vertfile},
        {sushi::shader_type::FRAGMENT, fragfile}
    })
{
    bind();
    uniforms.MVP = get_uniform_location("MVP");
    uniforms.normal_mat = get_uniform_location("normal_mat");
    uniforms.msdf = get_uniform_location("msdf");
    uniforms.pxRange = get_uniform_location("pxRange");
    uniforms.fgColor = get_uniform_location("fgColor");
    uniforms.texelSize = get_uniform_location("texelSize");
}

void msdf_shader_program::set_MVP(const glm::mat4& mat) {
    sushi::set_current_program_uniform(uniforms.MVP, mat);
}

void msdf_shader_program::set_normal_mat(const glm::mat4& mat) {
    sushi::set_current_program_uniform(uniforms.normal_mat, mat);
}

void msdf_shader_program::set_msdf(int slot) {
    sushi::set_current_program_uniform(uniforms.msdf, slot);
}

void msdf_shader_program::set_pxRange(float f) {
    sushi::set_current_program_uniform(uniforms.pxRange, f);
}

void msdf_shader_program::set_fgColor(const glm::vec4& vec) {
    sushi::set_current_program_uniform(uniforms.fgColor, vec);
}

void msdf_shader_program::set_texelSize(float f) {
    sushi::set_current_program_uniform(uniforms.texelSize, f);
}
