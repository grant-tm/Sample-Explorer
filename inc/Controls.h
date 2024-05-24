#ifndef CONTROLS_H
#define CONTROLS_H

#include <windows.h>
#include "SystemUtilities.h"

#define KBD_CTRL_S 19
#define KBD_CTRL_R 18
#define KBD_CTRL_T 20
#define KBD_CTRL_Q 17

#define KBD_RETURN 10
#define KBD_BACKSPACE 8
#define KBD_DELETE 127
#define KBD_SHIFT 16

inline bool is_held (int win_key);
inline bool is_pressed (int win_key);
inline bool is_whitelisted(int win_key);

char vk_to_char (int key);

char detect_commands (void);
char keyboard_listener (void);

#endif // CONTROLS_H