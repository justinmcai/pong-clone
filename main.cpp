#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#include <cstdio>
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };
enum GameMode { SINGLE, MULTI};

constexpr float WINDOW_SIZE_MULT = 1.0f;

constexpr int WINDOW_WIDTH = 800 * WINDOW_SIZE_MULT,
WINDOW_HEIGHT = 600 * WINDOW_SIZE_MULT;

constexpr float BG_RED = 0.0f,
BG_GREEN = 0.0f,
BG_BLUE = 0.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// source: https://yorukura-anime.com/
constexpr char KANO_SPRITE_FILEPATH[] = "paddle.png",
MAHIRU_SPRITE_FILEPATH[] = "paddle.png",
BALL_SPRITE_FILEPATH[] = "ball.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE = glm::vec3(0.2f, 1.0f, 0.0f),
INIT_BALL_SCALE = glm::vec3(0.2f, 0.2f, 0.0f),
INIT_POS_KANO = glm::vec3(4.9f, 0.0f, 0.0f),
INIT_POS_MAHIRU = glm::vec3(-4.9f, 0.0f, 0.0f),
INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
GameMode g_game_mode = MULTI;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_kano_matrix, g_projection_matrix, g_trans_matrix, g_mahiru_matrix, g_ball_matrix;

float g_previous_ticks = 0.0f;

GLuint g_kano_texture_id;
GLuint g_mahiro_texture_id;
GLuint g_ball_texture_id;

glm::vec3 g_kano_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_kano_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_mahiru_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_mahiru_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);

//paddle speed
float g_kano_speed = 5.0f;  // move 1 unit per second
float g_ball_speed = 2.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

constexpr GLint NUMBER_OF_TEXTURES = 1;  // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
constexpr GLint TEXTURE_BORDER = 0;  // this value MUST be zero

// console
FILE* fp;

void initConsole() {
    AllocConsole();
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
}
// end console

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("PONG",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr)
    {
        shutdown();
    }
