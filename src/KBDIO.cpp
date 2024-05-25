#include "..\inc\KBDIO.h"

void KBDIO::kbd_poll_inputs (void) {
    std::lock_guard<std::mutex> lock(mutex);
    for (int i=0; i<NUM_KEYS; ++i) {
        bool async_state = (GetAsyncKeyState(i) & 0x8000) != 0;
        if (async_state) {
            if (!held[i]) {
                pressed[i] = true;
                held[i] = true;
                hold_time[i] = 0;
            } else {
                pressed[i] = false;
                if (hold_time[i] < INT_MAX) {
                    hold_time[i]++;
                }
            }
        } else {
            if (held[i]) {
                pressed[i] = false;
                held[i] = false;
                hold_time[i] = 0;
            }
        }
    }
}

// Return first command or valid keyboard press
char KBDIO::kbd_listener (void) {
    while (1) {
        kbd_poll_inputs();
    }
}

// conversions
inline char KBDIO::kbd_code_to_char (int kbd_code) {
    return static_cast<char>(kbd_code);
}
inline int KBDIO::kbd_char_to_code (char c) {
    return static_cast<int>(c);
}

// is pressed
inline bool KBDIO::kbd_is_pressed (char c) {
    std::lock_guard<std::mutex> lock(mutex);
    return pressed[kbd_char_to_code(c)];
}
inline bool KBDIO::kbd_is_pressed (int kbd_code) {
    std::lock_guard<std::mutex> lock(mutex);
    return pressed[kbd_code];
}

// is held
inline bool KBDIO::kbd_is_held (char c) {
    std::lock_guard<std::mutex> lock(mutex);
    return held[kbd_char_to_code(c)];
}
inline bool KBDIO::kbd_is_held (int kbd_code) {
    std::lock_guard<std::mutex> lock(mutex);
    return held[kbd_code];
}

// is fresh
inline bool KBDIO::kbd_is_fresh (char c) {
    int kbd_code = kbd_char_to_code(c);
    return kbd_is_pressed(kbd_code) && !kbd_is_held(kbd_code);
}
inline bool KBDIO::kbd_is_fresh (int kbd_code) {
    return kbd_is_pressed(kbd_code) && !kbd_is_held(kbd_code);
}