//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_SHADER_HPP
#define SUSHI_SHADER_HPP

#include "common.hpp"

#include "gl.hpp"

#include <stdexcept>
#include <string>
#include <vector>

/// Sushi
namespace sushi {

class shader_error : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

/// Shader types.
enum class shader_type : GLenum {
    VERTEX = GL_VERTEX_SHADER,
    FRAGMENT = GL_FRAGMENT_SHADER
};

/// Deleter for OpenGL shader objects.
struct shader_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        GLuint shader = p;
        glDeleteShader(shader);
    }
};

/// Deleter for OpenGL shader program objects.
struct program_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        GLuint program = p;
        glDeleteProgram(program);
    }
};

/// A unique handle to an OpenGL shader object.
using unique_shader = unique_gl_resource<shader_deleter>;

/// A unique handle to an OpenGL shader program object.
using unique_program = unique_gl_resource<program_deleter>;

/// Creates a unique OpenGL shader object.
/// \return A unique shader object.
inline unique_shader make_unique_shader(GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        throw std::runtime_error("Failed to create shader!");
    }
    return unique_shader(shader);
}

/// Creates a unique OpenGL shader program object.
/// \return A unique shader program object.
inline unique_program make_unique_program() {
    GLuint program = glCreateProgram();
    if (program == 0) {
        throw std::runtime_error("Failed to create shader program!");
    }
    return unique_program(program);
}

/// Loads and compiles an OpenGL shader from an array of strings.
/// \param type The type of shader to be created.
/// \param code Shader source.
/// \return A unique shader object.
unique_shader compile_shader(shader_type type, std::vector<const GLchar*> code);

/// Loads and compiles an OpenGL shader from a file.
/// \param type The type of shader to be created.
/// \param fname File name.
/// \return A unique shader object.
unique_shader compile_shader_file(shader_type type, const std::string& fname);

/// Links a shader program.
/// \pre All of the shaders are compiled.
/// \param shaders List of shaders to link.
/// \return Unique handle to the new shader program.
unique_program link_program(const std::vector<unique_shader>& shaders);

/// Sets the current shader program.
/// \pre The program was successfully linked.
/// \param program Shader program to set.
inline void set_program(const unique_program& program) {
    glUseProgram(program.get());
}

/// Sets an integer uniform of the current program.
/// \param location Location of the uniform.
/// \param i Integer.
inline void set_current_program_uniform(GLint location, const GLint& i) {
    glUniform1i(location, i);
}

/// Sets a float uniform of the current program.
/// \param location Location of the uniform.
/// \param f Float.
inline void set_current_program_uniform(GLint location, const GLfloat& f) {
    glUniform1f(location, f);
}

/// Sets a vec2 uniform of the current program.
/// \param location Location of the uniform.
/// \param vec Vector.
inline void set_current_program_uniform(GLint location, const glm::vec2& vec) {
    glUniform2fv(location, 1, glm::value_ptr(vec));
}

/// Sets a vec3 uniform of the current program.
/// \param location Location of the uniform.
/// \param vec Vector.
inline void set_current_program_uniform(GLint location, const glm::vec3& vec) {
    glUniform3fv(location, 1, glm::value_ptr(vec));
}

/// Sets a vec4 uniform of the current program.
/// \param location Location of the uniform.
/// \param vec Vector.
inline void set_current_program_uniform(GLint location, const glm::vec4& vec) {
    glUniform4fv(location, 1, glm::value_ptr(vec));
}

/// Sets a mat3 uniform of the current program.
/// \param location Location of the uniform.
/// \param mat Matrix.
inline void set_current_program_uniform(GLint location, const glm::mat3& mat) {
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

/// Sets a mat4 uniform of the current program.
/// \param location Location of the uniform.
/// \param mat Matrix.
inline void set_current_program_uniform(GLint location, const glm::mat4& mat) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

/// Sets a uniform in the shader program.
/// \param program The shader program.
/// \param name The name of the uniform.
/// \param data The value to set to the uniform.
template<typename T>
void set_program_uniform(const unique_program& program, const std::string& name, const T& data) {
    sushi::set_program(program);
    auto location = glGetUniformLocation(program.get(), name.data());
    set_current_program_uniform(location, data);
}

/// Sets a uniform in the currently bound shader program.
/// \param name The name of the uniform.
/// \param data The value to set to the uniform.
template<typename T>
void set_uniform(const std::string& name, const T& data) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    auto location = glGetUniformLocation(program, name.data());
    set_current_program_uniform(location, data);
}

/// Base class for a safe shader wrapper.
/// Derived classes should cache uniform locations and implement setters for them.
class shader_base {
public:
    /// Default constructor.
    shader_base() = default;

    /// Compiles and links the provided source files.
    /// \param sources The source files.
    shader_base(std::initializer_list<std::pair<sushi::shader_type, std::string>> sources);

    /// Sets this program as the current one.
    void bind();

    /// Gets the location of the uniform with the given name.
    /// Requires this shader to be bound.
    /// \param name Name of the uniform.
    /// \returns The location of the uniform or -1 if not found.
    GLint get_uniform_location(const std::string& name) const;

    /// Gets the program.
    /// \returns Reference to the program.
    const unique_program& get_program() const;

private:
    unique_program program;
};

}

#endif //SUSHI_SHADER_HPP
