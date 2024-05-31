#ifndef UI_H
#define UI_H

#include "UIState.h"
#include <iostream>
#include <string>
#include <windows.h>

std::string format_string (std::string str, size_t length);

void render_ui (UIState *ui_state);

void print_search_results (std::vector<struct FileRecord> results, int n);

void ui_hide_cursor (void);

#endif // UI_H