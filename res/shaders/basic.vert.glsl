#version 430 core

layout (location = 0) in vec2 aPos;

uniform mat4 projection;

out vec2 FragPos;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    FragPos = gl_Position.xy;
}
