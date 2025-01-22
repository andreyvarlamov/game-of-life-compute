#version 430 core

layout(location = 0) in vec2 FragPos;

out vec4 FragColor;

uniform float canvas_w;
uniform float canvas_h;
uniform int grid_w;
uniform int grid_h;

layout(binding = 0) uniform sampler2D computed_grid_texture;

void main() {
     float x = FragPos.x / canvas_w;
     float y = FragPos.y / canvas_h;

     float s = texture(computed_grid_texture, vec2(x, y)).r;

     if (s > 0.5) {
          FragColor = vec4(0.8, 0.7, 0.7, 1.0);
     } else {
          FragColor = vec4(0.15, 0.13, 0.13, 1.0);
     }
}
