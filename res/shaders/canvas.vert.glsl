#version 430 core

layout (location = 0) in vec2 aPos;

out vec2 FragPos;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    FragPos = aPos;
}
