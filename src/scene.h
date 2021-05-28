#ifndef SCENE_H
#define SCENE_H

#include "game.h"

#include <stdint.h>

#define NUM_SCENE_PLANES 4

// world coordinates:
//  east (+x) and west (-x)
//  north (+y) and south (-y)
//  going up (+z) and going down (-z)
// screen coordinates:
//  u from left (0) to right (SCREEN_WIDTH-1)
//  v from top (0) to bottom (SCREEN_HEIGHT-1)

typedef struct landscape
{   // landscape is composed of 4 planes plus a skybox.

    // distances from the camera to each plane.
    float plane_distance[NUM_SCENE_PLANES];

    // in this root scope, we have helper variables which can
    // be modified every frame.

    struct
    {   // stuff that's happening in the sky
        // angle of tilt of the sun's axis, from the z-axis into the y-axis.
        //  0 means no tilt from z, sun goes around x-y plane
        //  +-64 means tilt to +-y axis, sun goes around x-z plane
        int8_t sun_alpha;
        // angle of the sun in the plane defined by the sun_alpha coordinate.
        // e.g. when sun_alpha = 0, then
        //  0 means sun in the east
        //  64 means sun in the north
        //  128 means sun in the west
        //  192 means sun in the south
        uint8_t sun_beta;
        // generally speaking, to get "normal" sun behavior
        // (rises in east, sets in west, angled to the south),
        // you want 64 > sun_alpha > 0, sun_beta deltas negative over time.

        // similar to sun.
        int8_t moon_alpha;
        uint8_t moon_beta;
    }   skybox;

    // colors for each plane, set by the scene_update, but can be modified afterwards for effect.
    uint16_t color[NUM_SCENE_PLANES];

    struct
    {   // these are derived variables which should not be changed by other files.

        // each plane is defined by the height from the top of the screen for each v coordinate;
        // these heights are indexed by position (u = 0 to SCREEN_WIDTH-1) and then plane index.
        // the first plane (plane_v_start[:][0]) is closest to the player, and therefore has
        // highest drawing priority.  it should be drawn if it its plane_v_start is
        // less-than-or-equal-to the current vga_line.  then try the next plane
        // (plane_v_start[:][1]), and so on.
        uint8_t plane_v_start[SCREEN_WIDTH][NUM_SCENE_PLANES];

        struct
        {   // what to draw when no plane intersects with the current screen position.
            int16_t sun_u;
            int16_t moon_u;

            uint8_t sun_v;
            uint8_t moon_v;
            uint16_t sky_color;

            uint16_t sun_color;
            uint16_t moon_color;
        }   skybox;
    }   to_draw;
}   landscape_t;

typedef struct scene
{
    // world randomizer.  probably should be set once
    // TODO: noise using this
    uint32_t seed;

    struct
    {   float position[3];
        float forward[3];
        float right[3];
        float velocity[3];
        uint8_t v_top;
        uint8_t v_bottom;
        int16_t u_left;
        int16_t u_right;
    }   player;

    struct
    {   float position[3];
        // unit vector corresponding to direction the camera is looking.
        // the z-component, forward[2], should be zero.
        float forward[3];
        // this right unit-vector for the camera:
        float right[3];
        // where the camera is in screen-height coordinates (0 to SCREEN_HEIGHT-1 probably):
        uint8_t screen_v;
    }   camera;

    landscape_t landscape;
}   scene_t;

extern scene_t scene;

void scene_restart();
void scene_update();
void scene_line();

#endif
