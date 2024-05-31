// Project Inclusions
#include "..\inc\SystemUtilities.h"
#include "..\inc\Database.h"
#include "..\inc\MKBDIO.h"
#include "..\inc\UI.h"
#include "..\inc\ThreadSafeQueue.h"
#include "..\inc\Scanner.h"

// definitions
namespace fs = std::filesystem;

void fetch_results (sqlite3* db, UIState *ui_state) {
    if (ui_state->search_exec) {
        const std::string &query = ui_state->search_buffer;
        ui_state->files = db_search_files_by_name(db, query);
    }
}

void thread1 (UIState *ui_state, MKBDIO *io_handle) {
    while(1) {
        for (int i=0; i<256; i++) {
            if(io_handle->key_is_fresh(i) == true) {
                io_handle->key_mark_unfresh(i);
                ui_state->control_queue->push(io_handle->code_to_char(i));
            }
        }
    }
}

void thread2 (sqlite3 *db, UIState *ui_state) {
    while(1) {
        ui_state->control_queue->start_producing();
        Sleep(200);
        while(ui_state->control_queue->is_producing()) {
            ui_state->process_inputs();
            if(ui_state->search_exec) {
                fetch_results(db, ui_state);
            }
            std::system("cls");
            render_ui(ui_state);
            while (ui_state->control_queue->is_producing() && 
                   ui_state->control_queue->empty()) {
                Sleep(100);
            }
        }
    }
}

int main (int argc, char* argv[]) {
   
    // open the database
    sqlite3* db = nullptr;
    if (sqlite3_open("audio_files.db", &db) != SQLITE_OK) {
        panicf("Cannot open database.\n");
    }
    db_initialize(db);

    // scan the files
    fprintf(stderr, "Scanning Files...\n");
    const std::string dir_path = "D:/Samples/Instruments/Keys";
    int db_size_before = db_get_num_rows (db, "audio_files");
    auto start = std::chrono::high_resolution_clock::now();
    scan_directory(db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    int db_size_after = db_get_num_rows (db, "audio_files");

    // report results
    fprintf(stderr, "Files Scanned: %d\n", db_size_after - db_size_before);
    fprintf(stderr, "Scan duration: %f\n", duration.count() / 1000);
    fprintf(stderr, "Scan Performance: %f Files / Second\n", float(db_size_after - db_size_before) / (duration.count()/1000));

    fprintf(stderr, "\rStarting in 3...");
    Sleep(1000);
    fprintf(stderr, "\rStarting in 2...");
    Sleep(1000);
    fprintf(stderr, "\rStarting in 1...");
    Sleep(1000);
    fprintf(stderr, "\r\n");

    // // create the UI
    // ui_hide_cursor();
    // ThreadSafeQueue<char> queue;
    // UIState *ui_state = new UIState;
    // ui_state->control_queue = &queue;
    // MKBDIO io_handle;
    // std::thread t1(&thread1, ui_state, &io_handle);
    // std::thread t2(&thread2, db, ui_state);
    // t1.join();
    // t2.join();

    // // clean up and exit
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  
    // delete ui_state->control_queue;
    // delete ui_state;
    fprintf(stderr, "Successful Exit\n");
    return EXIT_SUCCESS;
}