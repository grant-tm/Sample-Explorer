#ifndef MKBDIO_H
#define MKBDIO_H

#include <windows.h>
#include <array>
#include <mutex>
#include <thread>

#define NUM_KEYS 256

class MKBDIO {
public:
    MKBDIO (void);
    ~MKBDIO (void);

    // conversions
    char code_to_char (int);
    int  char_to_code (char);

    // thread safe state checks
    bool key_state  (char);
    bool key_state  (int);

private:
    
    // key state
    bool key_states[NUM_KEYS];
    
    // thread management
    std::mutex mutex;
    std::thread input_capture_thread;
    bool running;

    // capture and process keys
    void start_listener (void);
    void process_messages (MSG &);
    void setup_raw_input (HWND);
    void handle_raw_input (MSG &);
    void process_key_press (UINT, int);
};

#endif // KEYBOARD_IO_H
