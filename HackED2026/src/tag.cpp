#include "utils.h"

// This information is specific to each board. Set to connect to my laptop.
DeviceInfo device = {
    .uwb_index = 3,
    .local_ip = IPAddress(10, 42, 0, 30),
    .gateway = IPAddress(10, 42, 0, 1),
    .subnet = IPAddress(255, 255, 255, 0),
    .target_ip = IPAddress(10, 42, 0, 1),
    .current_role = UNINITIALIZED,
    .udp = WiFiUDP()
};


void setup() {
    // Initialize device
    init_setup(device, TAG);
}

void loop () {
    // String message = "";
    // read_serial(message, 0);
    // if (message.length() > 0)
    //     send_wifi_data(device, message);

    int parsed_ranges[NUM_ANCHORS] = {0};
    // get_converged_ranges(device, converged_ranges);
    if (get_raw_ranges(device, parsed_ranges)) {;

        send_wifi_data(device, 
            String("Raw Value: ") +
            String(parsed_ranges[0]) + ", " + 
            String(parsed_ranges[1]) + ", " + 
            String(parsed_ranges[2]) + ", " + 
            String(parsed_ranges[3]) + ", " + 
            String(parsed_ranges[4]) + ", " + 
            String(parsed_ranges[5]) + ", " + 
            String(parsed_ranges[6]) + ", " + 
            String(parsed_ranges[7]) + "\n"
        );
    }
}