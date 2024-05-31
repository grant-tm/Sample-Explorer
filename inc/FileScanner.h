#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

// Standard Library Inclusions
#include <string>
#include <vector>
#include <filesystem>
#include <thread>

// External Inclusions
#include "sqlite3.h"

// Project Inclusions
#include "Database.h"
#include "SystemUtilities.h"
#include "ThreadSafeQueue.h"
#include "ExplorerFile.h"
#include "KeySignitureDetector.h"

// Definitions
namespace fs = std::filesystem;
#define TRANSACTION_SIZE 2048

// Delimiter check function
constexpr inline bool char_is_delimiter (char);

// Character to lowercase conversion
constexpr inline char to_lower (char);

// Tag generation function
std::vector<std::string> generate_auto_tags (const std::string &);

// Tag concatenation function
std::string concatenate_tags (const std::vector<std::string> &);

// File processing function
struct ExplorerFile *process_file (sqlite3 *, const fs::directory_entry &);

// File extension validation
inline bool validate_file_extension (const fs::directory_entry *);

// Sub-directory finding function
std::vector<fs::path> find_sub_dirs (const fs::path &);

// File processing requirement check
bool requires_processing (sqlite3 *, const fs::directory_entry *);

// File queueing functions
void queue_files (sqlite3 *, const fs::path &, 
                ThreadSafeQueue<fs::directory_entry> *);

void queue_all_files (sqlite3 *, const fs::path &, 
                ThreadSafeQueue<fs::directory_entry> *);

// Processing queued files function
void process_queued_files (sqlite3 *, 
        ThreadSafeQueue<fs::directory_entry> *,
        ThreadSafeQueue<struct ExplorerFile *> *);

// Insert processed files function
void insert_processed_files (sqlite3*, 
    ThreadSafeQueue<struct ExplorerFile *> *);

// Directory scanning function
void scan_directory (sqlite3 *, const fs::path &);

#endif // FILE_SCANNER_H
