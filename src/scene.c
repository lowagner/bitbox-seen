#include "scene.h"

#include "game.h"

#include <math.h>

scene_t scene CCM_MEMORY;

#define CAMERA_HEIGHT_OFF_GROUND 5.0f
#define SCENE_ASPECT_RATIO 1.2f
// minimum vga_line to start drawing on:
#define MIN_V (2 * NUM_SCENE_PLANES /* calculating each plane */ + 2 /* blank lines at top*/)

#define VECTOR_EXPAND(vector) \
    (vector)[0], (vector)[1], (vector)[2]

#define VECTOR_ASSIGN(vector, x, y, z) \
{   (vector)[0] = (x); \
    (vector)[1] = (y); \
    (vector)[2] = (z); \
}

#define SQUARE(x) ((x) * (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define VECTOR_FOLLOW(update_me, with_this, delta) \
{   (update_me)[0] += delta * ((with_this)[0] - (update_me)[0]); \
    (update_me)[1] += delta * ((with_this)[1] - (update_me)[1]); \
    (update_me)[2] += delta * ((with_this)[2] - (update_me)[2]); \
}

#define VECTOR_FOLLOW_WITH_OFFSET(update_me, with_this, offset0, offset1, offset2, delta) \
{   (update_me)[0] += delta * ((with_this)[0] + offset0 - (update_me)[0]); \
    (update_me)[1] += delta * ((with_this)[1] + offset1 - (update_me)[1]); \
    (update_me)[2] += delta * ((with_this)[2] + offset2 - (update_me)[2]); \
}

#define VECTOR2_NORMALIZE(vector) \
{   float norm = sqrtf(SQUARE((vector)[0]) + SQUARE((vector)[1])); \
    ASSERT(norm > 1E-5); \
    (vector)[0] /= norm; \
    (vector)[1] /= norm; \
}

#define VECTOR2_ADD_WITH_MULTIPLIER(vector, add, multiplier) \
{   (vector)[0] += (add)[0] * (multiplier); \
    (vector)[1] += (add)[1] * (multiplier); \
}

#define VECTOR2S_SPIN_WITH_DELTA(forward, right, delta) \
{   VECTOR2_ADD_WITH_MULTIPLIER(forward, right, delta); \
    VECTOR2_NORMALIZE(forward); \
    /* cross forward with z-axis to get new right 
       x    y    z
       f[0] f[1] 0   --> x (f[1]) - y (f[0])
       0    0    1 
    */ \
    (right)[0] = (forward)[1]; \
    (right)[1] = -((forward)[0]); \
}

float scene_calculate_landscape_z(float *position);

void scene_restart()
{   int plane = -1;
    scene.landscape.plane_distance[++plane] = 1.5f;
    scene.landscape.color[plane] = RGB(100, 255, 100);
    scene.landscape.plane_distance[++plane] = 5.0f;
    scene.landscape.color[plane] = RGB(200, 255, 100);
    scene.landscape.plane_distance[++plane] = 15.0f;
    scene.landscape.color[plane] = RGB(100, 50, 50);
    scene.landscape.plane_distance[++plane] = 100.0f;
    scene.landscape.color[plane] = RGB(50, 10, 70);
    ASSERT(plane == NUM_SCENE_PLANES - 1)

    // set up camera correctly:
    VECTOR_ASSIGN(scene.camera.position, 0, 0, 0);
    scene.camera.position[2] =
            scene_calculate_landscape_z(scene.camera.position) + CAMERA_HEIGHT_OFF_GROUND;
    VECTOR_ASSIGN(scene.camera.forward, 0, 1, 0);
    VECTOR_ASSIGN(scene.camera.right, 1, 0, 0);
    scene.camera.screen_v = SCREEN_HEIGHT / 2;

    // set up player:
    VECTOR_ASSIGN(scene.player.position, 0, 1, scene.camera.position[2]);
    VECTOR_ASSIGN(scene.player.forward, 0, 1, 0);
    VECTOR_ASSIGN(scene.player.right, 1, 0, 0);
    VECTOR_ASSIGN(scene.player.velocity, 0, 0, 0);
}

void scene_update()
{   float delta = 0.f;
    if (GAMEPAD_HOLDING(0, up))
        delta += 0.1f;
    if (GAMEPAD_HOLDING(0, down))
        delta -= 0.1f;
    if (delta)
    {   VECTOR2_ADD_WITH_MULTIPLIER
        (   scene.player.position, scene.player.forward, delta
        );
        scene.player.position[2] = scene_calculate_landscape_z(scene.player.position);
    }
    delta = 0.f;
    if (GAMEPAD_HOLDING(0, left))
        delta -= 0.05f;
    if (GAMEPAD_HOLDING(0, right))
        delta += 0.05f;
    if (delta)
    {   VECTOR2S_SPIN_WITH_DELTA
        (   scene.player.forward, scene.player.right, delta
        );
    }
    VECTOR_FOLLOW(scene.camera.forward, scene.player.forward, 0.1f);
    VECTOR_FOLLOW(scene.camera.right, scene.player.right, 0.1f);
    VECTOR_FOLLOW_WITH_OFFSET
    (   scene.camera.position, scene.player.position,
        -scene.camera.forward[0],
        -scene.camera.forward[1],
        -scene.camera.forward[2] + CAMERA_HEIGHT_OFF_GROUND,
        0.1f
    );
    if (vga_frame % 64 == 0)
    {   // TODO: why is camera.z negative going up mountains?? did i flip the scene??
        message
        (   "camera position(%f, %f, %f):\n  forward(%f, %f, %f) and right(%f, %f, %f)\n",
            VECTOR_EXPAND(scene.camera.position),
            VECTOR_EXPAND(scene.camera.forward),
            VECTOR_EXPAND(scene.camera.right)
        );
    }
    // TODO: place the player in the camera's sights, adjust camera.screen_v as necessary
}

