#ifndef FILE_PROCESSING_H
#define FILE_PROCESSING_H

// Standard Library Inclusions
#include <string>
#include <vector>
#include <filesystem>

// External Inclusions
#include <omp.h>
#include <thread>
#include "sqlite3.h"

// Project Inclusions
#include "DatabaseInteractions.h"
#include "ThreadSafeQueue.h"
#include "ExplorerFile.h"

// Definitions
namespace fs = std::filesystem;
#define TRANSACTION_SIZE 2048

// Delimiter check function
constexpr inline bool char_is_delimiter(char ch);

// Character to lowercase conversion
constexpr inline char to_lower(char c);

// Tag generation function
std::vector<std::string> generate_auto_tags(const std::string& file_name);

// Tag concatenation function
std::string concatenate_tags(const std::vector<std::string>& tags);

// File processing function
struct ExplorerFile *process_file(sqlite3 *db, const fs::directory_entry& file);

// File extension validation
inline bool validate_file_extension(const fs::directory_entry& file);

// Sub-directory finding function
std::vector<fs::path> find_sub_dirs(const fs::path& root_path);

// File processing requirement check
bool requires_processing(sqlite3* db, const fs::directory_entry file);

// File queueing function
void queue_files_for_processing(sqlite3 *db, const fs::path& dir_path, 
                ThreadSafeQueue<fs::directory_entry> *files_to_process);

// Processing queued files function
void process_queued_files(sqlite3* db, 
        ThreadSafeQueue<fs::directory_entry> *proc_queue,
        ThreadSafeQueue<struct ExplorerFile *> *insrt_queue);

// Insert processed files function
void insert_processed_files(sqlite3* db, 
    ThreadSafeQueue<struct ExplorerFile *> *insrt_queue);

// Directory scanning function
void scan_directory(sqlite3 *db, const fs::path& dir_path);

#endif // FILE_PROCESSING_H