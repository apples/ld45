/// \file Sushi test file.


#include <glad/glad.h>
#include <sushi/sushi.hpp>
#include <GLFW/glfw3.h>

#include <iostream>

using namespace std;

using glm::vec2;
using glm::vec3;
using glm::mat4;

class example_shader : public sushi::shader_base {
public:
    example_shader() :
        shader_base({
            {sushi::shader_type::VERTEX, "assets/vert.glsl"},
            {sushi::shader_type::FRAGMENT, "assets/frag.glsl"}
        })
    {
        bind();
        uniforms.MVP = get_uniform_location("MVP");
        uniforms.DiffuseTexture = get_uniform_location("DiffuseTexture");
        uniforms.GrayScale = get_uniform_location("GrayScale");
    }

    void set_MVP(const mat4& mat) {
        sushi::set_current_program_uniform(uniforms.MVP, mat);
    }

    void set_DiffuseTexture(int i) {
        sushi::set_current_program_uniform(uniforms.DiffuseTexture, i);
    }

    void set_GrayScale(int i) {
        sushi::set_current_program_uniform(uniforms.GrayScale, i);
    }

private:
    struct {
        GLint MVP;
        GLint DiffuseTexture;
        GLint GrayScale;
    } uniforms;
};

struct window_data {
    bool a_pressed = false;
    bool a_released = false;
    bool a_down = false;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (auto user_pointer = glfwGetWindowUserPointer(window)) {
        auto data = static_cast<window_data*>(user_pointer);
        if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            data->a_pressed = true;
            data->a_down = true;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
            data->a_released= true;
            data->a_down = false;
        }
    }
}

int main() try {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW");
    }

    auto window = glfwCreateWindow(800, 600, "Sushi Test", nullptr, nullptr);

    if (!window) {
        throw std::runtime_error("Failed to open window");
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to load OpenGL extensions.");
    }

    glEnable(GL_DEPTH_TEST);

    auto texture = sushi::load_texture_2d("assets/test.png", false, false, false, false);
    auto mesh = sushi::load_static_mesh_file("assets/test.obj");
    auto program = example_shader();
    auto xrot = 0.f;
    auto yrot = 0.f;

    auto data = window_data{};

    glfwSetWindowUserPointer(window, &data);

    while (!glfwWindowShouldClose(window)) {
        data.a_pressed = false;
        data.a_released = false;

        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        program.bind();

        auto proj_mat = glm::perspective(90.f, 4.f / 3.f, 0.01f, 100.f);
        auto view_mat = glm::translate(glm::mat4(1.f), glm::vec3{0.f, 0.f, -5.f});
        auto model_mat = glm::mat4(1.f);

        model_mat = glm::rotate(model_mat, xrot, glm::vec3{1, 0, 0});
        model_mat = glm::rotate(model_mat, yrot, glm::vec3{0, 1, 0});

        xrot += 0.0001;
        yrot += 0.001;

        auto mvp = proj_mat * view_mat * model_mat;

        program.set_MVP(mvp);
        program.set_DiffuseTexture(0);

        if (data.a_pressed) {
            clog << "A pressed." << endl;
        }

        if (data.a_released) {
            clog << "A released." << endl;
        }

        program.set_GrayScale(data.a_down);

        sushi::set_texture(0, texture);
        sushi::draw_mesh(mesh);

        glfwSwapBuffers(window);
    }

    return EXIT_SUCCESS;
} catch (const exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    glfwTerminate();
    return EXIT_FAILURE;
}
