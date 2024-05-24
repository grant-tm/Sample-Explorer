#ifndef EXPLORER_FILE_H
#define EXPLORER_FILE_H

// Standard Library Inclusions
#include <string>

struct ExplorerFile {
    std::string file_path;
    std::string file_name;
    int file_size;
    
    int duration;
    
    int num_user_tags;
    std::string user_tags;
    
    int num_auto_tags;
    std::string auto_tags;
    
    int user_bpm;
    int user_key;
    
    int auto_bpm;
    int auto_key;
};

#endif