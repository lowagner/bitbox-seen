#include "scene.h"

#include "game.h"

#include <math.h>

scene_t scene CCM_MEMORY;

#define SCENE_ASPECT_RATIO 1.2f
// minimum vga_line to start drawing on:
#define MIN_V NUM_SCENE_PLANES

void scene_restart()
{
}

void scene_update()
{
}

static inline void scene_calculate_plane(int plane_index);

void scene_line()
{   if (vga_line < NUM_SCENE_PLANES)
    {   if (vga_line / 2 == 0)
            memset(draw_buffer, 0, 2*SCREEN_WIDTH);
        scene_calculate_plane(vga_line);
        return;
    }
}

float scene_calculate_landscape_z(float *position);

static inline void scene_calculate_plane(int plane_index)
{   float forward_distance = scene.landscape.plane_distance[plane_index];
    ASSERT(forward_distance > 1.0f);
    float plane_center[2] =
    {   scene.camera.position[0] + scene.camera.forward[0] * forward_distance,
        scene.camera.position[1] + scene.camera.forward[1] * forward_distance,
        // ignore z direction for now.
    };
    float half_plane_width = forward_distance * SCENE_ASPECT_RATIO;
    float plane_pixel_width = half_plane_width / (SCREEN_WIDTH / 2);
    int i = -1;
    for (float camera_x = -half_plane_width; camera_x <= half_plane_width; camera_x += plane_pixel_width)
    {   float position[2] =
        {   plane_center[0] + scene.camera.right[0] * camera_x,
            plane_center[1] + scene.camera.right[1] * camera_x,
            // again, ignore z
        };
        float landscape_z = scene_calculate_landscape_z(position);
        // do perspective:
        float delta_z = (scene.camera.position[2] - landscape_z) / forward_distance;
        // flip to screen coordinates and center based on camera's relative image location:
        float screen_v = scene.camera.screen_v - delta_z;
        uint8_t screen_v8;
        if (screen_v < MIN_V)
            screen_v8 = 0;
        else if (screen_v >= SCREEN_HEIGHT - MIN_V)
        {   STATIC_ASSERT(SCREEN_HEIGHT <= 255);
            screen_v8 = SCREEN_HEIGHT;
        }
        else
            screen_v8 = roundf(screen_v);
        scene.landscape.to_draw.plane_v_start[++i][plane_index] = screen_v8;        
    }
}

float scene_calculate_landscape_z(float *position)
{   // position is a float[2] being passed in.
    float distance2 = position[0] * position[0] + position[1] * position[1];
    return (position[0] - position[1] * sinf(position[0] * 0.1f)) * expf(-0.0001f * distance2);
}
