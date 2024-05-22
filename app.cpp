#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdarg>
#include <chrono>
#include <omp.h>
#include "sqlite3.h"
#include "ThreadSafeQueue.h"
#include "ExplorerFile.h"

namespace fs = std::filesystem;

#define TRANSACTION_SIZE 8192
int files_added = 0;

// COMPILE: g++ app.cpp -o app -std=c++17 -fopenmp -lstdc++fs -lsqlite3 -I\Users\Grant\Documents\SQLite -Wall -Wpedantic

//=============================================================================
// Utility
//=============================================================================

void panicf [[noreturn]] (const char* msg, ...) {
    va_list args;
    
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    std::exit(EXIT_FAILURE);
}

//=============================================================================
// Generate and Manipulate Tags
//=============================================================================

constexpr inline bool char_is_delimiter (char ch) {
    return ch == '_' || ch == ' ' || ch == '-' || ch == '.' || ch == '(' || ch == ')' || ch == '[' || ch == ']';
}

// Function to convert a character to lowercase (used inline)
constexpr inline char to_lower (char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// Generate tags from filename
std::vector<std::string> generate_auto_tags(const std::string& file_name) {
    
    std::vector<std::string> tags;
    std::string token;
    
    for (char ch : file_name) {
        if (!char_is_delimiter(ch)) [[likely]] {
            token += to_lower(ch);
            continue;
        }
        else if (!token.empty()) [[unlikely]] {
            if(token.size() > 2) {
                tags.push_back(token);
            }
            token.clear();
        }
    }
    if (!token.empty() && token.size() > 2) {
        tags.push_back(token);
    }
    
    return tags;
}

std::string concatenate_tags(const std::vector<std::string>& tags) {
    std::string result;
    for (const auto& tag : tags) {
        if (!result.empty()) [[unlikely]] {
            result += " ";
        }
        result += tag;
    }
    return result;
}

//=============================================================================
// Database Interactions
//=============================================================================

bool db_valid (sqlite3* db, const std::string& table_name) {
    
    // Prepare the SQL statement
    std::string sql = "SELECT 1 FROM " + table_name + " LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_valid: Failed to prepare statement.\n");
    }

    // Execute the SQL statement
    bool valid = (sqlite3_step(stmt) == SQLITE_ROW);

    // Finalize the statement
    sqlite3_finalize(stmt);
    
    // Return validity
    return valid;
}

void db_print_num_rows (sqlite3 *db) {

    // Prepare SQL statement
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM audio_files";  
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        panicf("db_print_num_rows: Failed to prepare statement.\n");
    }

    // Execute the SQL statement
    if (sqlite3_step(stmt) == SQLITE_ROW) [[likely]] {
        int rowCount = sqlite3_column_int(stmt, 0);
        std::cout << "Number of rows: " << rowCount << std::endl;
    } else [[unlikely]] {
        panicf("db_print_num_rows: Failed to execute statement\n.");
    }

    // Finalize the statement and close the database
    sqlite3_finalize(stmt);
}

void db_print_n_rows(sqlite3* db, const std::string& table_name, int num_rows) {
    std::string sql = "SELECT * FROM " + table_name + " LIMIT ?;";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_print_n_rows: Failed to prepare statement: %s.\n", sqlite3_errmsg(db));
    }

    // Bind the limit value to the statement
    if (sqlite3_bind_int(stmt, 1, num_rows) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_print_n_rows: Failed to prepare statement: %s.\n", sqlite3_errmsg(db));
    }

    // Execute the SQL statement and print the results
    int columnCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < columnCount; ++col) {
            const char* col_name = sqlite3_column_name(stmt, col);
            const char* col_text = (const char*)sqlite3_column_text(stmt, col);
            std::cout << col_name << ": " << (col_text ? col_text : "NULL") << "  ";
        }
        std::cout << std::endl;
    }
}

bool db_entry_exists (sqlite3* db, std::string file_path) {
    std::string sql = "SELECT 1 FROM audio_files WHERE file_path = ? LIMIT 1;";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_entry_exists: Failed to prepare statement: %s.\n", sqlite3_errmsg(db));
    }

    // Bind the file path to the statement
    if (sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_entry_exists: Failed to bind values: %s.\n", sqlite3_errmsg(db));
    }

    // Execute the SQL statement and check if the file exists
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    // Finalize the statement
    sqlite3_finalize(stmt);

    return exists;
}

