#include <iostream>
#include "..\inc\MKBDIO.h"
#include "..\inc\SystemUtilities.h"

int main() {
    MKBDIO io_handle;
    
    // Example usage
    while (true) {
        for(int i=64; i<91; i++) {
            if (io_handle.key_state(i)) {
                fprintf(stderr, "%c ", io_handle.code_to_char(i));
            } 
        }
        fprintf(stderr, "\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}