static inline uint16_t scene_skybox_color(int i);
static inline void scene_calculate_skybox();
static inline void scene_calculate_plane(int plane_index);

void scene_line()
{   if (vga_line < MIN_V)
    {   if (vga_line / 2 == 0)
        {   memset(draw_buffer, 0, 2*SCREEN_WIDTH);
            // TODO: calculate skybox on vga_line == 0
            if (vga_line % 2 == 0)
                scene_calculate_skybox();
        }
        else if (vga_line % 2 == 0)
            scene_calculate_plane(vga_line/2 - 1);
        return;
    }
    else if (vga_line >= SCREEN_HEIGHT - MIN_V)
    {   if ((vga_line - (SCREEN_HEIGHT - MIN_V)) / 2 == 0)
        {   memset(draw_buffer, 0, 2*SCREEN_WIDTH);
        }
        return;
    }
    uint16_t *dst = draw_buffer - 1;
    const uint8_t vga8 = vga_line;
    for (int i = 0; i < SCREEN_WIDTH; ++i)
    {   STATIC_ASSERT(NUM_SCENE_PLANES == 4);
        uint8_t *plane_v_start = scene.landscape.to_draw.plane_v_start[i];
        if (plane_v_start[0] > vga8)
        {   if (plane_v_start[1] > vga8)
            {   if (plane_v_start[2] > vga8)
                {   if (plane_v_start[3] > vga8)
                        *++dst = scene_skybox_color(i);
                    else
                        *++dst = scene.landscape.color[3];
                }
                else
                    *++dst = scene.landscape.color[2];
            }
            else
                *++dst = scene.landscape.color[1];
        }
        else
            *++dst = scene.landscape.color[0];
    }
    // Draw the player if they're on this line:
    if (vga8 >= scene.player.v_top && vga8 < scene.player.v_bottom)
    {   dst = draw_buffer + MAX(0, scene.player.u_left);
        uint16_t *const max_dst = draw_buffer + MIN(scene.player.u_right, SCREEN_WIDTH);
        while (dst < max_dst)
            *dst++ = RGB(255, 0, 0);
    }
}

static inline uint16_t scene_skybox_color(int i)
{   // TODO: more involved skybox:
    return scene.landscape.to_draw.skybox.sky_color;
}

static inline void scene_calculate_skybox()
{   scene.landscape.to_draw.skybox.sky_color = RGB(150, 190, 230);
}

static inline void scene_calculate_plane(int plane_index)
{   ASSERT(plane_index >= 0 && plane_index < NUM_SCENE_PLANES);
    float forward_distance = scene.landscape.plane_distance[plane_index];
    ASSERT(forward_distance > 1.0f);
    float plane_center[2] =
    {   scene.camera.position[0] + scene.camera.forward[0] * forward_distance,
        scene.camera.position[1] + scene.camera.forward[1] * forward_distance,
        // ignore z direction for now.
    };
    float half_plane_width = forward_distance * SCENE_ASPECT_RATIO;
    float plane_pixel_width = half_plane_width / (SCREEN_WIDTH / 2);
    int screen_u = -1;
    for (float camera_x = -half_plane_width; camera_x <= half_plane_width; camera_x += plane_pixel_width)
    {   float position[3] =
        {   plane_center[0] + scene.camera.right[0] * camera_x,
            plane_center[1] + scene.camera.right[1] * camera_x,
            0,
        };
        position[2] = scene_calculate_landscape_z(position);
        // do perspective:
        float delta_z = (scene.camera.position[2] - position[2]) / forward_distance;
        // flip to screen coordinates and center based on camera's relative image location:
        float landscape_v = scene.camera.screen_v - delta_z;
        uint8_t landscape_v8;
        if (landscape_v < MIN_V)
            landscape_v8 = 0;
        else if (landscape_v >= SCREEN_HEIGHT - MIN_V)
        {   STATIC_ASSERT(SCREEN_HEIGHT <= 255);
            landscape_v8 = SCREEN_HEIGHT;
        }
        else
            landscape_v8 = roundf(landscape_v);
        scene.landscape.to_draw.plane_v_start[++screen_u][plane_index] = landscape_v8;
    }
}

float scene_calculate_landscape_z(float *position)
{   // the first two coordinates (x and y at position[0] and position[1])
    // are used to calculate the z-value or height of the landscape (return value)
    float distance2 = position[0] * position[0] + position[1] * position[1];
    return 50.f * expf(-0.0001f * distance2) *
    (   position[0] - position[1] * sinf(position[0] * 0.1f)
    );
}
