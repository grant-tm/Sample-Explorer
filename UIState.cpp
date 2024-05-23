#include "UIState.h"

void UIState::process_inputs (void) {
    while(!control_queue.empty()) {
        char c;
        control_queue.wait_pop(c);
        control_dispatch(c);
    }
}

void UIState::control_dispatch (char c) {
    command_handler(c);
    switch(this->frame) {
        case 's':
            search_control_handler(c);
            return;
        case 'r':
            //result_control_handler(c);
            return;
        case 't':
            //tags_control_handler(c);
            return;
        case 'q':
            //quit();
            return;
        default:
            return;
    }
}

void UIState::command_handler (char c) {
    switch(c) {
        case 19: // Ctrl-S
            this->frame = 's';
            return;
        case 18: // Ctrl-R
            this->frame = 'r';
            return;
        case 20: // Ctrl-T 
            this->frame = 't';
            return;
        case 17: // Ctrl-Q
            this->frame = 'q';
            return;
        default:
            return;
    }
}

//==============================================================================
// Handle inputs in SEARCH frame
//==============================================================================

void UIState::search_control_handler (char c) {
    fprintf(stderr, "search_control_handler: %d\n", c);
    //std::string prev_query = search_query;
    update_search_buffer(c);
    // if (search_query != prev_query) {
    //     return;
    // }
}

// add a character to the serach buffer
void UIState::update_search_buffer (char c) {    
    
    int character = c;
    
    // backspace
    if (character == 8) {
        printf("erase\n");
        printf("cursor: %d\n", search_cursor);
        if(search_cursor > 0) {
            search_buffer.erase(--search_cursor);
        }
    }
    // delete
    else if (character == 127) {
        if(size_t(search_cursor) < search_buffer.length()-1) {
            search_buffer.erase(search_cursor + 1);
        }
    }
    // enter
    else if (character == 10) {
        search_query = search_buffer;
    }
    // printable characters
    else if (std::isprint(c)) {
        search_buffer.reserve(search_buffer.length() + 1);
        search_buffer.push_back(c);
        search_cursor++;
    }
}

//==============================================================================
// Handle inputs in RESULT frame
//==============================================================================

// void UIState::result_control_handler (char c) {
//     return;
// }

// int UIState::set_results_scroll (int scroll) {
//     if(scroll < 0) {
//         file_scroll = 0;
//     }
//     if(scroll > files.size()) {
//         file_scroll = files.size();
//     }
//     return file_scroll;
// }

// int UIState::add_results_scroll (int offset) {
//     set_results_scroll(file_scroll + offset);
// }