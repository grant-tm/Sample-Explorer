
// External Library Inclusions
#include <windows.h>

// Project Inclusions
#include "FileProcessing.h"
#include "SystemUtilities.h"
#include "DatabaseInteractions.h"
#include "ThreadSafeQueue.h"
#include "UI.h"

// definitions
namespace fs = std::filesystem;
#define UI_SEARCH_WIDTH 28
#define UI_RESULT_WIDTH 28
#define UI_TAG_WIDTH 21

// COMPILE: g++ app.cpp -o app -std=c++17 -fopenmp -lstdc++fs -lsqlite3 -I\Users\Grant\Documents\SQLite -Wall -Wpedantic

//==============================================================================
// UI
//==============================================================================

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

//==============================================================================
// MAIN
//==============================================================================

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
                printf("Key: %d", key);
                return vk_to_char(key);
            }
        }
        Sleep(10);
    }
}

int main (int argc, char* argv[]) {
   
    sqlite3* db = nullptr;
    if (sqlite3_open("audio_files.db", &db) != SQLITE_OK) {
        panicf("Cannot open database.\n");
    }

    db_initialize(db);
    const std::string dir_path = "D:/Samples";

    fprintf(stderr, "Starting Scan\n");
    auto start = std::chrono::high_resolution_clock::now();
    //scan_directory(db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    fprintf(stderr, "Scan duration: %f\n", duration.count() / 1000);

    if (db_table_valid(db, "audio_files")) {
        int num_rows = db_get_num_rows(db, "audio_files");
        fprintf(stderr, "Number of rows: %d\n", num_rows);
    }

    UIState ui_state;
    render_ui(&ui_state);
    while(1) {
        Sleep(50);
        char c = keyboard_listener();
        ui_state.control_queue.push(c);
        ui_state.process_inputs();
        render_ui(&ui_state);
    }

    return 0;
}