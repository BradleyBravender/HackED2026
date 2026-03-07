#include "utils.h"

// This information is specific to each board. Set to connect to my laptop.
DeviceInfo device = {
    .uwb_index = 0,
    .local_ip = IPAddress(10, 42, 0, 0),
    .gateway = IPAddress(10, 42, 0, 1),
    .subnet = IPAddress(255, 255, 255, 0),
    .target_ip = IPAddress(10, 42, 0, 1),
    .current_role = UNINITIALIZED,
    .udp = WiFiUDP()
};


void setup() {
    // Initialize device
    init_setup(device, ANCHOR);
}

void loop () {
    String message = "AT+DATA=2,HI";
    send_wifi_data(device, message); 
    send_radio_data(message, 1000, 0);
}