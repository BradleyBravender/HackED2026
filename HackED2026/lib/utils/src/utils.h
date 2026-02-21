#ifndef UTILS_H
#define UTILS_H

////////////
// IMPORTS //
////////////

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifi_credentials.h"
#include <stdbool.h>

/////////////////////////////
// PREPROCESSOR DIRECTIVES //
/////////////////////////////

#define UWB_TAG_COUNT 64
#define SERIAL_LOG Serial // A way to talk to the user
#define RESET 16
#define IO_RXD2 18 // For the ESP32 to DW3000 module
#define IO_TXD2 17 // For the ESP32 to DW3000 module
#define I2C_SDA 39 // For the OLED display
#define I2C_SCL 38 // For the OLED display
#define LOCAL_PORT 4210
#define TARGET_PORT 4210 // My computer port

#define NUM_ANCHORS 8
#define BUFF_SIZE 5 // The number of ranges in the buffer at a time
#define STABILITY_THRESHOLD 5 // The total variance range measurements can have

enum DeviceRole { TAG = 0, ANCHOR = 1, UNINITIALIZED = 2 };

extern HardwareSerial SERIAL_AT;
extern Adafruit_SSD1306 display;


// Bundles device-specific data together for easier parameter passing.
struct DeviceInfo {
    /// @brief A unique identifier to each board. Start at 0.
    int uwb_index;
    /// @brief Choose something outside DHCP range (e.g., .50).
    IPAddress local_ip;
    /// @brief The network ID. Found via `ip route | grep default`.
    IPAddress gateway;
    /// @brief Standard subnet mask.
    IPAddress subnet;
    /// @brief My computer IP address. Found via `hostname -I`
    IPAddress target_ip;
    /// @brief Either ANCHOR, TAG, or UNINITIALIZED.
    DeviceRole current_role;
    /// @brief Each board needs its own UDP object.
    WiFiUDP udp;
};

struct RangeBuffer {
    int buffer[BUFF_SIZE];
};

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////

/// @brief Used in setup() to init WiFi, the OLED display, and the device role
/// @param device 
/// @param new_role 
void init_setup(DeviceInfo& device, DeviceRole new_role);


/// @brief Builds AT+SETCFG command.
/// @param device 
/// @return 
String config_cmd(DeviceInfo& device);


/// @brief Builds the AT+SETCAP command.
/// @return Command string for capacity settings.
String cap_cmd();


/// @brief Sends strings from one device to all devices via radio.
/// @param command 
/// @param timeout 
/// @param debug 
/// @return 
String send_radio_data(String command, const int timeout, boolean debug);


/// @brief Sends strings via UDP for wireless, computer-based logging.
/// @param device 
/// @param message 
void send_wifi_data(DeviceInfo& device, const String& message);


/// @brief Refreshes the OLED display with useful information.
/// @param device 
/// @param message 
void updateOLED(DeviceInfo& device, const String& message);


/// @brief 
/// @param device 
/// @param new_role 
/// @param message 
void set_role(DeviceInfo& device, DeviceRole new_role, const String& message);


/// @brief 
/// @param device 
/// @param delay 
void set_delay(int delay);


/// @brief Reads the serial port between the MCU and the chip for incoming messages
/// @param message 
/// @param debug 
/// @return 
bool read_serial(String& message, boolean debug);


/// @brief Parses the software version in the AT+GETVER command
/// @param msg 
/// @return 
String parse_software_version(DeviceInfo& device, String version);


// String parse_range(char[] message);

#endif
