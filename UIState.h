
#ifndef UISTATE_H
#define UISTATE_H

#include <string>
#include <vector>

#include "ThreadSafeQueue.h"
#include "ExplorerFile.h"

class UIState {
public:
    char frame = 's'; // s search, r results

    ThreadSafeQueue<char> control_queue;

    std::string search_display = "Search...";
    std::string search_buffer = "";
    std::string search_query = "";
    int search_cursor = 0;

    std::vector<struct ExplorerFile> files;
    int file_scroll = 0;

    void process_inputs(void);
    void control_dispatch(char c);
    void command_handler(char c);

    void search_control_handler(char c);
    // void result_control_handler(char c);

    void update_search_buffer(char c);

    // int set_results_scroll(int scroll);
    // int add_results_scroll(int offset);

};

#endif