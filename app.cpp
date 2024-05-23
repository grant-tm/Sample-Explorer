// Project Inclusions
#include "FileProcessing.h"
#include "SystemUtilities.h"
#include "DatabaseInteractions.h"
#include "Controls.h"
#include "UI.h"
#include "ThreadSafeQueue.h"

// definitions
namespace fs = std::filesystem;

// COMPILE: g++ app.cpp -o app -std=c++17 -fopenmp -lstdc++fs -lsqlite3 -I\Users\Grant\Documents\SQLite -Wall -Wpedantic

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
        Sleep(65);
        char c = keyboard_listener();
        ui_state.control_queue.push(c);
        ui_state.process_inputs();
        render_ui(&ui_state);
    }

    return 0;
}