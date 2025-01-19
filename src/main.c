#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cglm/cglm.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define VERT_SHADER "res/shaders/canvas.vert.glsl"
#define FRAG_SHADER "res/shaders/grid.frag.glsl"
#define GOL_COMPUTE_SHADER "res/shaders/game_of_life.comp.glsl"

enum {
    SCREEN_WIDTH = 800,
    SCREEN_HEIGHT = 600,

    MAX_VERT = 4096,
    MAX_IDX = 16384,

    ONE_MB = 1024 * 1024,

    CANVAS_WIDTH = 200,
    CANVAS_HEIGHT = CANVAS_WIDTH,
    GRID_WIDTH = 20,
    GRID_HEIGHT = GRID_WIDTH
};

typedef struct {
    int w, h;
    GLFWwindow *glfw_window;
} Window_State;

typedef struct {
    uint32_t vbo;
    uint32_t ebo;
    uint32_t vao;
    uint32_t render_shader;
    uint32_t gol_compute_shader;
} Gl_State;

static Window_State g_window_state;
static Gl_State g_gl_state;
static char g_gl_error_buffer[ONE_MB];

static uint32_t g_compute_input_tex;
static uint32_t g_compute_output_tex;

void exit_with_error(const char *msg, ...);
void trace_log(const char *msg, ...);
void *xmalloc(size_t size);

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void window_size_callback(GLFWwindow *window, int width, int height);

void set_window_size(int width, int height);
void set_ortho_projection(int width, int height);

Gl_State initialize_gl_state();
uint32_t build_shader_from_file(const char *file_path, GLenum shader_type);
uint32_t link_vert_frag_shaders(uint32_t vert, uint32_t frag);
uint32_t link_comp_shader(uint32_t comp);
uint32_t build_shaders(const char *vert_file, const char *frag_file);
uint32_t build_compute_shader(const char *file_path);
void sub_vertex_data(float screen_width, float screen_height);

void create_compute_textures(int grid_w, int grid_h);
void run_gol_compute_procedure(int grid_w, int grid_h);

int main() {
    if (!glfwInit()) {
        exit_with_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_window_state.glfw_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Game of Life", NULL, NULL);
    if (g_window_state.glfw_window == NULL) {
        exit_with_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(g_window_state.glfw_window);

    glfwSetKeyCallback(g_window_state.glfw_window, keyboard_callback);
    glfwSetWindowSizeCallback(g_window_state.glfw_window, window_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        exit_with_error("Fail to load GL function pointers");
    }

    trace_log("Loaded OpenGL function pointers. Debug info:");
    trace_log("  Version:  %s", glGetString(GL_VERSION));
    trace_log("  GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    trace_log("  Vendor:   %s", glGetString(GL_VENDOR));
    trace_log("  Renderer: %s", glGetString(GL_RENDERER));

    g_gl_state = initialize_gl_state();

    set_window_size(SCREEN_WIDTH, SCREEN_HEIGHT);

    glClearColor(0.09f, 0.07f, 0.07f, 1.0f);

    sub_vertex_data((float)CANVAS_WIDTH, (float)CANVAS_HEIGHT);

    glUseProgram(g_gl_state.render_shader);
    glUniform1f(glGetUniformLocation(g_gl_state.render_shader, "canvas_w"), (float)CANVAS_WIDTH);
    glUniform1f(glGetUniformLocation(g_gl_state.render_shader, "canvas_h"), (float)CANVAS_HEIGHT);
    glUseProgram(0);

    create_compute_textures(GRID_WIDTH, GRID_HEIGHT);

    glUseProgram(g_gl_state.gol_compute_shader);
    glUniform2i(glGetUniformLocation(g_gl_state.gol_compute_shader, "grid_size"), GRID_WIDTH, GRID_HEIGHT);
    glUseProgram(0);

    /* run_gol_compute_procedure(); */

    trace_log("Entering main loop...");

    while (!glfwWindowShouldClose(g_window_state.glfw_window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // draw_grid();
        glUseProgram(g_gl_state.render_shader);
        glBindVertexArray(g_gl_state.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(g_window_state.glfw_window);
        glfwPollEvents();
    }

    trace_log("Exiting gracefully...");

    glfwDestroyWindow(g_window_state.glfw_window);
    glfwTerminate();
    return 0;
}

void exit_with_error(const char *msg, ...) {
    fprintf(stderr, "FATAL: ");
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void trace_log(const char *msg, ...) {
    printf("INFO: ");
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("\n");
}

void *xmalloc(size_t size) {
    void *d = malloc(size);
    if (!d) {
        exit_with_error("Failed to malloc");
    }
    return d;
}

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window; (void)key; (void)scancode; (void)action; (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        trace_log("Received ESC. Terminating...");
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        run_gol_compute_procedure(GRID_WIDTH, GRID_HEIGHT);
    } else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        // TODO
    }
}

void window_size_callback(GLFWwindow *window, int width, int height) {
    (void)window; (void)width; (void)height;

    set_window_size(width, height);
}

void set_window_size(int width, int height) {
    g_window_state.w = width;
    g_window_state.h = height;

    glViewport(0, 0, width, height);
    set_ortho_projection(width, height);
}

void set_ortho_projection(int width, int height) {
    mat4 projection;
    glm_ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, projection);

    glUseProgram(g_gl_state.render_shader);
    glUniformMatrix4fv(glGetUniformLocation(g_gl_state.render_shader, "projection"), 1, GL_FALSE, (float *)projection);
    glUseProgram(0);
}

Gl_State initialize_gl_state() {
    Gl_State result = {0};

    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);

    size_t total_size = MAX_VERT * (2) * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_IDX * sizeof(uint32_t), NULL, GL_STREAM_DRAW);

    // Positions -- vec2
    size_t stride = 2 * sizeof(float);
    size_t offset = 0;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)offset);
    glEnableVertexAttribArray(0);

    offset += stride * MAX_VERT;
    assert(offset == total_size);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    result.render_shader = build_shaders(VERT_SHADER, FRAG_SHADER);
    result.gol_compute_shader = build_compute_shader(GOL_COMPUTE_SHADER);

    return result;
}

