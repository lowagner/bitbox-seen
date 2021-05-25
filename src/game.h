#ifndef GAME_H
#define GAME_H
#include "bitbox.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#ifdef EMULATOR
#define EMU_ONLY(x) x
#else
#define EMU_ONLY(x) {}
#endif

#define ASSERT(x) EMU_ONLY({if (!(x)) \
{   message(#x " was not true at " __FILE__ ":%d!\n", __LINE__); \
    bitbox_die(-1, 0); \
}})

#define STATIC_ASSERT(x) EMU_ONLY(static_assert(x))

#define CLOSE(x, y) (fabs((x) - (y)) < 1e-3)

#define GAMEPAD_PRESS_TIMEOUT 8
#define GAMEPAD_PRESS(id, key) (game.input.new_buttons[id] & (gamepad_##key))
#define GAMEPAD_PRESSING(id, key) \\
(   (gamepad_buttons[id] & (gamepad_##key) & \\
    (~game.input.old_buttons[id]) | ((game.input.press_timeout[id] == 0)*gamepad_##key)) \\
)
#define GAMEPAD_HOLDING(id, key) (gamepad_buttons[id] & (gamepad_##key))

typedef enum
{   ModeNone=0,
    // TODO: add skippable intro
    ModeScene,
}   game_mode_t;

typedef struct game
{   
    // populated using a game_mode_t type:
    uint8_t mode;
    uint8_t previous_mode;

    uint8_t message[32];
    unsigned int message_timeout;

    struct
    {   // gamepad buttons that are on now but weren't on previously:
        uint16_t new_buttons[2];
        // gamepad buttons that were active last frame:
        uint16_t old_buttons[2];
        // gamepad wait time when pressing before retriggering another key-press:
        uint16_t press_timeout[2];
    }   input;

}   game_t;

void game_switch(game_mode_t new_game_mode);

void game_set_message_with_timeout(const char *msg, unsigned int timeout);

#define MESSAGE_TIMEOUT (10*64)

#define LL_ITERATE(container, next_name, index, starting_after, fn) \
{   uint8_t index = container[starting_after].next_name; \
    while (index) { \
        fn; \
        index = container[index].next_name; \
    }   \
}

#define LL_RESET(container, next_name, previous_name, MAX_COUNT) \
    container[0].next_name = 0; \
    container[0].previous_name = 0; \
    { \
    int i; \
    for (i = 0; i < MAX_COUNT - 1; ++i) \
    {   container[i].next_free = i + 1; \
    } \
    container[i].next_free = 0; \
    }

#define LL_NEW(container, next_name, previous_name, MAX_COUNT) \
    uint8_t index = container[0].next_free; \
    if (!index) \
        return 0; \
    ASSERT(index < MAX_COUNT); \
    container##_t *new_element = &container[index]; \
    \
    /* update the free list: */ \
    container[0].next_free = new_element->next_free; \
    \
    /* do the insertion: */ \
    uint8_t next = container[0].next_name; \
    new_element->next_name = next; \
    new_element->previous_name = 0; \
    container[next].previous_name = index; \
    \
    return index;

#define LL_FREE(container, next_name, previous_name, index) \
{   uint8_t previous = container[index].previous_name; \
    uint8_t next = container[index].next_name; \
    container[previous].next_name = next; \
    container[next].previous_name = previous; \
    container[index].next_free = container[0].next_free; \
    container[0].next_free = index; \
}

#endif
