#include "bitbox.h"
#include "font.h"
#include "scene.h"

game_t game CCM_MEMORY;

void game_init()
{   // Logic run once when setting up the Bitbox; Bitbox will call this, don't do it yourself.
#ifdef EMULATOR 
    // add tests here
#endif

    game.mode = ModeNone;
    game.previous_mode = ModeNone;

    font_init();

    game_switch(ModeScene);
}

void game_frame()
{   // Logic to run every frame; Bitbox will call this, don't do it yourself.
    game.input.new_buttons[0] = gamepad_buttons[0] & (~game.input.old_buttons[0]);
    game.input.new_buttons[1] = gamepad_buttons[1] & (~game.input.old_buttons[1]);

    switch (game.mode)
    {   // Run frame logic depending on game mode.
        case ModeScene:
            scene_update();
            break;
        default:
            break;
    }
    
    game.input.old_buttons[0] = gamepad_buttons[0];
    game.input.old_buttons[1] = gamepad_buttons[1];
    
    if (game.input.press_timeout[0])
        --game.input.press_timeout[0];
    if (game.input.press_timeout[1])
        --game.input.press_timeout[1];
    
    if (game.message_timeout && --game.message_timeout == 0)
        game.message[0] = 0; 
}

static void bsod_line();

void graph_line()
{   // Logic to draw for each line on the VGA display; Bitbox will call this, don't do it yourself.

    // TODO: consider doing some non-drawing work on odd frames:
    if (vga_odd)
        return;

    if (game.message[0])
    {   // Drawing the game message takes priority, nothing else can show up where that message goes:
        unsigned int delta_y = vga_line - 220;
        if (delta_y < 10)
        {   memset(draw_buffer, 0, 2*SCREEN_WIDTH);
            --delta_y;
            if (delta_y < 8)
            {   font_render_line_doubled(game.message, 36, (int)delta_y, 65535, 0);
            }
            // Don't allow any other drawing in this region...
            return;
        }
    }

    switch (game.mode)
    {   // Run drawing logic depending on game mode.
        case ModeScene:
            scene_line();
            break;
        default:
            bsod_line();
            break;
    }
}

void game_switch(game_mode_t new_game_mode)
{   // Switches to a new game mode.  Does nothing if already in that mode.
    if (new_game_mode == game.mode)
        return;
    // don't let things trigger again if possible
    game.input.press_timeout[0] = GAMEPAD_PRESS_TIMEOUT;
    game.input.press_timeout[1] = GAMEPAD_PRESS_TIMEOUT;

    game.previous_mode = game.mode;
    game.mode = new_game_mode;

    switch (game.mode)
    {   // Run start-up logic depending on game mode.
        case ModeScene:
            scene_restart();
            break;
        default:
            break;
    }
}

void game_set_message_with_timeout(const char *msg, unsigned int timeout)
{   // Sets a game message where everyone can see it, with a timeout.
    // use timeout=0 for a permanent message (at least until changed).
    // You may pass in a msg of NULL if you've already manipulated game.message directly.
    if (msg)
        strncpy((char *)game.message, msg, 31);

    game.message[31] = 0;
    game.message_timeout = timeout;
    message("[in-game message: \"%s\" (expiring %d)]\n", game.message, timeout);
}

#define BSOD_COLOR8 140

static void bsod_line()
{   // Draw a line for the blue-screen-of-death.
    int line = vga_line/10;
    int internal_line = vga_line%10;
    if (vga_line/2 == 0 || (internal_line/2 == 4))
    {   // Make the beautiful blue background we deserve:
        memset(draw_buffer, BSOD_COLOR8, 2*SCREEN_WIDTH);
        return;
    }
    line -= 4;
    if (line < 0 || line >= 16)
        return;
    uint32_t *dst = (uint32_t *)draw_buffer + 37;
    // TODO: just shift
    uint32_t color_choice[2] = { (BSOD_COLOR8*257)|((BSOD_COLOR8*257)<<16), 65535|(65535<<16) };
    int shift = ((internal_line/2))*4;
    for (int c=0; c<16; ++c)
    {   // Draw all the letters in the font for fun
        uint8_t row = (font[c+line*16] >> shift) & 15;
        for (int j=0; j<4; ++j)
        {   // Draw each pixel in the letter
            *(++dst) = color_choice[row&1];
            row >>= 1;
        }
        *(++dst) = color_choice[0];
    }
}
