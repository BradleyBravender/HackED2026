#include <stdbool.h>
#include <string.h> // if you use strncpy later

#define MAX_MSG_LEN 128

typedef bool (*pattern_fn_t)(const char *msg);


bool pattern_ack(const char *msg) {
    // TODO
}

bool pattern_ok(const char *msg) {
    // TODO
}

// This function blocks until a match is received
void receive_signal(
    pattern_fn_t p1,
    pattern_fn_t p2,
    bool matched[2]
) {

    char buffer[MAX_MSG_LEN];
    
    while (1) {
        // Read the next message
        read_from_serial(buffer);

        // Reset after each loop
        matched[0] = p1(buffer);
        matched[1] = p2(buffer);

        // If either pattern was matched, return
        if (matched[0] || matched[1]) return;
        
    }
}

int main() {
    bool matched[2];
    receive_signal(pattern_ack, pattern_ok, matched);
    return 0;
}