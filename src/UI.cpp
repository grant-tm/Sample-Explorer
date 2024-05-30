#include "..\inc\UI.h"

#define UI_SEARCH_WIDTH 70
#define UI_RESULT_WIDTH 70
#define UI_TAG_WIDTH 70

std::string format_string (std::string str, size_t length) {
    if (str.length() <= length) {
        return str + std::string(length - str.length(), ' ');
    } else {
        return str.substr(0, length-3) + "...";
    }
}

void render_ui (UIState *ui_state) {

    std::string display = format_string(ui_state->search_buffer, UI_SEARCH_WIDTH);

    std::string result[5];
    for(size_t i=0; i<5; i++) {
        if(i >= ui_state->files.size()) {
            result[i] = format_string("", UI_RESULT_WIDTH);
        } else {
            int index = ui_state->file_scroll;
            struct ExplorerFile file = ui_state->files[index];
            result[i] = format_string(file.file_name, UI_RESULT_WIDTH
            );
        }
    }

    std::string ui_frame(1, ui_state->frame);
    std::string queued_inputs = std::to_string((int) ui_state->control_queue->size());
    
    std::string ui =     
    "======================================================================\n" \
    "UI_State: " + ui_frame + "\n" \
    "Queued Inputs: " + queued_inputs + "\n" \
    "======================================================================\n" \
    "Search Bar: " + display + "\n"\
    "----------------------------------------------------------------------\n" \
    "Search Results:\n" \
    "\t" + result[0] + "\n" \
    "\t" + result[1] + "\n" \
    "\t" + result[2] + "\n" \
    "\t" + result[3] + "\n" \
    "\t" + result[4] + "\n" \
    "======================================================================\n";
    
    std::cerr << ui << std::endl;
}

void print_search_results (std::vector<struct ExplorerFile> results, int n) {
    int print_limit = static_cast<int>(results.size()) > n ? n : results.size();
    for(int i=0; i<print_limit; i++) {
        fprintf(stderr, "[%d] %s\n", i, results[i].file_name.c_str());
    }
    return;
}

void ui_hide_cursor (void) {
    // Get the console handle
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Get the current console cursor information
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    
    // Set the cursor visibility to false
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}