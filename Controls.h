#ifndef CONTROLS_H
#define CONTROLS_H

#include <windows.h>

inline bool is_pressed (char c);

char detect_commands (void);

char vk_to_char (int key);

char keyboard_listener (void);

#endif // CONTROLS_H