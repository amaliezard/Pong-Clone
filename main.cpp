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

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 960;

constexpr float RED = 0.9765625f, GREEN = 0.97265625f, BLUE = 0.9609375f, OPACITY = 1.0f;

enum GameStatus { ACTIVE, FINISHED };

SDL_Window* window;
GameStatus gameStatus = ACTIVE;
ShaderProgram shaderProgram;


glm::mat4 viewMatrix, leftPaddleMatrix, rightPaddleMatrix, ballMatrix, projectionMatrix;

float previousFrameTime = 0.0f;


GLuint paddleTextureID, ballTextureID;


float leftPaddleY = 0.0f;
float rightPaddleY = 0.0f;
float ballXSpeed = 2.3f;
float ballYSpeed = 1.9f;
glm::vec3 ballPosition;

bool rightPaddleAuto = false;
bool isGameOver = false;

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
    window = SDL_CreateWindow("Paddle Game Redesign", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    shaderProgram.load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    viewMatrix = glm::mat4(1.0f);

    glUseProgram(shaderProgram.get_program_id());
    shaderProgram.set_projection_matrix(projectionMatrix);
    shaderProgram.set_view_matrix(viewMatrix);

    glClearColor(RED, GREEN, BLUE, OPACITY);

    // Load textures
    paddleTextureID = load_texture("textures/paddle.png");
    ballTextureID = load_texture("textures/ball.png");

    // Initialize ball position and scaling
    ballMatrix = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), glm::vec3(0.2f, 0.2f, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameStatus = FINISHED;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]) {
        leftPaddleY += 0.05f;
    }
    if (keys[SDL_SCANCODE_S]) {
        leftPaddleY -= 0.05f;
    }

    if (!rightPaddleAuto) {
        if (keys[SDL_SCANCODE_UP]) {
            rightPaddleY += 0.05f;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            rightPaddleY -= 0.05f;
        }
    }

    if (keys[SDL_SCANCODE_T]) {
        rightPaddleAuto = !rightPaddleAuto;
    }

    leftPaddleY = glm::clamp(leftPaddleY, -0.75f, 0.75f);
    rightPaddleY = glm::clamp(rightPaddleY, -0.75f, 0.75f);
}

bool check_collision(glm::vec3 ballPos, glm::vec3 paddlePos, float paddleWidth, float paddleHeight, float ballRadius) {
    float paddleHalfW = paddleWidth / 2.0f;
    float paddleHalfH = paddleHeight / 2.0f;

    bool collisionX = ballPos.x + ballRadius >= paddlePos.x - paddleHalfW &&
                      ballPos.x - ballRadius <= paddlePos.x + paddleHalfW;

    bool collisionY = ballPos.y + ballRadius >= paddlePos.y - paddleHalfH &&
                      ballPos.y - ballRadius <= paddlePos.y + paddleHalfH;

    return collisionX && collisionY;
}

void update() {
    float currentTime = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = currentTime - previousFrameTime;
    previousFrameTime = currentTime;

    if (isGameOver) return;

    leftPaddleMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, leftPaddleY, 0.0f));
    rightPaddleMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.6f, rightPaddleY, 0.0f));

    ballMatrix = glm::translate(ballMatrix, glm::vec3(ballXSpeed * deltaTime, ballYSpeed * deltaTime, 0.0f));
    ballPosition = glm::vec3(ballMatrix[3]);

    if (ballPosition.y + 0.1f >= 1.0f || ballPosition.y - 0.1f <= -1.0f) {
        ballYSpeed = -ballYSpeed;
    }

    glm::vec3 leftPaddlePos = glm::vec3(leftPaddleMatrix[3]);
    glm::vec3 rightPaddlePos = glm::vec3(rightPaddleMatrix[3]);

    if (check_collision(ballPosition, leftPaddlePos, 0.2f, 1.0f, 0.1f)) {
        ballXSpeed = fabs(ballXSpeed) * 1.2f;
    }
    if (check_collision(ballPosition, rightPaddlePos, 0.2f, 1.0f, 0.1f)) {
        ballXSpeed = -fabs(ballXSpeed) * 1.2f;
    }

    if (rightPaddleAuto) {
        rightPaddleY += 0.03f * sin(SDL_GetTicks() / 1000.0f);
        rightPaddleY = glm::clamp(rightPaddleY, -0.75f, 0.75f);
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texCoords[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    glVertexAttribPointer(shaderProgram.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(shaderProgram.get_position_attribute());

    glVertexAttribPointer(shaderProgram.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(shaderProgram.get_tex_coordinate_attribute());

    shaderProgram.set_model_matrix(leftPaddleMatrix);
    glBindTexture(GL_TEXTURE_2D, paddleTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shaderProgram.set_model_matrix(rightPaddleMatrix);
    glBindTexture(GL_TEXTURE_2D, paddleTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shaderProgram.set_model_matrix(ballMatrix);
    glBindTexture(GL_TEXTURE_2D, ballTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(shaderProgram.get_position_attribute());
    glDisableVertexAttribArray(shaderProgram.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(window);
}

void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    initialise();

    while (gameStatus == ACTIVE) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
