#include "utils.h"

// This information is specific to each board.
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
    // The higher the delay, the shorter the estimated distance
    set_delay(16450);
    String command = "AT+GETANT?";
    send_wifi_data(device, send_radio_data(command, 2000, 0));
}

void loop () {
    // Request the antennae delay
}