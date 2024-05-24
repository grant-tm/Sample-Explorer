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

void fetch_results (sqlite3* db, UIState *ui_state) {
    if (ui_state->search_exec) {
        const std::string &query = ui_state->search_buffer;
        ui_state->files = db_search_files_by_name(db, query);
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
    scan_directory(db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    fprintf(stderr, "Scan duration: %f\n", duration.count() / 1000);

    if (db_table_valid(db, "audio_files")) {
        int num_rows = db_get_num_rows(db, "audio_files");
        fprintf(stderr, "Number of rows: %d\n", num_rows);
    }

    ui_hide_cursor();

    ThreadSafeQueue<char> queue;
    UIState *ui_state = new UIState;
    ui_state->control_queue = &queue;
    render_ui(ui_state);
    
    #pragma omp parallel sections 
    {
        #pragma omp section
        {
            ui_state->control_queue->start_producing();
            Sleep(1000);
            while(ui_state->control_queue->is_producing()) {
                ui_state->process_inputs();
                if(ui_state->search_exec) {
                    fetch_results(db, ui_state);
                }
                render_ui(ui_state);
                while (ui_state->control_queue->is_producing() && 
                            ui_state->control_queue->empty()) {
                    Sleep(25);
                }
            }
            fprintf(stderr, "UI RENDERING THREAD EXITING\n");
        }

        #pragma omp section
        {
            while(1) {
                Sleep(25);
                char c = keyboard_listener();
                ui_state->control_queue->push(c);
                if (ui_state->frame == 'q') {
                    ui_state->control_queue->stop_producing();
                    break;
                }
            }
            fprintf(stderr, "KEYBOARD CONTROL THREAD EXITING\n");
        }

        
    }

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  

    delete ui_state->control_queue;
    delete ui_state;
    return 0;
}