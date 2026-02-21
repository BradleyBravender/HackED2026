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


bool get_raw_ranges(DeviceInfo& device, int parsed_ranges[]) {
    String message = "";
    read_serial(message, 0);
    
    if (message.length() > 0) {
        parse_range(message, parsed_ranges);
        
        // send_wifi_data(device, 
        //     String("Raw Value: ") +
        //     String(parsed_ranges[0]) + ", " + 
        //     String(parsed_ranges[1]) + ", " + 
        //     String(parsed_ranges[2]) + ", " + 
        //     String(parsed_ranges[3]) + ", " + 
        //     String(parsed_ranges[4]) + ", " + 
        //     String(parsed_ranges[5]) + ", " + 
        //     String(parsed_ranges[6]) + ", " + 
        //     String(parsed_ranges[7]) + "\n"
        // );

        return true;

    }

    return false;
}


void get_converged_ranges(DeviceInfo& device, int parsed_ranges[NUM_ANCHORS]) {
    struct RangeBuffer buff_array[NUM_ANCHORS] = {};
    int index = 0;
    bool filled = false;

    while (true) {
        // Returns a list of distances
        if (!get_raw_ranges(device, parsed_ranges)) {
            delay(10);
            continue;
        };        

        for (int i = 0; i < NUM_ANCHORS; i++)  {
            buff_array[i].buffer[index] = parsed_ranges[i];
        }

        index = (index + 1) % BUFF_SIZE; 

        // At this point, we know we've gone through the whole buffer
        if (index == 0)
            filled = true;

        if (!filled)
            continue;

        bool can_return = true;

        // Reset max, min, and sum after every buffer update
        for (int i = 0; i < NUM_ANCHORS; i++) {
            int max = buff_array[i].buffer[0];
            int min = buff_array[i].buffer[0];
            int sum = 0;

            for (int j = 0; j < BUFF_SIZE; j++) {
                int v = buff_array[i].buffer[j];
                
                if (v > max) max = v;
                if (v < min) min = v;

                if (v > 0 && sum > LONG_MAX - v) {
                    send_wifi_data(device, "WARNING: sum approaching LONG_MAX");
                }
                if (v < 0 && sum < LONG_MIN - v) {
                    send_wifi_data(device, "WARNING: sum approaching LONG_MIN");
                }

                sum += v;
            }

            if (max - min > STABILITY_THRESHOLD) {
                can_return = false;
                break;
            }

            // Calculate the mean distance to the ith anchor
            parsed_ranges[i] = sum / BUFF_SIZE;

        }

        // can_return is true when all distances have converged
        if (can_return) return;

        delay(5);
    }
}

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


void parse_range(String message, int ranges[]) {
    int i = 0;

    // Convert Arduino String to C string
    char buf[128];               // Make sure buffer is big enough
    message.toCharArray(buf, sizeof(buf));

    char *start = strstr(buf, "range:(");
    if (!start) return;
    // Move past "range:(""
    start += 7;

    char *end = strchr(start, ')');
    if(!end) return;

    char temp[128];  // Make sure it's large enough
    size_t len = end - start;
    strncpy(temp, start, len);
    temp[len] = '\0';

    // Tokenize by comma
    char *token = strtok(temp, ",");
    while (token && i < 8) {
        ranges[i++] = atoi(token);  // Convert to int
        token = strtok(NULL, ",");
    }
}


//////////////
// BARRIERS //
//////////////

//-----------------------------------------------------------------------------

/* C: send a get_distance(x, y) string. Block until a distance is received, or 
until the timeout period is reached. If a distance is received, return it. If not,
loop again */
int get_distance(int id1, int id2) { 

    const unsigned long timeout_ms = 200;  // timeout per attempt
    String message;

    while (true) {
        // Send out a request for the distance between boards id1 and id2
        String request = "get_distance(" + String(id1) + "," + String(id2) + ")";

        send_radio_data(request, 1000, 0);

        unsigned long start_time = millis();

        while (millis() - start_time < timeout_ms) {
            // Now listen until the distance is returned
            // message = "";

            // If a message is received...
            if (read_serial(message, 0)) {
                message.trim();
                int distance = message.toInt();

                return distance;
            }

            delay(1);
        } 
    }
}

//-----------------------------------------------------------------------------

/* F: Continuously read the serial monitor until "ID,ACK" is received from each
anchor */
const int NUM_IDS = 4;  // IDs 0..3

// Returns true if all IDs have been received
bool all_received(bool received[]) {
    for (int i = 0; i < NUM_IDS; i++) {
        if (!received[i]) return false;
    }
    return true;
}


void listen_for_ack() {
    bool received[NUM_IDS] = {false, false, false, false};
    String message;

    unsigned long start_time = millis();
    const unsigned long timeout_ms = 5000;  // total listen timeout

    while (!all_received(received) && millis() - start_time < timeout_ms) {
        if (read_serial(message, 0)) {  // assume this is non-blocking
            message.trim();
            // Message format: "ID,ACK"
            int commaIndex = message.indexOf(',');
            if (commaIndex > 0) {
                int id = message.substring(0, commaIndex).toInt();
                if (id >= 0 && id < NUM_IDS) {
                    received[id] = true;
                    Serial.print("Received ACK from ID: ");
                    Serial.println(id);
                }
            }
        }
        delay(1); // small delay to prevent tight CPU loop
    }

    if (all_received(received)) {
        Serial.println("All ACKs received!");
    } else {
        Serial.println("Timeout reached before receiving all ACKs.");
    }
}

//-----------------------------------------------------------------------------

/* Q: Continuously read the serial monitor until "SUCCESS" is received, but only 
from device x */
wait_for_measurement(int id) {}

//-----------------------------------------------------------------------------

/* N/O: Block until "calibration complete" is received, or, until 
"get_distance(x, y)" is received where x or y matches the device's ID. Determine
if it was x or y that matched. */
measurement_loop() {}

//-----------------------------------------------------------------------------

/* The tag needs to recieve a go-ahead from the laptop*/
receive_wifi_data(ok_signal);