void db_initialize (sqlite3 *db) {
    
    const char* sql = "CREATE TABLE IF NOT EXISTS audio_files ("\
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
                        "file_path TEXT UNIQUE,"\
                        "file_name TEXT NOT NULL,"\
                        "file_size INTEGER,"\
                        "duration REAL,"\
                        "num_user_tags INTEGER,"\
                        "user_tags TEXT NOT NULL,"\
                        "num_auto_tags INTEGER,"\
                        "auto_tags TEXT NOT NULL,"\
                        "user_bpm INTEGER,"\
                        "user_key INTEGER,"\
                        "auto_bpm INTEGER,"\
                        "auto_key INTEGER"\
                    ");";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        sqlite3_free(err_msg);
        panicf("db_initialize: Error creating table\n");
    }
}

void db_insert_file(sqlite3 *db, const struct ExplorerFile *file, sqlite3_stmt *stmt) {
    sqlite3_bind_text(stmt, 1, file->file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, file->file_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, file->file_size);
    sqlite3_bind_double(stmt, 4, file->duration);
    sqlite3_bind_int(stmt, 5, file->num_user_tags);
    sqlite3_bind_text(stmt, 6, file->user_tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, file->num_auto_tags);
    sqlite3_bind_text(stmt, 8, file->auto_tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, file->user_bpm);
    sqlite3_bind_int(stmt, 10, file->user_key);
    sqlite3_bind_int(stmt, 11, file->auto_bpm);
    sqlite3_bind_int(stmt, 12, file->auto_key);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error inserting data: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_reset(stmt); // Reset the statement to use it again
}

void db_insert_files (sqlite3 *db, ThreadSafeQueue<struct ExplorerFile *> &files) {
    files_added += files.size();
    fprintf(stderr, "\rInserted %d files.", files_added);
    
    const char* sql =   "INSERT OR IGNORE INTO audio_files ("\
                            "file_path,"\
                            "file_name,"\
                            "file_size,"\
                            "duration,"\
                            "num_user_tags,"\
                            "user_tags,"\
                            "num_auto_tags,"\
                            "auto_tags,"\
                            "user_bpm,"\
                            "user_key,"\
                            "auto_bpm,"\
                            "auto_key)"\
                        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("Error preparing statement: %s\n.", sqlite3_errmsg(db));
        return;
    } 

    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    while (!files.empty()) {
        struct ExplorerFile* file;
        files.wait_pop(file);
        db_insert_file(db, file, stmt);
        delete file;
    }

    // Commit transaction
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_finalize(stmt);
}

//=============================================================================
// Scanning
//=============================================================================

struct ExplorerFile *process_file (sqlite3 *db, const fs::directory_entry& file) {
    // allocate memory for entry parameters
    struct ExplorerFile *db_entry = new struct ExplorerFile;
    
    // calcualte all entry parameters
    db_entry->file_path = file.path().string();
    db_entry->file_name = file.path().filename().string();
    db_entry->file_size = fs::file_size(file.path());
    
    db_entry->duration = 0;
    
    db_entry->num_user_tags = 0;
    db_entry->user_tags = "";
    
    std::vector<std::string> tags = generate_auto_tags(db_entry->file_name);
    db_entry->num_auto_tags = tags.size();
    db_entry->auto_tags = concatenate_tags(tags);
    
    db_entry->user_bpm = 0;
    db_entry->user_key = 0;
    
    db_entry->auto_bpm = 0;
    db_entry->auto_key = 0;

    return db_entry;
}

inline bool validate_file_extension (const fs::directory_entry& file) {    
    auto extension = file.path().extension().string();
    if (extension == ".mp3" || extension == ".wav") {
        return true;
    } else {
        return false;
    }
}

std::vector<fs::path> find_subdirectories (const fs::path& root_path) {
    std::vector<fs::path> subdirectories;
    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (entry.is_directory()) {
            subdirectories.push_back(entry.path());
        }
    }
    return subdirectories;
}

