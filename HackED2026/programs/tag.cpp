#include "utils.h"

// This information is specific to each board. Set to connect to my laptop.
DeviceInfo device = {
    .uwb_index = 2,
    .local_ip = IPAddress(10, 42, 0, 40),
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
    String message = "";
    read_serial(message, 0);
    if (message.length() > 0)
        send_wifi_data(device, message);

    // for (int i = 0; i < message.length(); i++) {
    //     send_wifi_data(device,
    //         String(i) + ": " +
    //         String((int)message[i]) + " '" + message[i] + "'"
    //     );
    // }

}