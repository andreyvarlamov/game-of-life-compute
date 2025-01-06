#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum {
    SCREEN_WIDTH = 800,
    SCREEN_HEIGHT = 600
};

void exit_with_error(const char *msg, ...);
void trace_log(const char *msg, ...);

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

int main() {
    if (!glfwInit()) {
        exit_with_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Game of Life", NULL, NULL);
    if (window == NULL) {
        exit_with_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyboard_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        exit_with_error("Fail to load GL function pointers");
    }

    trace_log("Loaded OpenGL function pointers. Debug info:");
    trace_log("  Version:  %s", glGetString(GL_VERSION));
    trace_log("  GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    trace_log("  Vendor:   %s", glGetString(GL_VENDOR));
    trace_log("  Renderer: %s", glGetString(GL_RENDERER));

    trace_log("Entering main loop...");
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // glfwSwapBuffers(window);
    }

    trace_log("Exiting gracefully...");

    glfwDestroyWindow(window);
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

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window; (void)key; (void)scancode; (void)action; (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        trace_log("Received ESC. Terminating...");
        glfwSetWindowShouldClose(window, true);
    }
}
