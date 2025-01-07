#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <cglm/cglm.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define VERT_SHADER "res/shaders/basic.vert.glsl"
#define FRAG_SHADER "res/shaders/basic.frag.glsl"

enum {
    SCREEN_WIDTH = 800,
    SCREEN_HEIGHT = 600,

    MAX_VERT = 4096,
    MAX_IDX = 16384,

    ONE_MB = 1024 * 1024
};

typedef struct {
    int w, h;
    GLFWwindow *glfw_window;
} Window_State;

typedef struct {
    uint32_t vbo;
    uint32_t ebo;
    uint32_t vao;
    uint32_t shader;
} Gl_State;

static Window_State g_window_state;
static Gl_State g_gl_state;
static char g_gl_error_buffer[ONE_MB];

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
uint32_t build_shaders(const char *vert_file, const char *frag_file);
void sub_vertex_data();

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

    sub_vertex_data();

    trace_log("Entering main loop...");

    while (!glfwWindowShouldClose(g_window_state.glfw_window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(g_gl_state.shader);

        glBindVertexArray(g_gl_state.vao);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
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

    glUseProgram(g_gl_state.shader);
    glUniformMatrix4fv(glGetUniformLocation(g_gl_state.shader, "projection"), 1, GL_FALSE, (float *)projection);
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

    result.shader = build_shaders(VERT_SHADER, FRAG_SHADER);

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

    char *shader_src = xmalloc(file_size);
    fread(shader_src, file_size, 1, file);
    fclose(file);

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
        exit_with_error("Failed to link shader program. Error:\n  %s", g_gl_error_buffer);
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

void sub_vertex_data() {
    glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.vbo);

    size_t total_size = MAX_VERT * (2) * sizeof(float);
    size_t vert_to_sub_count = 3;
    size_t idx_to_sub_count = 3;
    assert(vert_to_sub_count < MAX_VERT);
    assert(idx_to_sub_count < MAX_IDX);
    size_t stride, offset;

    // Positions -- vex2
    offset = 0;
    stride = 2 * sizeof(float);
    float positions[] = {
        200.0f, 450.0f,
        600.0f, 450.0f,
        400.0f, 150.0f
    };
    assert(sizeof(positions) == stride * vert_to_sub_count);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(positions), positions);

    offset += stride * MAX_VERT;
    assert(offset == total_size);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl_state.ebo);

    uint32_t indices[] = {
        0, 1, 2
    };
    assert(sizeof(indices) / sizeof(indices[0]) == idx_to_sub_count);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
