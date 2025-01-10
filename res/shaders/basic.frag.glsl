#version 430 core

layout (location = 0) in vec2 FragPos;
out vec4 FragColor;

uniform int grid_w;
uniform int grid_h;
uniform int grid_state[400];

void main() {
     int x = int((FragPos.x + 1.0) * 0.5 * grid_w);
     int y = int((-FragPos.y + 1.0) * 0.5 * grid_h);
     int idx = y * grid_w + x;

     if (grid_state[idx] == 1) {
          FragColor = vec4(0.8, 0.7, 0.7, 1.0);
     } else {
          FragColor = vec4(0.15, 0.13, 0.13, 1.0);
     }
}