uint32_t build_shader_from_file(const char *file_path, GLenum shader_type) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        exit_with_error("Failed to open shader file");
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *shader_src = xmalloc(file_size + 1);
    fread(shader_src, file_size, 1, file);
    fclose(file);
    shader_src[file_size] = '\0';

    uint32_t shader_id = glCreateShader(shader_type);
    glShaderSource(shader_id, 1, (const char **)&shader_src, NULL);
    glCompileShader(shader_id);

    int success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader_id, ONE_MB, NULL, g_gl_error_buffer);
        exit_with_error("Failed to compile shader (type 0x%04X). Error:\n  %s\nSource:\n%s\n",
                        shader_type, g_gl_error_buffer, shader_src);
    }

    free(shader_src);

    return shader_id;
}

uint32_t link_vert_frag_shaders(uint32_t vert, uint32_t frag) {
    uint32_t program_id = glCreateProgram();
    glAttachShader(program_id, vert);
    glAttachShader(program_id, frag);
    glLinkProgram(program_id);

    int success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program_id, ONE_MB, NULL, g_gl_error_buffer);
        exit_with_error("Failed to link vert-frag shader program. Error:\n  %s", g_gl_error_buffer);
    }

    return program_id;
}

uint32_t link_comp_shader(uint32_t comp) {
    uint32_t program_id = glCreateProgram();
    glAttachShader(program_id, comp);
    glLinkProgram(program_id);

    int success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program_id, ONE_MB, NULL, g_gl_error_buffer);
        exit_with_error("Failed to link comp shader program. Error:\n  %s", g_gl_error_buffer);
    }

    return program_id;
}

uint32_t build_shaders(const char *vert_file, const char *frag_file) {
    uint32_t vert_shader = build_shader_from_file(vert_file, GL_VERTEX_SHADER);
    uint32_t frag_shader = build_shader_from_file(frag_file, GL_FRAGMENT_SHADER);
    uint32_t shader_program = link_vert_frag_shaders(vert_shader, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return shader_program;
}

uint32_t build_compute_shader(const char *file_path) {
    uint32_t comp_shader = build_shader_from_file(file_path, GL_COMPUTE_SHADER);
    uint32_t program = link_comp_shader(comp_shader);
    glDeleteShader(comp_shader);
    return program;
}

void sub_vertex_data(float screen_width, float screen_height) {
    glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.vbo);

    size_t total_size = MAX_VERT * (2) * sizeof(float);
    size_t vert_to_sub_count = 4;
    size_t idx_to_sub_count = 6;
    assert(vert_to_sub_count < MAX_VERT);
    assert(idx_to_sub_count < MAX_IDX);
    size_t stride, offset;

    // Positions -- vex2
    offset = 0;
    stride = 2 * sizeof(float);
    float positions[] = {
        0.0f, 0.0f,
        0.0f, screen_height,
        screen_width, 0.0f,
        screen_width, screen_height,
    };
    assert(sizeof(positions) == stride * vert_to_sub_count);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(positions), positions);

    offset += stride * MAX_VERT;
    assert(offset == total_size);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl_state.ebo);

    uint32_t indices[] = {
        0, 1, 2,
        2, 1, 3
    };
    assert(sizeof(indices) / sizeof(indices[0]) == idx_to_sub_count);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void create_compute_textures(int grid_w, int grid_h) {
    uint32_t grid_state_texture[2];
    glGenTextures(2, grid_state_texture);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, grid_state_texture[i]);

        if (i == 0) {
            uint8_t *r8grid = malloc(grid_w * grid_h);
            for (int j = 0; j < grid_w * grid_h; j++) {
                r8grid[j] = (rand() % 10 == 0) * 255;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, grid_w, grid_h, 0, GL_RED, GL_UNSIGNED_BYTE, r8grid);
            free(r8grid);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, grid_w, grid_h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    g_compute_input_tex = grid_state_texture[0];
    g_compute_output_tex = grid_state_texture[1];

    glUseProgram(g_gl_state.render_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_compute_input_tex);
    glUniform1i(glGetUniformLocation(g_gl_state.render_shader, "computed_grid_texture"), 0);
    glUseProgram(0);
}

void run_gol_compute_procedure(int grid_w, int grid_h) {
    glUseProgram(g_gl_state.gol_compute_shader);
    glBindImageTexture(0, g_compute_input_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    glBindImageTexture(1, g_compute_output_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

    int work_groups_x = (grid_w + 15) / 16;
    int work_groups_y = (grid_h + 15) / 16;
    glDispatchCompute(work_groups_x, work_groups_y, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    uint32_t temp = g_compute_input_tex;
    g_compute_input_tex = g_compute_output_tex;
    g_compute_output_tex = temp;

    glUseProgram(g_gl_state.render_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_compute_input_tex);
    glUniform1i(glGetUniformLocation(g_gl_state.render_shader, "computed_grid_texture"), 0);
    glUseProgram(0);
}
