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

#define TRANSACTION_SIZE 2048
int files_added = 0;

// COMPILE: g++ app.cpp -o app -std=c++17 -fopenmp -lstdc++fs -lsqlite3 -I\Users\Grant\Documents\SQLite -Wall -Wpedantic

//==============================================================================
// Utility
//==============================================================================

void panicf [[noreturn]] (const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    std::exit(EXIT_FAILURE);
}

//==============================================================================
// Generate and Manipulate Tags
//==============================================================================

// These delimiters are used to automatically generate tags from filenames
constexpr inline bool char_is_delimiter (char ch) {
    return (ch == '_' || 
            ch == ' ' || 
            ch == '-' || 
            ch == '.' || 
            ch == '(' || 
            ch == ')' || 
            ch == '[' || 
            ch == ']');
}

// to_lower converts characters to their lowercase equivalent.
// This function is used to convert tags to lowercase in generate_auto_tags()
constexpr inline char to_lower (char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// generate_auto_tags generates tags from filename based on a set of delimiters
// the delimiters are set in char_is_delimiter()
// tags must be 3 characters or greater
// tags are returned in a vector of strings
std::vector<std::string> generate_auto_tags(const std::string& file_name) {
    
    std::vector<std::string> tags;
    std::string token;
    
    // for all the characters in the filename:
    // nondelimiter characters are accumualted in 'token'
    // when a delimiter is hit, save token if long enough

    for (char ch : file_name) {
        if (!char_is_delimiter(ch)) [[likely]] {
            // tags are lowercase
            token += to_lower(ch);
            continue;
        }
        else if (!token.empty()) [[unlikely]] {
            // tags must be 3 or more characters
            if(token.size() > 2) {
                tags.push_back(token);
            }
            token.clear();
        }
    }

    // push the reminaing token, if one
    if (!token.empty() && token.size() > 2) {
        tags.push_back(token);
    }
    
    return tags;
}

// concatenate strings in a vector of strings into one space delimited string
std::string concatenate_tags(const std::vector<std::string>& tags) {
    std::string result;
    for (const auto& tag : tags) {
        if (!result.empty()) [[likely]] {
            result += " ";
        }
        result += tag;
    }
    return result;
}

//==============================================================================
// Database Interactions
//==============================================================================

// checks if the given database table exists
bool db_table_valid (sqlite3* db, const std::string& table_name) {
    
    // prepare statement to select 1 element from db and table
    std::string sql = "SELECT 1 FROM " + table_name + " LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_table_valid: Failed to prepare statement.\n");
    }
    
    // execute statement: db table is valid if execution return matches row
    bool valid = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return valid;
}

// print the number of rows in a databse table
void db_print_num_rows (sqlite3 *db, const std::string& table_name) {

    // prepare statement to select the count of all rows in the table
    sqlite3_stmt* stmt;
    std::string sql = "SELECT COUNT(*) FROM " + table_name;  
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        panicf("db_print_num_rows: Failed to prepare statement.\n");
    }

    // execute the SELECT command
    if (sqlite3_step(stmt) == SQLITE_ROW) [[likely]] {
        int row_count = sqlite3_column_int(stmt, 0);
        fprintf(stderr, "Number of rows: %d\n", row_count);
        sqlite3_finalize(stmt);
    } else [[unlikely]] {
        sqlite3_finalize(stmt);
        panicf("db_print_num_rows: Failed to execute SELECT COUNT(*)\n.");
    }
}

// prints the first n entries in a database table
void db_print_n_rows(sqlite3* db, const std::string& table_name, int num_rows) {
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM " + table_name + " LIMIT ?;";
    
    // prepare sql statement to select the first n entries in the databse
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_print_n_rows: Failed to prepare SELECT statement.\n");
    }

    // bind the limit value (number of entries to select) to the statement
    if (sqlite3_bind_int(stmt, 1, num_rows) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_print_n_rows: Failed to bind limit\n.");
    }

    // execute the SQL statement and print the result
    int columnCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < columnCount; ++col) {
            
            // select column name and content
            const char* col_name = sqlite3_column_name(stmt, col);
            const char* col_text = (const char*)sqlite3_column_text(stmt, col);
            
            // print the column contents
            fprintf(stderr, "%s: ", col_name);
            if (col_text) [[likely]] {
                fprintf(stderr, "%s ", col_name);
            } else {
                fprintf(stderr, "NULL ");
            }
        }
        fprintf(stderr, "\n");
    }
}

