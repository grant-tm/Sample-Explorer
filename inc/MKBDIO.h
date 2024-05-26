#ifndef MKBDIO_H
#define MKBDIO_H

#include <windows.h>
#include <array>
#include <mutex>
#include <thread>

#define NUM_KEYS 256

// commands
#define KBD_CTRL_S 19
#define KBD_CTRL_R 18
#define KBD_CTRL_T 20
#define KBD_CTRL_Q 17

// non-printable characters
#define KBD_RETURN 10
#define KBD_BACKSPACE 8
#define KBD_DELETE 127
#define KBD_SHIFT 16

class MKBDIO {
public:
    
    MKBDIO (void);
    ~MKBDIO (void);

    // conversions
    char code_to_char (int);
    int  char_to_code (char);

    // thread safe state checks
    bool key_state (char);
    bool key_state (int);

    bool key_is_fresh (char);
    bool key_is_fresh (int);

    void key_mark_unfresh (char);
    void key_mark_unfresh (int);

private:
    
    // key state
    bool key_states[NUM_KEYS];
    bool key_fresh[NUM_KEYS];
    
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
