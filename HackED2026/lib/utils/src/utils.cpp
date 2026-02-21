#include <utils.h>

HardwareSerial SERIAL_AT(2);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

///////////////////
// CONFIGURATION //
///////////////////

void init_setup(DeviceInfo& device, DeviceRole new_role) {
    // Start the UWB module cleanly
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);

    // Set up WiFI
    WiFi.begin(SSID, PASSWORD); 
    while (WiFi.status() != WL_CONNECTED) delay(500);
    delay(1000);
    device.udp.begin(LOCAL_PORT);

    // Set up the board
    SERIAL_LOG.begin(115200);
    SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    SERIAL_AT.println("AT"); // Sanity check the UWB module.

    // Initialize the OLED display
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        SERIAL_LOG.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();

    // Display the firmware version
    String version = parse_software_version(device, send_radio_data("AT+GETVER?", 2000, 0));
    set_role(device, new_role, version);
}


String config_cmd(DeviceInfo& device) {
    String temp = "AT+SETCFG=";
    temp = temp + device.uwb_index; // Set device id
    temp = temp + "," + device.current_role; // Set device role (0:Tag / 1:Anchor)
    temp = temp + ",1"; // Set frequency 850k or 6.8M
    temp = temp + ",1"; // Set range filter

    return temp;
}


String cap_cmd() {
    String temp = "AT+SETCAP=";
    temp = temp + UWB_TAG_COUNT; // Set Tag capacity
    temp = temp + ",10"; //  Time of a single time slot  6.5M : 10MS  850K ： 15MS
    //X3:extMode, whether to increase the passthrough command when transmitting
    //(0: normal packet when communicating, 1: extended packet when communicating)
    temp = temp + ",1";
    
    return temp;
}    


///////////////////
// COMMUNICATION //
///////////////////

String send_radio_data(String command, const int timeout, boolean debug) {
    String response = "";
    // command = command + "\r\n";

    // Sends the string via WiFi
    // send_wifi_data(command);
    // Sends the string to other devices via radio
    // The command string contains 'AT+DATA=<# of chars>,<message>'
    SERIAL_AT.println(command); // send the read character to the SERIAL_LOG

    long int time = millis();

    while ((time + timeout) > millis()) {
        while (SERIAL_AT.available()) {
            // The esp has data so display its output to the serial window
            char c = SERIAL_AT.read(); // read the next character.
            response += c;
        }
    }
    //if (debug) send_wifi_data("Response: " + response);

    return response;
}


void send_wifi_data(DeviceInfo& device, const String& message) {
    String msg = String(device.uwb_index) + ": " + message;
    device.udp.beginPacket(device.target_ip, TARGET_PORT);
    device.udp.print(msg);
    device.udp.endPacket();
}


/////////////
// HELPERS //
/////////////

void updateOLED(DeviceInfo& device, const String& message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("SnowScape");

    display.setCursor(0, 12);
    display.print("ID: ");
    display.println(device.uwb_index);

    display.setCursor(0, 22);
    display.print("Role: ");
    display.println(device.current_role == TAG ? "TAG" : "ANCHOR");

    display.setCursor(0, 32);
    display.print("SW: ");
    display.println(message);

    display.setCursor(0, 42);
    display.print("IP: ");
    display.println(device.local_ip.toString());

    display.display();
}


// void set_role(DeviceRole& current_role, DeviceRole new_role, int uwb_index, const String& message, IPAddress local_ip) {
void set_role(DeviceInfo& device, DeviceRole new_role, const String& message) {   
    if (device.current_role == new_role) return;

    device.current_role = new_role;

    send_radio_data("AT?", 2000, 1); // Verify that serial port communication to the module works
    send_radio_data("AT+RESTORE", 5000, 0); // Clear config info and reset the config to default
    send_radio_data(config_cmd(device), 2000, 0); // Runs AT+SETCFG
    send_radio_data(cap_cmd(), 2000, 0); // Run AT+SETCAP to set the system refresh rate
    send_radio_data("AT+SETRPT=1", 2000, 0); // Set automatic distance reporting status (1 = on)
    send_radio_data("AT+SAVE", 2000, 0); // Save the config in non-volatile flash
    send_radio_data("AT+RESTART", 2000, 0); // Reload the config saved in flash memory

    updateOLED(device, message);
}


