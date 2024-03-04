/**
* Author: Matthew Ma
* Assignment: Pong Clone
* Date due: 2024-03-02, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_GLEXT_PROTOTYPES 1
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

// —— NEW STUFF —— //
#include <ctime>   //
#include "cmath"   //
// ——————————————— //

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl",
PLAYER_SPRITE_FILEPATH[] = "assets/mario.png",
PLAYER_SPRITE_FILEPATH_2[] = "assets/wii.png",
PLAYER_SPRITE_FILEPATH_3[] = "assets/ball.png";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float MINIMUM_COLLISION_DISTANCE = 1.0f;

const int   NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

SDL_Window* g_display_window;
bool  g_game_is_running = true;
bool  game_over = false;
float g_previous_ticks = 0.0f;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix,
g_model_matrix,
g_projection_matrix,
g_model_matrix_2,
g_ball_matrix;

GLuint g_player_texture_id,
g_player_texture_id_2,
g_ball_texture_id;

glm::vec3 g_player_position = glm::vec3(-4.8f, 0.0f, 0.0f);
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_scale = glm::vec3(0.5f, 1.2, 0.0f);

glm::vec3 g_player_position_2 = glm::vec3(4.8f, 0.0f, 0.0f);
glm::vec3 g_player_movement_2 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_scale_2 = glm::vec3(0.5f, 1.2f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(2.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(-0.5f, 0.2f, 0.0f);
glm::vec3 g_ball_scale = glm::vec3(0.75f, 0.75f, 0.0f);

float g_player_speed = 6.0f;
float g_ball_speed = 5.0f;
float cpu_move = 1.0f;

const float MAX_X = 5.0f,
MAX_Y = 3.75f,
MIN_X = -MAX_X,
MIN_Y = -MAX_Y;

bool cpu_playing = false;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Collisions!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_model_matrix = glm::mat4(1.0f);

    g_model_matrix_2 = glm::mat4(1.0f);
    g_model_matrix_2 = glm::translate(g_model_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_player_position_2 += g_player_movement_2;

    g_ball_matrix = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_player_texture_id_2 = load_texture(PLAYER_SPRITE_FILEPATH_2);
    g_ball_texture_id = load_texture(PLAYER_SPRITE_FILEPATH_3);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    g_player_movement = glm::vec3(0.0f);
    g_player_movement_2 = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_game_is_running = false;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (!game_over)
    {
        if (key_state[SDL_SCANCODE_W])
        {
            g_player_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_S])
        {
            g_player_movement.y = -1.0f;
        }

        if (cpu_playing == false)
        {
            if (key_state[SDL_SCANCODE_UP])
            {
                g_player_movement_2.y = 1.0f;
            }
            else if (key_state[SDL_SCANCODE_DOWN])
            {
                g_player_movement_2.y = -1.0f;
            }
        }
        else
        {
            g_player_movement_2.y = cpu_move;
        }
        
        if (key_state[SDL_SCANCODE_T])
        {
            cpu_playing = true;
        }
 
    }

    if (glm::length(g_player_movement) > 1.0f)
    {
        g_player_movement = glm::normalize(g_player_movement);
    }

    if (glm::length(g_player_movement_2) > 1.0f)
    {
        g_player_movement_2 = glm::normalize(g_player_movement_2);
    }

    if (glm::length(g_ball_movement) > 1.0f)
    {
        g_ball_movement = glm::normalize(g_ball_movement);
    }
}

bool collided(glm::vec3 ball_pos, glm::vec3 obj_pos, glm::vec3 obj_sca)
{
    /** ———— COLLISION DETECTION ———— **/
    float x_distance = fabs(ball_pos.x - obj_pos.x) - .5;
    float y_distance = fabs(ball_pos.y - obj_pos.y) - .5;

    if (x_distance < 0.0f && y_distance < 0.0f)
    {
        return true;
    }
    return false;
}
void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_model_matrix = glm::mat4(1.0f);
    g_player_position += g_player_movement * g_player_speed * delta_time;
    g_model_matrix = glm::translate(g_model_matrix, g_player_position);
    g_model_matrix = glm::scale(g_model_matrix, g_player_scale);

    //Player 2
    g_model_matrix_2 = glm::mat4(1.0f);
    g_player_position_2 += g_player_movement_2 * g_player_speed * delta_time;
    g_model_matrix_2 = glm::translate(g_model_matrix_2, g_player_position_2);
    g_model_matrix_2 = glm::scale(g_model_matrix_2, g_player_scale_2);

    //ball
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    g_ball_matrix = glm::scale(g_ball_matrix, g_ball_scale);
    
    if (g_ball_position.x > MAX_X)
    {
        game_over = true;
    }
    if (g_ball_position.x < MIN_X)
    {
        game_over = true;
    }
    if (game_over) 
    {
        g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    // —————————————————————— NEW STUFF ——————————————————————— //
    if (!game_over)
    {
        if (collided(g_ball_position, g_player_position, g_player_scale))
        {
            g_ball_movement.x = 0.5;
        }
        if (collided(g_ball_position, g_player_position_2, g_player_scale_2))
        {
            g_ball_movement.x = -0.5;
        }

        if (g_ball_position.y > MAX_Y)
        {
            g_ball_movement.y = -g_ball_movement.y;
        }

        if (g_ball_position.y < MIN_Y)
        {
            g_ball_movement.y = -g_ball_movement.y;
        }
    }
    // —————————————————————————————————————————————————————————//
    if ((g_player_position.y > MAX_Y - 0.6) or (g_player_position.y < MIN_Y + 0.6))
    {
        g_player_position -= g_player_movement * g_player_speed * delta_time;
    }

    if ((g_player_position_2.y > MAX_Y - 0.6) or (g_player_position_2.y < MIN_Y + 0.6))
    {
        g_player_position_2 -= g_player_movement_2 * g_player_speed * delta_time;
        cpu_move = -cpu_move;
    }
}


void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_model_matrix, g_player_texture_id);
    draw_object(g_model_matrix_2, g_player_texture_id_2);
    draw_object(g_ball_matrix, g_ball_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}


