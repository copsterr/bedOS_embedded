//Instead of EEPROM, This library utilize Preferences.h
#ifndef EEPROMCONFIG_H
#include "Esp32OSConfig.h"
#include <Preferences.h>
#define EEPROMCONFIG_H
#define MAX_STRING_SIZE 100
#define DEFAULT_SERIAL_BAUDRATE_ON_STARTUP 0 //set this to 1 to default baudrate on startup

//Parameter Number
//Address
//Parameter

//WIFI


class EEPROMConfig{
  public:
    void init();
    void readAllParameter();
    void writeAllParameter();
    void clearAllParameter();
    void reportAllParameter(Stream* s);

    void setBaudrate(int _baudrate);
    void setWifiSSID(char* str);
    void setWifiPassword(char* str);
    void setMqttServerName(char* str);
    void setMqttServerPort(uint16_t _port);
    void setMqttUsername(char* str);
    void setMqttPassword(char* str);
    void setMqttClientName(char* str);
    void setMqttTxTopic(char* str);
    void setMqttRxTopic(char* str);
    void setStrainGaugeLowPassFilterTimeConstant(float ts);
    void setStrainGaugeHighPassFilterTimeConstant(float ts);

    void updateBaudrate();
    void updateWifiSSID();
    void updateWifiPassword();
    void updateMqttServerName();
    void updateMqttServerPort();
    void updateMqttUsername();
    void updateMqttPassword();
    void updateMqttClientName();
    void updateMqttTxTopic();
    void updateMqttRxTopic();
    void updateStrainGaugeLowPassFilterTimeConstant();
    void updateStrainGaugeHighPassFilterTimeConstant();

    int getBaudrate();
    char* getWifiSSID();
    char* getWifiPassword();
    char* getMqttServerName();
    uint16_t getMqttServerPort();
    char* getMqttUsername();
    char* getMqttPassword();
    char* getMqttClientName();
    char* getMqttTxTopic();
    char* getMqttRxTopic();
    float getStrainGaugeLowPassFilterTimeConstant();
    float getStrainGaugeHighPassFilterTimeConstant();

    void reportBaudrate(Stream* s);
    void reportWifiSSID(Stream* s);
    void reportWifiPassword(Stream* s);
    void reportMqttServerName(Stream* s);
    void reportMqttServerPort(Stream* s);
    void reportMqttUsername(Stream* s);
    void reportMqttPassword(Stream* s);
    void reportMqttClientName(Stream* s);
    void reportMqttTxTopic(Stream* s);
    void reportMqttRxTopic(Stream* s);
    void reportStrainGaugeLowPassFilterTimeConstant(Stream* s);
    void reportStrainGaugeHighPassFilterTimeConstant(Stream* s);
  private:
    int baudrate = DEFAULT_BAUDRATE;
    char wifiSSID[MAX_STRING_SIZE] = DEFAULT_WIFI_SSID;
    char wifiPassword[MAX_STRING_SIZE] = DEFAULT_WIFI_PASSWORD;
    char mqttServerName[MAX_STRING_SIZE] = DEFAULT_MQTT_SERVER_NAME;
    uint16_t mqttServerPort = DEFAULT_MQTT_SERVER_PORT;
    char mqttUsername[MAX_STRING_SIZE] = DEFAULT_MQTT_USERNAME;
    char mqttPassword[MAX_STRING_SIZE] = DEFAULT_MQTT_PASSWORD;
    char mqttClientName[MAX_STRING_SIZE] = DEFAULT_MQTT_CLIENT_NAME;
    char mqttTxTopic[MAX_STRING_SIZE] = DEFAULT_MQTT_TX_TOPIC;
    char mqttRxTopic[MAX_STRING_SIZE] = DEFAULT_MQTT_RX_TOPIC;
    float strainGaugeLowPassFilterTimeConstant = DEFAULT_STRAIN_GAUGE_LOW_PASS_FILTER_TIME_CONSTANT;
    float strainGaugeHighPassFilterTimeConstant = DEFAULT_STRAIN_GAUGE_HIGH_PASS_FILTER_TIME_CONSTANT;
};
extern EEPROMConfig eepromConfig;
extern Preferences preferences;







#endif
