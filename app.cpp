#include "file_processor.h"
#include "system_utilities.h"
#include "database_interactions.h"

namespace fs = std::filesystem;

// COMPILE: g++ app.cpp -o app -std=c++17 -fopenmp -lstdc++fs -lsqlite3 -I\Users\Grant\Documents\SQLite -Wall -Wpedantic

//==============================================================================
// UI
//==============================================================================

void print_search_results (std::vector<struct ExplorerFile> results, int n) {
    int print_limit = static_cast<int>(results.size()) > n ? n : results.size();
    for(int i=0; i<print_limit; i++) {
        fprintf(stderr, "[%d] %s\n", i, results[i].file_name.c_str());
    }
    return;
}

// Function to search files by name
std::vector<ExplorerFile> search_files_by_name (sqlite3* db, 
                                            const std::string& search_query) {
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT "\
                    "file_path, "\
                    "file_name, "\
                    "file_size, "\
                    "duration, "\
                    "num_user_tags, "\
                    "user_tags, "\
                    "num_auto_tags, "\
                    "auto_tags, "\
                    "user_bpm, "\
                    "user_key, "\
                    "auto_bpm, "\
                    "auto_key "\
                    "FROM audio_files WHERE file_name LIKE ?;";
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("search_files_by_name: Failed to prepare statement.\n");
    }
    
    // Bind the search query with wildcard characters for pattern matching
    std::string query_param = "%" + search_query + "%";
    sqlite3_bind_text(stmt, 1, query_param.c_str(), -1, SQLITE_STATIC);
    
    // Execute the statement and process the results
    std::vector<ExplorerFile> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        struct ExplorerFile file;
        
        file.file_path = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 0));
        file.file_name = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 1));
        file.file_size = sqlite3_column_int(stmt, 2);
        file.duration = sqlite3_column_int(stmt, 3);
        file.num_user_tags = sqlite3_column_int(stmt, 4);
        file.user_tags = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 5));
        file.num_auto_tags = sqlite3_column_int(stmt, 6);
        file.auto_tags = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 7));
        file.user_bpm = sqlite3_column_int(stmt, 8);
        file.user_key = sqlite3_column_int(stmt, 9);
        file.auto_bpm = sqlite3_column_int(stmt, 10);
        file.auto_key = sqlite3_column_int(stmt, 11);

        results.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

//==============================================================================
// MAIN
//==============================================================================

int main (int argc, char* argv[]) {
   
    sqlite3* db = nullptr;
    if (sqlite3_open("audio_files.db", &db) != SQLITE_OK) {
        panicf("Cannot open database.\n");
    }

    if (argc >= 2) {
        db_initialize(db);
        if (db_table_valid(db, "audio_files")) {
            db_print_num_rows(db, "audio_files");
        }

        const std::string dir_path = argv[1];

        fprintf(stderr, "Starting Scan\n");
        auto start = std::chrono::high_resolution_clock::now();
        scan_directory(db, dir_path);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        fprintf(stderr, "Scan duration: %f\n", duration.count() / 1000);

        if (db_table_valid(db, "audio_files")) {
            db_print_num_rows(db, "audio_files");
        }
    }

    while(1) {
        fprintf(stdout, "Search: ");
        std::string query;
        std::getline(std::cin, query);
        if(query == "quit" || query == "exit"){
            break;
        }
        print_search_results(search_files_by_name(db, query), 10);
    }

    sqlite3_close(db);

    return 0;
}