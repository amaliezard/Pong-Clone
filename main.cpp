/**
* Author: Amalie Zard
* Assignment: Pong Clone 
* Date due: 2024-14-10, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <cmath>
#include <iostream>

#define LOG(msg) std::cout << msg << std::endl;

constexpr int screen_width = 1280;
constexpr int screen_height = 960;

constexpr float red = 0.9765625f, green = 0.97265625f, blue = 0.9609375f, opacity = 1.0f;

enum game_status { active, finished };

SDL_Window* window;
game_status game_status = active;
ShaderProgram shader_program;

glm::mat4 view_matrix, left_paddle_matrix, right_paddle_matrix, ball_matrix, projection_matrix;

float previous_frame_time = 0.0f;

GLuint paddle_texture_id, ball_texture_id;

float left_paddle_y = 0.0f;
float right_paddle_y = 0.0f;
float ball_x_speed = 2.3f;
float ball_y_speed = 1.9f;
glm::vec3 ball_position;

bool right_paddle_auto = false;
bool is_game_over = false;

GLuint load_texture(const char* path) {
    int width, height, components;
    unsigned char* data = stbi_load(path, &width, &height, &components, STBI_rgb_alpha);
    if (!data) {
        LOG("Failed to load texture: " << path);
        exit(1);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);
    return texture;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Paddle Game Redesign", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    glViewport(0, 0, screen_width, screen_height);

    shader_program.load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    projection_matrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    view_matrix = glm::mat4(1.0f);

    glUseProgram(shader_program.get_program_id());
    shader_program.set_projection_matrix(projection_matrix);
    shader_program.set_view_matrix(view_matrix);

    glClearColor(red, green, blue, opacity);

    // Load textures
    paddle_texture_id = load_texture("textures/paddle.png");
    ball_texture_id = load_texture("textures/ball.png");

    // Initialize ball position and scaling
    ball_matrix = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), glm::vec3(0.2f, 0.2f, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            game_status = finished;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]) {
        left_paddle_y += 0.05f;
    }
    if (keys[SDL_SCANCODE_S]) {
        left_paddle_y -= 0.05f;
    }

    if (!right_paddle_auto) {
        if (keys[SDL_SCANCODE_UP]) {
            right_paddle_y += 0.05f;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            right_paddle_y -= 0.05f;
        }
    }

    if (keys[SDL_SCANCODE_T]) {
        right_paddle_auto = !right_paddle_auto;
    }

    left_paddle_y = glm::clamp(left_paddle_y, -0.75f, 0.75f);
    right_paddle_y = glm::clamp(right_paddle_y, -0.75f, 0.75f);
}

bool check_collision(glm::vec3 ball_pos, glm::vec3 paddle_pos, float paddle_width, float paddle_height, float ball_radius) {
    float paddle_half_w = paddle_width / 2.0f;
    float paddle_half_h = paddle_height / 2.0f;

    bool collision_x = ball_pos.x + ball_radius >= paddle_pos.x - paddle_half_w &&
                       ball_pos.x - ball_radius <= paddle_pos.x + paddle_half_w;

    bool collision_y = ball_pos.y + ball_radius >= paddle_pos.y - paddle_half_h &&
                       ball_pos.y - ball_radius <= paddle_pos.y + paddle_half_h;

    return collision_x && collision_y;
}

void update() {
    float current_time = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = current_time - previous_frame_time;
    previous_frame_time = current_time;

    if (is_game_over) return;

    left_paddle_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, left_paddle_y, 0.0f));
    right_paddle_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.6f, right_paddle_y, 0.0f));

    ball_matrix = glm::translate(ball_matrix, glm::vec3(ball_x_speed * delta_time, ball_y_speed * delta_time, 0.0f));
    ball_position = glm::vec3(ball_matrix[3]);

    if (ball_position.y + 0.1f >= 1.0f || ball_position.y - 0.1f <= -1.0f) {
        ball_y_speed = -ball_y_speed;
    }

    glm::vec3 left_paddle_pos = glm::vec3(left_paddle_matrix[3]);
    glm::vec3 right_paddle_pos = glm::vec3(right_paddle_matrix[3]);

    if (check_collision(ball_position, left_paddle_pos, 0.2f, 1.0f, 0.1f)) {
        ball_x_speed = fabs(ball_x_speed) * 1.2f;
    }
    if (check_collision(ball_position, right_paddle_pos, 0.2f, 1.0f, 0.1f)) {
        ball_x_speed = -fabs(ball_x_speed) * 1.2f;
    }

    if (right_paddle_auto) {
        right_paddle_y += 0.03f * sin(SDL_GetTicks() / 1000.0f);
        right_paddle_y = glm::clamp(right_paddle_y, -0.75f, 0.75f);
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float tex_coords[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    glVertexAttribPointer(shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(shader_program.get_position_attribute());

    glVertexAttribPointer(shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(shader_program.get_tex_coordinate_attribute());

    shader_program.set_model_matrix(left_paddle_matrix);
    glBindTexture(GL_TEXTURE_2D, paddle_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shader_program.set_model_matrix(right_paddle_matrix);
    glBindTexture(GL_TEXTURE_2D, paddle_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shader_program.set_model_matrix(ball_matrix);
    glBindTexture(GL_TEXTURE_2D, ball_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(shader_program.get_position_attribute());
    glDisableVertexAttribArray(shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(window);
}

void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    initialise();

    while (game_status == active) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
