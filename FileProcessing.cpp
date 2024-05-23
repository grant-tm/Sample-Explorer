#include "FileProcessing.h"

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