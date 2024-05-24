#include "..\inc\Controls.h"

bool held_keys[256] = { false };

inline bool key_is_pressed (int win_key) {
    if(win_key > 0 && win_key < 255) {
        bool pressed = (GetAsyncKeyState(win_key) & 0x8000) != 0;
        
        if (pressed) {
            held_keys[win_key] = true;
        } else if(held_keys[win_key]) {
            held_keys[win_key] = false;
        }

        return pressed;
    }
    return false;
}

inline bool key_is_held (int win_key) {
    return held_keys[win_key];
}

inline bool key_is_newly_pressed (int win_key) {
    bool held = key_is_held(win_key);
    bool pressed = key_is_pressed(win_key);
    return pressed && !held;
}

inline bool key_is_whitelisted (int win_key) {
    return win_key != 16 && win_key != 17;
}

char detect_commands (void) {
    // Check if a specific key is pressed
    if(key_is_pressed(VK_CONTROL)) {
        if (key_is_newly_pressed('S')) {
            return KBD_CTRL_S;
        }
        else if (key_is_newly_pressed('R')) {
            return KBD_CTRL_R;
        }
        else if (key_is_newly_pressed('T')) {
            return KBD_CTRL_T;
        }
        else if (key_is_newly_pressed('Q')) {
            return KBD_CTRL_Q;
        }
    }
    return 0;
}

char vk_to_char (int key) {
    char ret = '\0';
    switch(key) {
        case KBD_RETURN:
            ret = '\n';
            break;
        case KBD_BACKSPACE:
            ret = '\b';
            break;
        case KBD_DELETE:
            ret = 127;
            break;
        case KBD_SHIFT:
            ret = '\0';
            break;
        default:
            ret = key;
    }
    return ret;
}

// Return first command or valid keyboard press
char keyboard_listener (void) {
    while (1) {
        // Handle commands
        char comm = detect_commands();
        if(comm != 0) {
            return comm;
        }
        
        // Handle regular keys
        for (int key = 8; key < 256; ++key) {
            if (key_is_newly_pressed(key) && key_is_whitelisted(key)) {
                return vk_to_char(key);
            }
        }
        Sleep(20);
    }
}