#include "..\inc\MKBDIO.h"

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

MKBDIO::MKBDIO() {
    // init the key states to false
    for(int i=0; i<NUM_KEYS; i++) {
        key_states[i] = false;
    }

    // start the thread to listen for key events
    running = true;
    input_capture_thread = std::thread(&MKBDIO::start_listener, this);
}

MKBDIO::~MKBDIO() {
    running = false;
    if (input_capture_thread.joinable()) {
        input_capture_thread.join();
    }
}

// conversions
char MKBDIO::code_to_char (int kbd_code) {
    return static_cast<char>(kbd_code);
}
int MKBDIO::char_to_code (char c) {
    return static_cast<int>(c);
}

// get the state of a specific key from a character
bool MKBDIO::key_state (char c) {
    int vk_code = char_to_code(c);
    std::lock_guard<std::mutex> lock(mutex);
    if (vk_code < 0 || vk_code > 255) {
        return false;
    }
    return key_states[vk_code];
}

// get the state of a specific key from the vk_code
bool MKBDIO::key_state (int vk_code) {
    std::lock_guard<std::mutex> lock(mutex);
    if (vk_code < 0 || vk_code > 255) {
        return false;
    }
    return key_states[vk_code];
}

//=============================================================================
// PRIVATE FUNCTIONS
//=============================================================================

void MKBDIO::start_listener (void) {
    
    // create a hidden window to receive keyboard messages
    HWND hwnd = CreateWindowEx( 
        0, 
        "STATIC", 
        NULL, 
        WS_OVERLAPPEDWINDOW, 
        0, 0, 0, 0, 
        NULL, NULL, NULL, NULL
    );
    
    if (!hwnd) {
        return;
    }

    // set the window to receive raw keyboard input
    setup_raw_input(hwnd);

    // start message loop
    MSG msg;
    while (running) {
        process_messages(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // destory window when done capturing
    DestroyWindow(hwnd);
}

// set the window to receive raw keyboard input
void MKBDIO::setup_raw_input (HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

// respond to WM_INPUT messages, ignore others
void MKBDIO::process_messages (MSG& msg) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_INPUT) {
            handle_raw_input(msg);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// define response to inputs
void MKBDIO::handle_raw_input (MSG& msg) {
    
    UINT dwSize;
    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[dwSize];
    if (!lpb) {
        return;
    }

    if (GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
        RAWINPUT *raw = (RAWINPUT*)lpb;
        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            UINT msg = raw->data.keyboard.Message;
            int vk_code = raw->data.keyboard.VKey;
            process_key_press(msg, vk_code);   
        }
    }

    delete[] lpb;
}

void MKBDIO::process_key_press (UINT msg, int vk_code) {
    std::lock_guard<std::mutex> lock(mutex);
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        key_states[vk_code] = true;
    } else if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
        key_states[vk_code] = false;
    }
}