// determines if an entry already exists in a database table
// file paths are used as unique identifiers of table entries
bool db_entry_exists (sqlite3* db, const std::string& table_name, 
                    std::string file_path) {

    // prepare the SQL statement to select 1 entry with matching file_path
    sqlite3_stmt* stmt;
    std::string sql = "SELECT 1 FROM " + table_name + 
                      " WHERE file_path = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_entry_exists: Failed to prepare SELECT statement\n");
    }

    // bind the file path to the select statment
    if (sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, 
        SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_entry_exists: Failed to bind values.\n");
    }

    // execute the SQL statement to check if the file exists
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

// set up the audio_files table if it doesn't already exist
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

// this function works in conjunction with db_insert_files to submit files in
// transactions. This allows the reuse of a sqlite3_stmt, which is much
// faster than inserting entries one at a time
void db_insert_file(sqlite3 *db, const struct ExplorerFile *file, 
                    sqlite3_stmt *stmt) {
    
    // bind the ExplorerFile data to the INSERT statement arguments
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
        panicf("db_insert_file: Error inserting data.\n");
    }

    // reset the statement for reuse
    sqlite3_reset(stmt); 
}

// db_insert_files inserts entries in the audio_files database table
// data to insert comes from a vector of ExplorerFile structs
void db_insert_files (sqlite3 *db, 
                        ThreadSafeQueue<struct ExplorerFile *> *files) {
    
    // track how many files have been inserted
    files_added += files->size();
    fprintf(stderr, "\rInserted %d files.", files_added);
    
    // create statement to insert all members of explorer file struct
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
        panicf("db_insert_files: Error preparing statemen.\n");
    } 

    // insert files in a single transaction
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    while (!files->empty()) {
        struct ExplorerFile* file;
        files->wait_pop(file);
        db_insert_file(db, file, stmt);
        delete file;
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
}

//==============================================================================
// Scanning
//==============================================================================

// given a directory entry, find and record attributes in ExplorerFile struct
struct ExplorerFile *process_file (sqlite3 *db, 
                                    const fs::directory_entry& file) {
    
    // allocate memory for entry parameters
    struct ExplorerFile *db_entry = new struct ExplorerFile;

    // identification
    db_entry->file_path = file.path().string();
    db_entry->file_name = file.path().filename().string();
    db_entry->file_size = fs::file_size(file.path());
    
    // calculate file duration
    db_entry->duration = 0;
    
    // default user tags
    db_entry->num_user_tags = 0;
    db_entry->user_tags = "";
    
    // generate auto tags
    std::vector<std::string> tags = generate_auto_tags(db_entry->file_name);
    db_entry->num_auto_tags = tags.size();
    db_entry->auto_tags = concatenate_tags(tags);
    
    // default user bpm and key
    db_entry->user_bpm = 0;
    db_entry->user_key = 0;
    
    // TODO: predict bpm and key
    db_entry->auto_bpm = 0;
    db_entry->auto_key = 0;

    return db_entry;
}

// check if file extension is .mp3 or .wav
inline bool validate_file_extension (const fs::directory_entry& file) {    
    auto extension = file.path().extension().string();
    if (extension == ".mp3" || extension == ".wav") {
        return true;
    } else {
        return false;
    }
}

// Find the sub-directories in the top layer in a directory
// 
// This is used as a heuristic for dividing file scanning work between threads.
// However, splitting work by sub-directory may not evenly divide work, and bare 
// files in the root directory won't be scanned without changes to 
// scan_directory() 
std::vector<fs::path> find_sub_dirs (const fs::path& root_path) {
    std::vector<fs::path> sub_dirs;
    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (entry.is_directory()) {
            sub_dirs.push_back(entry.path());
        }
    }
    return sub_dirs;
}

