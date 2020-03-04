#ifndef ESP32_OS_CONFIG_H
#define ESP32_OS_CONFIG_H
//Configurable parameter
#define SAMPLING_TIME_US 1000//1000 is 1000us (1000Hz), 100000 is 100000us (10Hz)
#define USE_WIFI 0
#define USE_MQTT 0
#define DEBUG_MODE 1 //use baudrate 115200
#define DEFAULT_BAUDRATE_FOR_DEBUG_MODE 115200
#define USE_DEFAULT_BAUDRATE 0


#define BUTTON_HYSTERESIS_US 20000
#define CURTAIN_HYSTERESIS_US 100000

//Pattern
#define MAX_SUB_PATTERN_LENGTH 32
#define NUMBER_OF_PATTERN 8

//Button Gesture
#define WAIT_FOR_BUTTON_GESTURE 80000//in microseconds. Any button event will wait for other buttons event with this period before trigger the event. Example: the button #0 is pressed, and #1 button is pressed 0.4 seconds later (wait time 0.5 seconds) OS will report as both button pressed together. But if wait time expire, the OS will ignore other button until every buttons are released.
#define BUTTON_GESTURE_LONG_PRESS_TRIGGER_PERIOD 1000000//if button gesture is pressed longer than this period, it will start trigger event every AUTOMATIC_TRIGGER_PERIOD
#define BUTTON_GESTURE_AUTOMATIC_TRIGGER_PERIOD 100000

#define STRAIN_GAUGE_READ_MODE 25 //25-27 pulse mode
#define STRAIN_GAUGE_BYPASS 0//0 for no bypass, 1 for first sensor bypass, 2 for second sensor bypass, 4 for third sensor bypass, 8 for fourth sensor bypass, 3 for first & second bypass

//Communication parameter
#define DEFAULT_BAUDRATE BAUDRATE_921600
#define DEFAULT_WIFI_SSID "WIFI_SSID"
#define DEFAULT_WIFI_PASSWORD "WIFI_PASS"
#define DEFAULT_MQTT_SERVER_NAME "127.0.0.1"
#define DEFAULT_MQTT_SERVER_PORT 1883
#define DEFAULT_MQTT_USERNAME "user"
#define DEFAULT_MQTT_PASSWORD "password"
#define DEFAULT_MQTT_CLIENT_NAME "Esp32Client"
#define DEFAULT_MQTT_TX_TOPIC "ESP32_TX"
#define DEFAULT_MQTT_RX_TOPIC "ESP32_RX"

//Other parameter
//negative time constant will disable filter
#define DEFAULT_STRAIN_GAUGE_LOW_PASS_FILTER_TIME_CONSTANT 0.04
#define DEFAULT_STRAIN_GAUGE_HIGH_PASS_FILTER_TIME_CONSTANT 0.04

//Non-configurable
//BAUDRATE
#define BAUDRATE_115200 0
#define BAUDRATE_230400 1
#define BAUDRATE_460800 2
#define BAUDRATE_921600 3
#endif
