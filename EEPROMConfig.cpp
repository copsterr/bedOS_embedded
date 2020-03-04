#include "EEPROMConfig.h"
#include <Preferences.h>
Preferences preferences;
EEPROMConfig eepromConfig;


void EEPROMConfig::init(){
  preferences.begin("SmartBed", false);
  readAllParameter();
}
void EEPROMConfig::readAllParameter(){
  updateBaudrate();
  updateWifiSSID();
  updateWifiPassword();
  updateMqttServerName();
  updateMqttServerPort();
  updateMqttUsername();
  updateMqttPassword();
  updateMqttClientName();
  updateMqttTxTopic();
  updateMqttRxTopic();
  updateStrainGaugeLowPassFilterTimeConstant();
  updateStrainGaugeHighPassFilterTimeConstant();
}
void EEPROMConfig::writeAllParameter(){
  preferences.putInt("Baudrate", baudrate);
  preferences.putString("WIFI SSID", wifiSSID);
  preferences.putString("WIFI Password", wifiPassword);
  preferences.putString("MQTT SN", mqttServerName);
  preferences.putUShort("MQTT SP", mqttServerPort);
  preferences.putString("MQTT UN", mqttUsername);
  preferences.putString("MQTT UP", mqttPassword);
  preferences.putString("MQTT CN", mqttClientName);
  preferences.putString("MQTT TX Topic", mqttTxTopic);
  preferences.putString("MQTT RX Topic", mqttRxTopic);
  preferences.putFloat("LPTC", strainGaugeLowPassFilterTimeConstant);
  preferences.putFloat("HPTC", strainGaugeHighPassFilterTimeConstant);
}
void EEPROMConfig::clearAllParameter(){
  preferences.clear();
}
void EEPROMConfig::reportAllParameter(Stream* s){
  readAllParameter();
  s->println("Reporting parameters: ");
  s->flush();
  reportBaudrate(s);
  reportWifiSSID(s);
  reportWifiPassword(s);
  reportMqttServerName(s);
  reportMqttServerPort(s);
  reportMqttUsername(s);
  reportMqttPassword(s);
  reportMqttClientName(s);
  reportMqttTxTopic(s);
  reportMqttRxTopic(s);
  reportStrainGaugeLowPassFilterTimeConstant(s);
  reportStrainGaugeHighPassFilterTimeConstant(s);
  s->println("End of parameters report.");
  s->flush();
}
void EEPROMConfig::setBaudrate(int _baudrate){
  preferences.putInt("Baudrate", _baudrate);
  updateBaudrate();
}
void EEPROMConfig::setWifiSSID(char* str){
  preferences.putString("WIFI SSID", str);
  updateWifiSSID();
}
void EEPROMConfig::setWifiPassword(char* str){
  preferences.putString("WIFI Password", str);
  updateWifiPassword();
}
void EEPROMConfig::setMqttServerName(char* str){
  preferences.putString("MQTT SN", str);
  updateMqttServerName();
}
void EEPROMConfig::setMqttServerPort(uint16_t _port){
  preferences.putUShort("MQTT SP", _port);
  updateMqttServerPort();
}
void EEPROMConfig::setMqttUsername(char* str){
  preferences.putString("MQTT UN", str);
  updateMqttUsername();
}
void EEPROMConfig::setMqttPassword(char* str){
  preferences.putString("MQTT UP", str);
  updateMqttPassword();
}
void EEPROMConfig::setMqttClientName(char* str){
  preferences.putString("MQTT CN", str);
  updateMqttClientName();
}
void EEPROMConfig::setMqttTxTopic(char* str){
  preferences.putString("MQTT TX Topic", str);
  updateMqttTxTopic();
}
void EEPROMConfig::setMqttRxTopic(char* str){
  preferences.putString("MQTT RX Topic", str);
  updateMqttRxTopic();
}
void EEPROMConfig::setStrainGaugeLowPassFilterTimeConstant(float ts){
  preferences.putFloat("LPTC", ts);
  updateStrainGaugeLowPassFilterTimeConstant();
}
void EEPROMConfig::setStrainGaugeHighPassFilterTimeConstant(float ts){
  preferences.putFloat("HPTC", ts);
  updateStrainGaugeHighPassFilterTimeConstant();
}
void EEPROMConfig::updateBaudrate(){
  #if USE_DEFAULT_BAUDRATE == 1
  baudrate = BAUDRATE_DEFAULT;
  #else
  baudrate = preferences.getInt("Baudrate", DEFAULT_BAUDRATE);
  #endif
}
void EEPROMConfig::updateWifiSSID(){
  preferences.getString("WIFI SSID", wifiSSID, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateWifiPassword(){
  preferences.getString("WIFI Password", wifiPassword, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttServerName(){
  preferences.getString("MQTT SN", mqttServerName, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttServerPort(){
  mqttServerPort = preferences.getUShort("MQTT SP", DEFAULT_MQTT_SERVER_PORT);
}
void EEPROMConfig::updateMqttUsername(){
  preferences.getString("MQTT UN", mqttUsername, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttPassword(){
  preferences.getString("MQTT UP", mqttPassword, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttClientName(){
  preferences.getString("MQTT CN", mqttClientName, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttTxTopic(){
  preferences.getString("MQTT TX Topic", mqttTxTopic, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateMqttRxTopic(){
  preferences.getString("MQTT RX Topic", mqttRxTopic, MAX_STRING_SIZE-1);
}
void EEPROMConfig::updateStrainGaugeLowPassFilterTimeConstant(){
  strainGaugeLowPassFilterTimeConstant = preferences.getFloat("LPTC", DEFAULT_STRAIN_GAUGE_LOW_PASS_FILTER_TIME_CONSTANT);
}
void EEPROMConfig::updateStrainGaugeHighPassFilterTimeConstant(){
  strainGaugeHighPassFilterTimeConstant = preferences.getFloat("HPTC", DEFAULT_STRAIN_GAUGE_HIGH_PASS_FILTER_TIME_CONSTANT);
}
int EEPROMConfig::getBaudrate(){
  switch(baudrate){
    case BAUDRATE_115200:
      return 115200;
      break;
    case BAUDRATE_230400:
      return 230400;
      break;
    case BAUDRATE_460800:
      return 460800;
      break;
    case BAUDRATE_921600:
      return 921600;
      break;
    default:
      return 115200;
      break;
  }
}
char* EEPROMConfig::getWifiSSID(){
  return wifiSSID;
}
char* EEPROMConfig::getWifiPassword(){
  return wifiPassword;
}
char* EEPROMConfig::getMqttServerName(){
  return mqttServerName;
}
uint16_t EEPROMConfig::getMqttServerPort(){
  return mqttServerPort;
}
char* EEPROMConfig::getMqttUsername(){
  return mqttUsername;
}
char* EEPROMConfig::getMqttPassword(){
  return mqttPassword;
}
char* EEPROMConfig::getMqttClientName(){
  return mqttClientName;
}
char* EEPROMConfig::getMqttTxTopic(){
  return mqttTxTopic;
}
char* EEPROMConfig::getMqttRxTopic(){
  return mqttRxTopic;
}
float EEPROMConfig::getStrainGaugeLowPassFilterTimeConstant(){
  return strainGaugeLowPassFilterTimeConstant;
}
float EEPROMConfig::getStrainGaugeHighPassFilterTimeConstant(){
  return strainGaugeHighPassFilterTimeConstant;
}
void EEPROMConfig::reportBaudrate(Stream* s){
  s->print("Serial Baudrate: ");
  s->println(getBaudrate());
  s->flush();
}
void EEPROMConfig::reportWifiSSID(Stream* s){
  s->print("WIFI SSID: ");
  s->println(getWifiSSID());
  s->flush();
}
void EEPROMConfig::reportWifiPassword(Stream* s){
  s->print("WIFI PASSWORD: ");
  s->println(getWifiPassword());
  s->flush();
}
void EEPROMConfig::reportMqttServerName(Stream* s){
  s->print("MQTT Server Name: ");
  s->println(getMqttServerName());
  s->flush();
}
void EEPROMConfig::reportMqttServerPort(Stream* s){
  s->print("MQTT Server Port: ");
  s->println(getMqttServerPort());
  s->flush();
}
void EEPROMConfig::reportMqttUsername(Stream* s){
  s->print("MQTT Username: ");
  s->println(getMqttUsername());
  s->flush();
}
void EEPROMConfig::reportMqttPassword(Stream* s){
  s->print("MQTT Password: ");
  s->println(getMqttPassword());
  s->flush();
}
void EEPROMConfig::reportMqttClientName(Stream* s){
  s->print("MQTT Client Name: ");
  s->println(getMqttClientName());
  s->flush();
}
void EEPROMConfig::reportMqttTxTopic(Stream* s){
  s->print("MQTT TX Topic: ");
  s->println(getMqttTxTopic());
  s->flush();
}
void EEPROMConfig::reportMqttRxTopic(Stream* s){
  s->print("MQTT RX Topic: ");
  s->println(getMqttRxTopic());
  s->flush();
}
void EEPROMConfig::reportStrainGaugeLowPassFilterTimeConstant(Stream* s){
  s->print("Strain Gauge Low Pass Filter Time Constant: ");
  s->println(getStrainGaugeLowPassFilterTimeConstant());
  s->flush();
}
void EEPROMConfig::reportStrainGaugeHighPassFilterTimeConstant(Stream* s){
  s->print("Strain Gauge High Pass Filter Time Constant: ");
  s->println(getStrainGaugeHighPassFilterTimeConstant());
  s->flush();
}