// Check if file meets the requirements to be analyzed and included in the db
// Files must exist, be a regular file, and have a .mp3 or .wav extension.
bool requires_processing (sqlite3* db, const fs::directory_entry file) {
    if (file.is_regular_file() && 
        validate_file_extension(file) && 
        !db_entry_exists(db, "audio_files", file.path().string()))[[unlikely]]{
        return true;
    } else {
        return false;
    }
}

void queue_files_for_processing (sqlite3 *db, const fs::path& dir_path, 
                ThreadSafeQueue<fs::directory_entry> *files_to_process ) {

    // calculate the number of top-level sub-directories in the root directory
    // TODO: Find a better method to split up work
    // This is used as a heuristic to divide work evenly betweeen threads,
    // see description of find_sub_dirs for overview of limitations.
    std::vector<fs::path> sub_dirs = find_sub_dirs(dir_path);
    int num_sub_dirs = sub_dirs.size();
    fprintf(stderr, "Top-Level Sub-directories: %d\n", num_sub_dirs);

    // enable "producing" flag to prevent consumer from exiting early
    files_to_process->start_producing();

    // add directory entries to processing queue
    // entries are added if they are valid .mp3 or .wav files
    #pragma omp parallel for
    for(int i=0; i<num_sub_dirs; i++) {
        fs::path sub_dir = sub_dirs[i];
        if (fs::exists(sub_dir)) {
            for (const auto& file : fs::recursive_directory_iterator(sub_dir)) {
                if (requires_processing(db, file)) {
                    files_to_process->push(file);
                }
            }
        }
    }

    // singal done producing
    files_to_process->stop_producing();
}

void process_queued_files (sqlite3* db, 
        ThreadSafeQueue<fs::directory_entry> *proc_queue,
        ThreadSafeQueue<struct ExplorerFile *> *insrt_queue) {

    insrt_queue->start_producing();
    while (!proc_queue->empty() || proc_queue->is_producing()) {
        fs::directory_entry file;
        proc_queue->wait_pop(file);
        if (!file.path().empty()) {
            struct ExplorerFile *procd_file = process_file(db, file);
            insrt_queue->push(procd_file);
        }
    }
    insrt_queue->stop_producing();
}

void insert_processed_files (sqlite3* db, 
    ThreadSafeQueue<struct ExplorerFile *> *insrt_queue) {

    while (!insrt_queue->empty() || insrt_queue->is_producing()) {
        if(insrt_queue->size() >= TRANSACTION_SIZE) {
            db_insert_files(db, insrt_queue);
        }
    }
    fprintf(stderr, "\rInserted %d files.\n", files_added);
}

void scan_directory (sqlite3 *db, const fs::path& dir_path) {
    
    ThreadSafeQueue<fs::directory_entry> processing_queue;
    ThreadSafeQueue<struct ExplorerFile *> insertion_queue;

    #pragma omp sections
    {
        // locate files eligable for processing & entry
        #pragma omp section
        {
            queue_files_for_processing(db, dir_path, &processing_queue);
        }

        // process files for entry
        #pragma omp section
        {
            process_queued_files(db, &processing_queue, &insertion_queue);
            
        }

        // enter processed files
        #pragma omp section
        {
            insert_processed_files(db, &insertion_queue);
        }
    }
    
}

//==============================================================================
// UI
//==============================================================================

void print_search_results (std::vector<struct ExplorerFile> results, int n) {
    int print_limit = (static_cast<int>(results.size()) > n) ? n : results.size();
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
        
        file.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        file.file_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file.file_size = sqlite3_column_int(stmt, 2);
        file.duration = sqlite3_column_int(stmt, 3);
        file.num_user_tags = sqlite3_column_int(stmt, 4);
        file.user_tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        file.num_auto_tags = sqlite3_column_int(stmt, 6);
        file.auto_tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
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

    print_search_results(search_files_by_name(db, "break"), 10);
    print_search_results(search_files_by_name(db, "keys"), 10);
    print_search_results(search_files_by_name(db, "V_RIOT"), 10);

    sqlite3_close(db);

    return 0;
}