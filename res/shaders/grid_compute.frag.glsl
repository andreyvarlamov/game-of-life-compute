#version 430 core

layout (location = 0) in vec2 FragPos;

out vec4 FragColor;

uniform float canvas_w;
uniform float canvas_h;

uniform sampler2D computedGridTexture;

void main() {
     float cell_w = canvas_w / grid_w;
     float cell_h = canvas_h / grid_h;
     float x = float(int(FragPos.x / cell_w));
     float y = float(int(FragPos.y / cell_h));

     float sample = texture(computedGridTexture, vec2(x, y));

     if (sample > 0.5) {
          FragColor = vec4(0.8, 0.7, 0.7, 1.0);
     } else {
          FragColor = vec4(0.15, 0.13, 0.13, 1.0);
     }
}