void set_delay(int delay) {
    send_radio_data(String("AT+SETANT=") + String(delay), 2000, 0);
    send_radio_data("AT+SAVE", 2000, 0); // Save the config in non-volatile flash
    send_radio_data("AT+RESTART", 2000, 0); // Reload the config saved in flash memory
}


bool read_serial(String& message, boolean debug) {
    if (!SERIAL_AT.available()) return false;

    String line = SERIAL_AT.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) return false;

    if (debug) {
        Serial.print("RAW: ");
        Serial.println(line);
    }
    
    message = line;    
    return true;
}


// int get_range(void) {
//     int buf_size = 5;
//     int stability_threshold = 5;
//     int buffer[buf_size];
//     int index = 0;
//     bool filled = false;

//     while (true) {
//         int new_value = get_measurement();

//         buffer[index] = new_value;
//         index = (index + 1) % buf_size;

//         // At this point, we know we've gone through the whole buffer
//         if (index == 0)
//             filled = true;

//         if (!filled)
//             continue;

//         int max = buffer[0];
//         int min = buffer[0];
//         int sum = buffer[0];

//         for (int i = 1; i < buf_size; i++) {
//             if (buffer[i] > max) max = buffer[i];
//             if (buffer[i] < min) min = buffer[i];
//             sum += buffer[i];
//         }

//         if ((max - min) < stability_threshold) {
//             // The mean value
//             return sum / buf_size;
//         }
//     }
// }


// int sample_index = 0;


// void get_ranges(int ranges[NUM_ANCHORS]) {
//     struct RangeBuffer buff_array[NUM_ANCHORS];
//     int index = 0;
//     bool filled = false;

//     while (true) {
//         // Returns a list of distances
//         get_measurement(ranges);

//         for (int i = 0; i < NUM_ANCHORS; i++) 
//             buff_array[i].buffer[index] = ranges[i];

//         index = (index + 1) % BUFF_SIZE;

//         // At this point, we know we've gone through the whole buffer
//         if (index == 0)
//             filled = true;

//         if (!filled)
//             continue;

//         bool can_return = true;

//         // Reset max, min, and sum after every buffer update
//         for (int i = 0; i < NUM_ANCHORS; i++) {
//             int max = buff_array[i].buffer[0];
//             int min = buff_array[i].buffer[0];
//             int sum = buff_array[i].buffer[0];

//             for (int j = 1; j < BUFF_SIZE; j++) {
//                 int v = buff_array[i].buffer[j];
                
//                 if (v > max) max = v;
//                 if (v < min) min = v;
//                 sum += v;
//             }

//             if (max - min > STABILITY_THRESHOLD) {
//                 can_return = false;
//                 break;
//             }

//             // Calculate the mean distance to the ith anchor
//             ranges[i] = sum / BUFF_SIZE;

//         }

//         // can_return is true when all distances have converged
//         if (can_return) return;

//     }
// }

/////////////
// PARSERS //
/////////////

String parse_software_version(DeviceInfo& device, String version) {
    // Remove \0 from the beginning of the string
    while (version.length() > 0 && version[0] == '\0') {
        version.remove(0, 1);  // remove the first character
    }
    
    int start = version.indexOf("software:");
    if (start < 0) return "";

    start += strlen("software:");

    int end = version.indexOf(",", start);

    return version.substring(start, end);
}


// void parse_range(char message[]) {
//     int i = 0;
//     int ranges[8];

//     char *start = strstr(message, "range:(");
//     if (!start) return;
//     // Move past "range:(""
//     start += 7;

//     char *end = strchr(start, ')');
//     if(!end) return;

//     char temp[128];  // Make sure it's large enough
//     size_t len = end - start;
//     strncpy(temp, start, len);
//     temp[len] = '\0';

//     // Tokenize by comma
//     char *token = strtok(temp, ",");
//     while (token && i < 8) {
//         ranges[i++] = atoi(token);  // Convert to int
//         token = strtok(NULL, ",");
//     }

//     // Print result
//     for (int j = 0; j < i; j++) {
//         printf("%d ", ranges[j]);
//     }
//     printf("\n");

// }