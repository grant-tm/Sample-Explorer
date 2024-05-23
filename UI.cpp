#include "UI.h"

#define UI_SEARCH_WIDTH 28
#define UI_RESULT_WIDTH 28
#define UI_TAG_WIDTH 21


std::string format_string (std::string str, size_t length) {
    if (str.length() <= length) {
        return str + std::string(length - str.length(), ' ');
    } else {
        return str.substr(0, length-3) + "...";
    }
}

void render_ui (UIState *ui_state) {

    std::system("cls");

    std::string display = format_string(ui_state->search_buffer, UI_SEARCH_WIDTH);

    std::string result[5];
    for(size_t i=0; i<5; i++) {
        if(i >= ui_state->files.size()) {
            result[i] = format_string("", UI_RESULT_WIDTH);
        } else {
            int index = ui_state->file_scroll;
            struct ExplorerFile file = ui_state->files[index];
            result[i] = format_string(file.file_name, UI_TAG_WIDTH);
        }
    }

    std::string tags[5];
    for(int i=0; i<5; i++) {
        tags[i] = format_string("example_tag", 21);
    }
    
    std::string ui = 
    "|==============================|=======================|\n"\
    "| " + display              + " | " + tags[0]       + " |\n"\
    "|==============================| " + tags[1]       + " |\n"\
    "| " + result[0]            + " | " + tags[2]       + " |\n"\
    "| " + result[1]            + " | " + tags[3]       + " |\n"\
    "| " + result[2]            + " | " + tags[4]       + " |\n"\
    "| " + result[3]            + " |===========|===========|\n"\
    "| " + result[4]            + " | Key: ---- | BPM: ---- |\n"\
    "|==============================|===========|===========|\n";
    fprintf(stdout, "%s", ui.c_str());
}

void print_search_results (std::vector<struct ExplorerFile> results, int n) {
    int print_limit = static_cast<int>(results.size()) > n ? n : results.size();
    for(int i=0; i<print_limit; i++) {
        fprintf(stderr, "[%d] %s\n", i, results[i].file_name.c_str());
    }
    return;
}