bool requires_processing (sqlite3* db, const fs::directory_entry file) {
    if (file.is_regular_file() && 
        validate_file_extension(file) && 
        !db_entry_exists(db, file.path().string())) [[unlikely]] {
        return true;
    } else [[likely]] {
        return false;
    }
}

// // unparallelized scan function
// void scan_directory (sqlite3 *db, const fs::path& dir_path) {    
//     for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
//         if (entry.is_regular_file() && validate_file_extension(entry)) {    
//             process_file(db, entry);
//         }
//     }
// }

// void scan_directory (sqlite3 *db, const fs::path& dir_path) {
    
//     // scan subdirectories for valid files and push them on the thread safe queue
//     std::vector<fs::path> subdirs = find_subdirectories(dir_path);
//     int num_subdirs = subdirs.size();
//     fprintf(stderr, "Top-Level Subdirectories: %d\n", num_subdirs);
    
//     ThreadSafeQueue<fs::directory_entry> files;

//     // push all directory entries
//     #pragma omp parallel for
//     for(int i=0; i<num_subdirs; i++) {
//         if (fs::exists(subdirs[i])) {
//             for (const auto& entry : fs::recursive_directory_iterator(subdirs[i])) {
//                 if (requires_processing(db, entry)) {
//                     files.push(entry);
//                 }
//             }
//         }
//     }
    
//     int num_files = files.size();
//     fprintf(stderr, "Unprocessed Files: %d\n", num_files);

//     // process valid files
//     ThreadSafeQueue<struct ExplorerFile *> explorer_file_queue;
//     #pragma omp parallel for
//     for (int i=0; i<num_files; i++) {
//         fs::directory_entry file;
//         files.wait_pop(file);
//         if (!file.path().empty()) {
//             struct ExplorerFile *processed_file = process_file(db, file);
//             explorer_file_queue.push(processed_file);
//             if(explorer_file_queue.size() >= TRANSACTION_SIZE) {
//                 db_insert_files(db, explorer_file_queue);
//             }
//         }
//     }

//     db_insert_files(db, explorer_file_queue);
//     fprintf(stderr, "\n");
// }

void scan_directory (sqlite3 *db, const fs::path& dir_path) {
    
    // scan subdirectories for valid files and push them on the thread safe queue
    std::vector<fs::path> subdirs = find_subdirectories(dir_path);
    int num_subdirs = subdirs.size();
    fprintf(stderr, "Top-Level Subdirectories: %d\n", num_subdirs);
    
    ThreadSafeQueue<fs::directory_entry> files;
    ThreadSafeQueue<struct ExplorerFile *> explorer_file_queue;

    

    #pragma omp sections
    {
        #pragma omp section
        {
            files.start_producing();

            // push all directory entries
            #pragma omp parallel for
            for(int i=0; i<num_subdirs; i++) {
                if (fs::exists(subdirs[i])) {
                    for (const auto& entry : fs::recursive_directory_iterator(subdirs[i])) {
                        if (requires_processing(db, entry)) {
                            files.push(entry);
                        }
                    }
                }
            }

            files.stop_producing();
        }
        
        #pragma omp section
        {
            while (!files.empty() || files.is_producing()) {
                fs::directory_entry file;
                files.wait_pop(file);
                if (!file.path().empty()) {
                    struct ExplorerFile *processed_file = process_file(db, file);
                    explorer_file_queue.push(processed_file);
                    if(explorer_file_queue.size() >= TRANSACTION_SIZE) {
                        db_insert_files(db, explorer_file_queue);
                    }
                }
            }
        }
    }

    db_insert_files(db, explorer_file_queue);
    fprintf(stderr, "\n");
}

//=============================================================================
// MAIN
//=============================================================================

int main (int argc, char* argv[]) {
   
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    const std::string dir_path = argv[1];
    
    sqlite3* db = nullptr;

    if (sqlite3_open("audio_files.db", &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    db_initialize(db);

    if (db_valid(db, "audio_files")) {
        db_print_num_rows(db);
    }
    std::cout << "Starting Scan" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    scan_directory(db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Scan Duration: " << duration.count() / 1000 << " seconds" << std::endl;

    if (db_valid(db, "audio_files")) {
        db_print_num_rows(db);
    }
    
    // db_print_n_rows(db, "audio_files", 5);

    sqlite3_close(db);

    return 0;
}