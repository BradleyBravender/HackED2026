#include "utils.h"

// This information is specific to each board. Set to connect to my laptop.
DeviceInfo device = {
    .uwb_index = 4,
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

    int a0a1 = get_distance(0, 1);
    int a0a2 = get_distance(0, 2);
    int a0v3 = get_distance(0, 3); 
    int a1a2 = get_distance(1, 2); 
    int a1v3 = get_distance(1, 3); 
    int a2v3 = get_distance(2, 3); 

    char distances[64];
    sprintf(distances, "%d %d %d %d %d %d", a0a1, a0a2, a0v3, a1a2, a1v3, a2v3);

    // Send the distances to the laptop
    send_wifi_data(device, distances);
    
    // Wait until the program has calculated the position of every device.
    // TODO
    receive_wifi_data(ok_signal);
    
    // Inform all anchors that the calibration is complete
    send_radio_data("CALIBRATION COMPLETE", 1000, 0);
    
    // Wait until all anchors send their okay signal
    // TODO
    recieve_radio_data(device, "SIGNAL IS ACKNOWLEDGE")


}

void loop () {
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
