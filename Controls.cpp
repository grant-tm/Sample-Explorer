#include "Controls.h"

inline bool is_pressed (char c) {
    return GetAsyncKeyState(c) & 0x8000;
}

char detect_commands (void) {
    // Check if a specific key is pressed
    if(is_pressed(VK_CONTROL)) {
        if (is_pressed('S')) {
            return 19;
        }
        else if (is_pressed('R')) {
            return 18;
        }
        else if (is_pressed('T')) {
            return 20;
        }
        else if (is_pressed('Q')) {
            return 17;
        }
    }
    return 0;
}

char vk_to_char (int key) {
    if(key == 0x0D) {
        return '\n';
    }
    if(key == 0x08) {
        return '\b';
    }
    if(key == 0x2E) {
        return 127;
    }
    return key;
}

char keyboard_listener (void) {
    while (1) {
        char comm = detect_commands();
        if(comm != 0) {
            return comm;
        }
        for (int key = 8; key <= 255; ++key) {
            if (is_pressed(key) && key != 17) {
                return vk_to_char(key);
            }
        }
        Sleep(10);
    }
}