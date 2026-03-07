#include "utils.h"

// This information is specific to each board.
DeviceInfo device = {
    .uwb_index = 2,
    .local_ip = IPAddress(10, 42, 0, 20),
    .gateway = IPAddress(10, 42, 0, 1),
    .subnet = IPAddress(255, 255, 255, 0),
    .target_ip = IPAddress(10, 42, 0, 1),
    .current_role = UNINITIALIZED,
    .udp = WiFiUDP()
};

void setup() {
    // TODO: set this in the same function?
    // Initialize device
    init_setup(device, ANCHOR);
    // The higher the delay, the shorter the estimated distance
    set_delay(16450);
    String command = "AT+GETANT?";
    send_wifi_data(device, send_radio_data(command, 2000, 0));

    // TODO: I've written logic here that should best be in measurement_loop
    while (true) {
        // Block until "CALIBRATION COMPLETE" or "get_distance(x,y)" is received
        received_command = measurement_loop()

        // TODO: ensure that received command is only either "CAL..." or "get_distance(x,y)"

        // TODO: Make sure "CALIBRATION COMPLETE" is the proper signal
        if (strcmp(received_command, "CALIBRATION COMPLETE") == 0) {
            // TODO: Make sure "ACK" is the proper signal
            send_radio_data("ACK", 1000, 0)
            // Calibration is complete, we can not exit
            break;
        }

        // At this point, we can assume that the received signal was not "CAL..."
        
        int id1;
        int id2;
        parse_get_distance(received_command, id1, id2);

        if (device.uwb_index == id1) {
            // Wait until the other device in the get_distance command sends SUCCESS
            wait_for_measurement(id2);
            continue;
        
        } else if (device.uwb_index == id2) {
            // TODO: fix these arguments
            set_role(device, TAG);

            // TODO: Modify this to be proper
            int distance = get_distance();

            // TODO: set the proper string to send ID,SUCCESS
            send_radio_data("ID,SUCCESS", 1000, 0);
            
            // Transform back into an anchor
            set_role(device, ANCHOR);

            // Send the distance between this tag and id2 to the laptop
            send_radio_data(String(distance), 1000, 0);
        }

    }
}

void loop () {
    // Request the antennae delay
}