#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_kano_matrix = glm::mat4(1.0f);
    g_mahiru_matrix = glm::mat4(1.0f);
    //here = do I need to translate ball matrix?
    g_ball_matrix = glm::mat4(1.0f);
    g_mahiru_matrix = glm::translate(g_mahiru_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_mahiru_position += g_mahiru_movement;

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_kano_texture_id = load_texture(KANO_SPRITE_FILEPATH);
    g_mahiro_texture_id = load_texture(MAHIRU_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_kano_movement = glm::vec3(0.0f);
    g_mahiru_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_t:
                // Move the player right
                g_game_mode = SINGLE;
                break;

            case SDLK_q:
                // Quit the game with a keystroke
                g_app_status = TERMINATED;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }


    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_W])
    {
        g_mahiru_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_mahiru_movement.y = -1.0f;
    }

    if (g_game_mode == MULTI) {
        if (key_state[SDL_SCANCODE_UP])
        {
            g_kano_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            g_kano_movement.y = -1.0f;
        }
    }
    //if (key_state[SDL_SCANCODE_RIGHT])
    //{
    //    g_kano_movement.x = 1.0f;
    //}
    //else if (key_state[SDL_SCANCODE_LEFT])
    //{
    //    g_kano_movement.x = -1.0f;
    //}

    // This makes sure that the player can't "cheat" their way into moving
    // faster
    if (glm::length(g_kano_movement) > 1.0f)
    {
        g_kano_movement = glm::normalize(g_kano_movement);
    }

    if (glm::length(g_mahiru_movement) > 1.0f)
    {
        g_mahiru_movement = glm::normalize(g_mahiru_movement);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    float sign = 1.0f;

    if (g_ball_movement == glm::vec3(0.0f, 0.0f, 0.0f)) {
        //g_ball_movement = glm::vec3(-1.0f, 1.0f, 0.0f);
    }

    if (INIT_POS_BALL.y + g_ball_position.y >= 3.7f) {
        g_ball_movement.y = -1.0f;
    }
    if (INIT_POS_BALL.y + g_ball_position.y <= -3.7f) {
        g_ball_movement.y = 1.0f;
    }
    if (INIT_POS_BALL.x + g_ball_position.x >= 5.0f) {
        g_ball_movement.x = 0.0f;
        g_ball_movement.y = 0.0f;
        g_app_status = TERMINATED;
    }
    if (INIT_POS_BALL.x + g_ball_position.x <= -5.0f) {
        g_ball_movement.x = 0.0f;
        g_ball_movement.y = 0.0f;
        g_app_status = TERMINATED;
    }
    //left paddle
    float x_distance_kano = fabs(g_kano_position.x + INIT_POS_KANO.x - g_ball_position.x) -
        ((INIT_SCALE.x + INIT_BALL_SCALE.x) / 2.0f);

    float y_distance_kano = fabs(g_kano_position.y + INIT_POS_KANO.y - g_ball_position.y) -
        ((INIT_SCALE.y + INIT_BALL_SCALE.y) / 2.0f);

    if (x_distance_kano < 0.0f && y_distance_kano < 0.0f)
    {
        g_ball_movement.x = -1.0f;
    }
    //right paddle
    float x_distance_mahiru = fabs(g_mahiru_position.x + INIT_POS_MAHIRU.x - g_ball_position.x) -
        ((INIT_SCALE.x + INIT_BALL_SCALE.x) / 2.0f);

    float y_distance_mahiru = fabs(g_mahiru_position.y + INIT_POS_MAHIRU.y - g_ball_position.y) -
        ((INIT_SCALE.y + INIT_BALL_SCALE.y) / 2.0f);

    if (x_distance_mahiru < 0.0f && y_distance_mahiru < 0.0f)
    {
        g_ball_movement.x = 1.0f;
    }

    //move ball
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    //LOG
    //std::cout << "x = " << INIT_POS_BALL.x + g_ball_position.x << ": y = " << INIT_POS_BALL.y + g_ball_position.y << std::endl;

    //collision detection: paddle-border
    if (INIT_POS_KANO.y + g_kano_position.y <= 3.3f && INIT_POS_KANO.y + g_kano_position.y >= -3.3f)
    {
        if (g_game_mode == SINGLE) {
            g_kano_movement.y = -1.0f;
        }
        //movement :: add direction * units per second * elapsed time
        g_kano_position += sign * g_kano_movement * g_kano_speed * delta_time;
        std::cout << "y = " << INIT_POS_KANO.y + g_kano_position.y << std::endl;
    }
    else if (INIT_POS_KANO.y + g_kano_position.y >= 3.3f){
        //movement :: add direction * units per second * elapsed time
        g_kano_position.y = 3.29999999999f;
        if (g_game_mode == SINGLE) {
            sign = -1.0f;
        }
    }
    else {
        g_kano_position.y = -3.29999999999f;
        if (g_game_mode == SINGLE) {
            sign = 1.0f;
        }
    }
    //mahiru paddle
    if (INIT_POS_MAHIRU.y + g_mahiru_position.y <= 3.3f && INIT_POS_MAHIRU.y + g_mahiru_position.y >= -3.3f)
    {
        g_mahiru_position += g_mahiru_movement * g_kano_speed * delta_time;
    }
    else if (INIT_POS_MAHIRU.y + g_mahiru_position.y >= 3.3f) {
        g_mahiru_position.y = 3.29999999999f;
    }
    else {
        g_mahiru_position.y = -3.29999999999f;
    }

    //Regular update stuff
    g_kano_matrix = glm::mat4(1.0f);
    g_kano_matrix = glm::translate(g_kano_matrix, INIT_POS_KANO);
    g_kano_matrix = glm::translate(g_kano_matrix, g_kano_position);

    g_mahiru_matrix = glm::mat4(1.0f);
    g_mahiru_matrix = glm::translate(g_mahiru_matrix, INIT_POS_MAHIRU);
    g_mahiru_matrix = glm::translate(g_mahiru_matrix, g_mahiru_position);

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    g_kano_matrix = glm::scale(g_kano_matrix, INIT_SCALE);
    g_mahiru_matrix = glm::scale(g_mahiru_matrix, INIT_SCALE);
    g_ball_matrix = glm::scale(g_ball_matrix, INIT_BALL_SCALE);






}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_kano_matrix, g_kano_texture_id);
    draw_object(g_mahiru_matrix, g_mahiro_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);


    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initConsole();
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
