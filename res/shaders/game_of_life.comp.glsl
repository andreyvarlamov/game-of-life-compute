#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, r8) uniform image2D input_grid;
layout(binding = 1, r8) uniform image2D output_grid;

uniform ivec2 grid_size;

void main() {
     ivec2 cell = ivec2(gl_GlobalInvocationID.xy);

     if (cell.x >= grid_size.x || cell.y >= grid_size.y) {
        return;
     }

     int alive_neighbors = 0;
     for (int dy = -1; dy <= 1; dy++) {
         for (int dx = -1; dx <= 1; dx++) {
             if (dx == 0 && dy == 0) continue;
             ivec2 neighbor = cell + ivec2(dx, dy);

             neighbor = (neighbor + grid_size) % grid_size;

             alive_neighbors += int(imageLoad(input_grid, neighbor).r > 0.5);
         }
     }

     int current_state = int(imageLoad(input_grid, cell).r > 0.5);
     int next_state = 0;
     if ((current_state == 1 && (alive_neighbors == 2 || alive_neighbors == 3)) ||
         (current_state == 0 && alive_neighbors == 3)) {
         next_state = 1;
     }

     imageStore(output_grid, cell, vec4(next_state, 0.0, 0.0, 1.0));
}
