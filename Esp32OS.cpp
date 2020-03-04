#include "Esp32OS.h"
#include "Esp32OSConfig.h"
#include "LowLevelLibrary.h"
#include "EEPROMConfig.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "ReducedMCommand.h"
#include "Arduino.h"
//M Command Index
char commandIndex[] = "M Command Index \n\r\
M0:Echo \n\r\
M1:Reboot \n\r\
M100:Report All Parameter, use P1 to clear all parameter \n\r\
M101:Set Serial Baudrate \n\r\
M102: Set WIFI SSID with String \n\r\
M103 Set WIFI Password with String \n\r\
M104: Set MQTT Server Name with String \n\r\
M105: Set MQTT Server Port with P \n\r\
M106: Set MQTT Username with String \n\r\
M107: Set MQTT Password with String \n\r\
M108: Set MQTT Client Name with String \n\r\
M109: Set MQTT Tx Topic with String \n\r\
M110: Set MQTT Rx Topic with String \n\r\
M201: Set Strain Gauge Low Pass Filter Time Constant with S, optional P \n\r\
M202: Set Strain Gauge High Pass Filter Time Constant with S, optional P \n\r";

char externalCommandIndex[] __attribute__((weak)) = "External Command Index not available \n\r";

//Strain gauge filter
BandPassFilter strainGaugeFilter[4];
//Config
//Reboot
RTC_DATA_ATTR int rebootSource = -1;
bool rebootViaMqtt = false;

ReducedMCommand serialRMC(&Serial, 0);
#if USE_MQTT == 1
WiFiClient client;
PubSubClient mqtt(client);
PacketBuffer mqttWifiSerial;
ReducedMCommand mqttWifiRMC(&mqttWifiSerial, 1);
#endif

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Timer interrupt
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t OSSemaphore = NULL;
volatile StateFilter buttonFilter[4];
volatile StateFilter curtainFilter[3];
volatile InputState realtimeInputState;
volatile InputState inputState;
volatile InputState runtimeInputState;
volatile OutputState hiddenOutputState;//saved old state
volatile OutputState realtimeOutputState;//manipulate this inside realtime function only
volatile OutputState outputState;//before enter user realtime calculation(), os will copy this into realtimeOutputState
volatile OutputState runtimeOutputState;//after main loop, os will copy this into realtime output state, inside run-time critical section
volatile OutputState hiddenRuntimeOutputState;//use to check if runtimeOutputState has changed by user.

volatile bool timerFlag = false;
#if USE_MQTT == 1
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  #if DEBUG_MODE == 1
  Serial.println("MQTT Message received");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload of ");
  Serial.print(length);
  Serial.print(" bytes: ");
  for(int i = 0; i<length; i++){
    Serial.print(payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  #endif
  if(String(topic) == eepromConfig.getMqttRxTopic()){
    for(int i = 0; i<length; i++){
      mqttWifiSerial.insertToReceivingBuffer((char)payload[i]);
    }
    if(payload[length-1] > 0)
      mqttWifiSerial.insertToReceivingBuffer(0);
  }
}
void onMqttWifiFlush(){
  if(mqtt.connected()){
    if(mqttWifiSerial.bufferNotSent > 0){
      mqtt.beginPublish(eepromConfig.getMqttTxTopic(), mqttWifiSerial.bufferNotSent+1, true);
      while(mqttWifiSerial.bufferNotSent > 0){
        mqtt.write(mqttWifiSerial.readFromSendingBuffer());
      }
      mqtt.write(0);
      mqtt.endPublish();
    }
  }else{
    while(mqttWifiSerial.bufferNotSent > 0){
      mqttWifiSerial.readFromSendingBuffer();
    }
  }
}
#endif
void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
  timerFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}
void onTimerFlag(){
  //do things
  //get all variable
  enterCriticalSection();
  //Copy state
  memcpy((byte*)&realtimeInputState, (byte*)&inputState, sizeof(InputState));//copy sync to realtime
  memcpy((byte*)&realtimeOutputState, (byte*)&outputState, sizeof(OutputState));//copy sync to realtime
  exitCriticalSection();
  realtimeInputState.potentiometerValue = readAnalogPotentiometer();
  realtimeInputState.accelerometerValue = readAnalogAccelerometer();
  realtimeInputState.oldButtonStatus = realtimeInputState.buttonStatus;
  realtimeInputState.buttonStatus = 0;
  for(int i = 0; i<4; i++){
    realtimeInputState.oldButtonPressed[i] = realtimeInputState.buttonPressed[i];
    realtimeInputState.buttonPressed[i] = buttonFilter[i].getFilteredState(readPushButton(i));
    realtimeInputState.buttonStatus += realtimeInputState.buttonPressed[i]? (1<<i):0;
  }
  if(realtimeInputState.buttonStatus == 0){
    realtimeInputState.readyToAcceptGesture = true;
    realtimeInputState.buttonGestureTimerCounter = 0;
    realtimeInputState.buttonGestureAutoTriggerCounter = 0;
  }else{//if buttonStatus != 0
    //Check if there is no falling
    if((realtimeInputState.oldButtonStatus & ~realtimeInputState.buttonStatus) > 0){//Some button are released before gesture event happens, ignore gesture
      realtimeInputState.readyToAcceptGesture = false;//Ignore until all buttons are released
    }
    if(realtimeInputState.readyToAcceptGesture){
      if(realtimeInputState.buttonGestureTimerCounter < WAIT_FOR_BUTTON_GESTURE){
        if(realtimeInputState.buttonStatus != realtimeInputState.oldButtonStatus){//more button is pressed
          realtimeInputState.buttonGestureTimerCounterFlag = 0;//reset flag
          realtimeInputState.buttonGestureAutoTriggerCounterFlag = 0;//reset flag
          realtimeInputState.currentGesture = realtimeInputState.buttonStatus;//set gesture
          realtimeInputState.buttonGestureTimerCounter = 0;//restart counter
          realtimeInputState.buttonGestureAutoTriggerCounter = 0;
        }else{
          realtimeInputState.buttonGestureTimerCounter += SAMPLING_TIME_US;
          if(realtimeInputState.buttonGestureTimerCounter >= WAIT_FOR_BUTTON_GESTURE){//call event
            realtimeInputState.buttonGestureTimerCounterFlag++;
          }
        }
      }else{
        realtimeInputState.buttonGestureAutoTriggerCounter += SAMPLING_TIME_US;
        if(realtimeInputState.buttonGestureAutoTriggerCounter >= BUTTON_GESTURE_LONG_PRESS_TRIGGER_PERIOD){
          realtimeInputState.buttonGestureAutoTriggerCounter -= BUTTON_GESTURE_AUTOMATIC_TRIGGER_PERIOD;
          realtimeInputState.buttonGestureAutoTriggerCounterFlag++;
        }
      }
    }
  }
  for(int i = 0; i<3; i++){
    realtimeInputState.oldCurtainState[i] = realtimeInputState.curtainState[i];
    realtimeInputState.curtainState[i] = curtainFilter[i].getFilteredState(readLightCurtain(i));
    if(realtimeInputState.curtainState[i] == realtimeInputState.oldCurtainState[i])
      realtimeInputState.realtimeCounterOfCurtain[i]++;
    else{
      realtimeInputState.realtimeCountsOfLastCurtainEvent[i] = realtimeInputState.realtimeCounterOfCurtain[i];
      realtimeInputState.realtimeCounterOfCurtain[i] = 0;
    }
    realtimeInputState.curtainRiseFlag[i] = (realtimeInputState.curtainState[i] && !realtimeInputState.oldCurtainState[i])? true:realtimeInputState.curtainRiseFlag[i];
    realtimeInputState.curtainFallFlag[i] = (!realtimeInputState.curtainState[i] && realtimeInputState.oldCurtainState[i])? true:realtimeInputState.curtainFallFlag[i];
  }
  realtimeInputState.strainGaugeReadFlag = realtimeInputState.strainGaugeReadFlag || readStrainGaugeValue((int32_t*)realtimeInputState.strainGaugeValue, STRAIN_GAUGE_READ_MODE, STRAIN_GAUGE_BYPASS);
  for(int i = 0; i<4; i++){
    realtimeInputState.strainGaugeFilteredValue[i] = strainGaugeFilter[i].filter((float)realtimeInputState.strainGaugeValue[i]);
  }
  //do other calculation
  realtimeFunction();
  //do signal pattern
  for(auto cdtIt = cdtVector.begin(); cdtIt != cdtVector.end(); cdtIt++){
    (*cdtIt)->onRealtime();
  }
  for(auto patternIt = patternVector.begin(); patternIt != patternVector.end(); patternIt++){
    (*patternIt)->realtimeRun();
  }
  //drive output
  if(realtimeOutputState.rgbLedState[0] != hiddenOutputState.rgbLedState[0]){
    hiddenOutputState.rgbLedState[0] = realtimeOutputState.rgbLedState[0];
    driveRLed(hiddenOutputState.rgbLedState[0]);
  }
  if(realtimeOutputState.rgbLedState[1] != hiddenOutputState.rgbLedState[1]){
    hiddenOutputState.rgbLedState[1] = realtimeOutputState.rgbLedState[1];
    driveGLed(hiddenOutputState.rgbLedState[1]);
  }
  if(realtimeOutputState.rgbLedState[2] != hiddenOutputState.rgbLedState[2]){
    hiddenOutputState.rgbLedState[2] = realtimeOutputState.rgbLedState[2];
    driveBLed(hiddenOutputState.rgbLedState[2]);
  }
  if(realtimeOutputState.lightLedState != hiddenOutputState.lightLedState){
    hiddenOutputState.lightLedState = realtimeOutputState.lightLedState;
    driveLightLed(hiddenOutputState.lightLedState);
  }
  if(realtimeOutputState.buzzerState != hiddenOutputState.buzzerState){
    hiddenOutputState.buzzerState = realtimeOutputState.buzzerState;
    driveBuzzer(hiddenOutputState.buzzerState);
  }
  
  enterCriticalSection();
  //Copy state
  memcpy((byte*)&inputState, (byte*)&realtimeInputState, sizeof(InputState));//copy from realtime to sync
  memcpy((byte*)&outputState, (byte*)&realtimeOutputState, sizeof(OutputState));
  exitCriticalSection();
}
void onTimerFlagTask(void *pvParameters){
  volatile bool tempFlag;
  while(true){
    portENTER_CRITICAL_ISR(&timerMux);
    tempFlag = timerFlag;
    portEXIT_CRITICAL_ISR(&timerMux);
    if(tempFlag){
      portENTER_CRITICAL_ISR(&timerMux);
      timerFlag = false;
      tempFlag = timerFlag;
      portEXIT_CRITICAL_ISR(&timerMux);
      onTimerFlag();
    }
  }
}
void enterCriticalSection(){
  if(xSemaphoreTake(OSSemaphore, 100) != pdTRUE){
    Serial.println("Taking OSSemaphore failed");
  }
  //portENTER_CRITICAL_ISR(&timerMux);
}
void exitCriticalSection(){
  //portEXIT_CRITICAL_ISR(&timerMux);
  xSemaphoreGive(OSSemaphore);
}
void waitForRealtimeFunctionFinished(){
  
}
#if (USE_WIFI || USE_MQTT) == 1
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          //should initialize mqtt and udp here
          #if DEBUG_MODE == 1
            Serial.println("Wifi is connected.");
          #endif
          #if USE_MQTT == 1
          if(!mqtt.connected()){
            if(mqtt.connect(eepromConfig.getMqttClientName(), eepromConfig.getMqttUsername(), eepromConfig.getMqttPassword())){
              #if DEBUG_MODE == 1
                Serial.println("MQTT is connected.");
              #endif
              mqtt.subscribe(eepromConfig.getMqttRxTopic());
              //mqtt.publish(eepromConfig.getMqttTxTopic(), "Hello.");
              //mqtt.subscribe(eepromConfig.getMqttRxTopic());
              if(rebootViaMqtt){
                rebootViaMqtt = false;
                mqtt.publish(eepromConfig.getMqttTxTopic(), "Rebooted.", true);
              }else{
                mqtt.publish(eepromConfig.getMqttTxTopic(), "Hello.");
              }
              
            }else{
              #if DEBUG_MODE == 1
                Serial.println("Cannot connect to MQTT server.");
              #endif
            }
          }
          #endif
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          break;
    }
}
#endif
void Esp32OSInitialize(){
  #if DEBUG_MODE == 1
  Serial.begin(DEFAULT_BAUDRATE_FOR_DEBUG_MODE);
  #endif
  //set touch threshold
  for(int i = 0; i<4; i++){
    buttonFilter[i].hysteresisLength = BUTTON_HYSTERESIS_US/SAMPLING_TIME_US;
  }
  for(int i = 0; i<3; i++){
    curtainFilter[i].hysteresisLength = CURTAIN_HYSTERESIS_US/SAMPLING_TIME_US;
  }
  eepromConfig.init();
  #if DEBUG_MODE == 1
  Serial.println("EEPROM init finished.");
  eepromConfig.reportAllParameter(&Serial);
  #endif
  OSSemaphore = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(onTimerFlagTask, "onTimerFlagTask", 1024, NULL, 1, NULL, 1);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, SAMPLING_TIME_US, true);
  setupSignalPattern();
  #if DEBUG_MODE == 1
  Serial.println("Signal pattern setup finished.");
  #endif
  for(int i = 0; i<4; i++){
    strainGaugeFilter[i].setTimeConstant(eepromConfig.getStrainGaugeLowPassFilterTimeConstant(), eepromConfig.getStrainGaugeHighPassFilterTimeConstant());
  }
  #if DEBUG_MODE == 1
  Serial.println("Set strain gauge filter... finished");
  #endif
  
  #if USE_WIFI == 1
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  #if DEBUG_MODE == 1
  Serial.println("WIFI Event linked");
  #endif
  WiFi.begin(eepromConfig.getWifiSSID(), eepromConfig.getWifiPassword());
  #if DEBUG_MODE == 1
  Serial.println("WIFI begin() called.");
  Serial.print("SSID: ");
  Serial.println(eepromConfig.getWifiSSID());
  Serial.print("Password: ");
  Serial.println(eepromConfig.getWifiPassword());
  #endif
  #endif
  #if DEBUG_MODE == 0
  Serial.begin(eepromConfig.getBaudrate());
  #endif
  #if USE_MQTT == 1
  mqtt.setServer(eepromConfig.getMqttServerName(), eepromConfig.getMqttServerPort());
  mqtt.setCallback(mqttCallback);
  mqttWifiSerial.onFlush = onMqttWifiFlush;
  #endif
  switch(rebootSource){
    case 0://Serial
      Serial.println("Rebooted.");
      break;
    case 1://mqtt
      rebootViaMqtt = true;
      break;
  }
  rebootSource = -1;
  delay(1000);
  lowLevelInitialize();
  #if DEBUG_MODE == 1
  Serial.println("LowLevelInitialize() finished.");
  #endif
  timerAlarmEnable(timer);
  #if DEBUG_MODE == 1
  Serial.println("Timer start");
  #endif
  
}
void Esp32OSRun(){
  //copy to runtime
  enterCriticalSection();
  memcpy((byte*)&runtimeInputState, (byte*)&inputState, sizeof(InputState));
  memcpy((byte*)&runtimeOutputState, (byte*)&outputState, sizeof(OutputState));
  exitCriticalSection();
  byte tempC;
  while(Serial.available()){
    tempC = Serial.read();
    #if DEBUG_MODE == 1
    
    Serial.print(tempC);
    
    #endif
    serialRMC.insertToString(tempC);
    if(tempC == 10){
      #if DEBUG_MODE == 1
      Serial.println("Auto Insert Null");
      #endif
      serialRMC.insertToString((char) 0);
      #if DEBUG_MODE == 1
      Serial.println("Auto Insert Null");
      #endif
      runCommand(&serialRMC);
      #if DEBUG_MODE == 1
      Serial.println("Command Ran");
      #endif
    }else if(tempC == 0){
      #if DEBUG_MODE == 1
      Serial.println("Null received");
      #endif
      runCommand(&serialRMC);
      #if DEBUG_MODE == 1
      Serial.println("Command Ran");
      #endif
    }
    
  }
  #if USE_MQTT == 1
  mqtt.loop();
  while(mqttWifiSerial.available()){
    if(mqttWifiRMC.insertToString(mqttWifiSerial.read()))
      runCommand(&mqttWifiRMC);
  }
  #endif
  
  
  while(runtimeInputState.buttonGestureTimerCounterFlag > 0){
    runtimeInputState.buttonGestureTimerCounterFlag--;
    enterCriticalSection();
    if(inputState.buttonGestureTimerCounterFlag > 0)//make sure that this is not go down below 0
      inputState.buttonGestureTimerCounterFlag--;
    exitCriticalSection();
    onButtonEvent(runtimeInputState.currentGesture, false);
  }
  while(runtimeInputState.buttonGestureAutoTriggerCounterFlag > 0){
    runtimeInputState.buttonGestureAutoTriggerCounterFlag--;
    enterCriticalSection();
    if(inputState.buttonGestureAutoTriggerCounterFlag > 0)//make sure that this is not go down below 0
      inputState.buttonGestureAutoTriggerCounterFlag--;
    exitCriticalSection();
    onButtonEvent(runtimeInputState.currentGesture, true);
  }
  for(int i = 0; i<3; i++){//check Curtain Flag
    if(runtimeInputState.curtainRiseFlag[i]){
      enterCriticalSection();
      runtimeInputState.curtainRiseFlag[i] = false;
      inputState.curtainRiseFlag[i] = false;
      exitCriticalSection();
      onCurtainChange(i, 1, (runtimeInputState.realtimeCountsOfLastCurtainEvent[i]*SAMPLING_TIME_US)/1000);//rising
    }
    if(runtimeInputState.curtainFallFlag[i]){
      enterCriticalSection();
      runtimeInputState.curtainFallFlag[i] = false;
      inputState.curtainFallFlag[i] = false;
      exitCriticalSection();
      onCurtainChange(i, 0, (runtimeInputState.realtimeCountsOfLastCurtainEvent[i]*SAMPLING_TIME_US)/1000);//Falling
    }
  }
  if(runtimeInputState.strainGaugeReadFlag){
    enterCriticalSection();
    runtimeInputState.strainGaugeReadFlag = false;
    inputState.strainGaugeReadFlag = false;
    exitCriticalSection();
    onStrainGaugeRead();
  }
  mainLoop();
  for(auto cdtIt = cdtVector.begin(); cdtIt != cdtVector.end(); cdtIt++){
    (*cdtIt)->onRuntime();
  }
  updateOutputState();

}
void runCommand(ReducedMCommand* rmc){
  #if DEBUG_MODE == 1
  Serial.println("Run Command");
  #endif
  if(!rmc->hasM){
    rmc->source->println("Invalid Command.");
    rmc->source->write(commandIndex, sizeof(commandIndex)-1);
    rmc->source->write(externalCommandIndex, sizeof(externalCommandIndex)-1);
    rmc->source->flush();
    return;
  }
  switch(rmc->M){
    case 0://Echo
      rmc->source->print("Echo:");
      if(rmc->hasP){
        rmc->source->print(" P");
        rmc->source->print(rmc->P);
      }
      if(rmc->hasS){
        rmc->source->print(" S");
        rmc->source->print(rmc->S);
      }
      if(rmc->hasString){
        rmc->source->print(' ');
        rmc->source->print('"');
        rmc->source->print(rmc->stringBuffer);
        rmc->source->print('"');
      }
      rmc->source->println();
      //rmc->source->write(commandIndex, sizeof(commandIndex)-1);
      rmc->source->flush();
      break;
    case 1:
      rmc->source->println("Rebooting in 5 seconds");
      rmc->source->flush();
      rebootSource = rmc->sourceIndex;
      esp_sleep_enable_timer_wakeup(5000000);
      delay(5000);
      esp_deep_sleep_start();
      break;
    case 100:
      if(rmc->hasP && rmc->P == 1){
        rmc->source->println("Clear all parameter.");
        rmc->source->flush();
        eepromConfig.clearAllParameter();
      }else{
        eepromConfig.reportAllParameter(rmc->source);
        rmc->source->println("Call M100 P1 to clear all parameter");
        rmc->source->flush();
      }
      break;
    //From 101 onward, set parameter
    case 101:
      if(rmc->hasP){
        rmc->source->println("Set Baudrate");
        rmc->source->flush();
        eepromConfig.setBaudrate(rmc->P);
        eepromConfig.reportBaudrate(rmc->source);
      }else{
        eepromConfig.reportBaudrate(rmc->source);
        rmc->source->println("Syntax: M101 Px");
        rmc->source->flush();
        rmc->source->println("x is baudrate");
        rmc->source->flush();
        rmc->source->println("Available Baudrate: ");
        rmc->source->flush();
        rmc->source->println("0: 115200");
        rmc->source->flush();
        rmc->source->println("1: 230400");
        rmc->source->flush();
        rmc->source->println("2: 460800");
        rmc->source->flush();
        rmc->source->println("3: 921600");
        rmc->source->flush();
      }
      break;
    case 102:
      if(rmc->hasString){
        rmc->source->println("Set WIFI SSID.");
        rmc->source->flush();
        eepromConfig.setWifiSSID(rmc->stringBuffer);
        eepromConfig.reportWifiSSID(rmc->source);
      }else{
        eepromConfig.reportWifiSSID(rmc->source);
      }
      break;
    case 103:
      if(rmc->hasString){
        rmc->source->println("Set WIFI Password.");
        rmc->source->flush();
        eepromConfig.setWifiPassword(rmc->stringBuffer);
        eepromConfig.reportWifiPassword(rmc->source);
      }else{
        eepromConfig.reportWifiPassword(rmc->source);
      }
      break;
    case 104:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Server Name.");
        rmc->source->flush();
        eepromConfig.setMqttServerName(rmc->stringBuffer);
        eepromConfig.reportMqttServerName(rmc->source);
      }else{
        eepromConfig.reportMqttServerName(rmc->source);
      }
      break;
    case 105:
      if(rmc->hasP){
        rmc->source->println("Set MQTT Server Port.");
        rmc->source->flush();
        eepromConfig.setMqttServerPort(rmc->P);
        eepromConfig.reportMqttServerPort(rmc->source);
      }else{
        eepromConfig.reportMqttServerPort(rmc->source);
      }
      break;
    case 106:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Username.");
        rmc->source->flush();
        eepromConfig.setMqttUsername(rmc->stringBuffer);
        eepromConfig.reportMqttUsername(rmc->source);
      }else{
        eepromConfig.reportMqttUsername(rmc->source);
      }
      break;
    case 107:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Password.");
        rmc->source->flush();
        eepromConfig.setMqttPassword(rmc->stringBuffer);
        eepromConfig.reportMqttPassword(rmc->source);
      }else{
        eepromConfig.reportMqttPassword(rmc->source);
      }
      break;
    case 108:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Client Name.");
        rmc->source->flush();
        eepromConfig.setMqttClientName(rmc->stringBuffer);
        eepromConfig.reportMqttClientName(rmc->source);
      }else{
        eepromConfig.reportMqttClientName(rmc->source);
      }
      break;
    case 109:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Tx Topic.");
        rmc->source->flush();
        eepromConfig.setMqttTxTopic(rmc->stringBuffer);
        eepromConfig.reportMqttTxTopic(rmc->source);
      }else{
        eepromConfig.reportMqttTxTopic(rmc->source);
      }
      break;
    case 110:
      if(rmc->hasString){
        rmc->source->println("Set MQTT Rx Topic.");
        rmc->source->flush();
        eepromConfig.setMqttRxTopic(rmc->stringBuffer);
        eepromConfig.reportMqttRxTopic(rmc->source);
      }else{
        eepromConfig.reportMqttRxTopic(rmc->source);
      }
      break;
    case 201:
      if(rmc->hasS){
        if(rmc->S < 0){
          rmc->source->println("Disable Low Pass Filter for Strain Gauge.");
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setLowPassFilterTimeConstant(rmc->S);
          }
        }else{
          rmc->source->print("Set Low Pass Filter for Strain Gauge to ");
          rmc->source->println(rmc->S);
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setLowPassFilterTimeConstant(rmc->S);
          }
        }
        if(rmc->hasP && (rmc->P == 1)){
          rmc->source->println("Saved");
          eepromConfig.setStrainGaugeLowPassFilterTimeConstant(rmc->S);
        }else{
          rmc->source->println("Add P1 to save parameter");
        }
      }else if(rmc->hasP){
        if(rmc->P == 1){
          eepromConfig.setStrainGaugeLowPassFilterTimeConstant(strainGaugeFilter[0].getLowPassFilterTimeConstant());
          rmc->source->println("Parameter Saved");
        }else if(rmc->P == 2){
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setLowPassFilterTimeConstant(eepromConfig.getStrainGaugeLowPassFilterTimeConstant());
          }
          eepromConfig.reportStrainGaugeLowPassFilterTimeConstant(rmc->source);
          rmc->source->println("Parameter Loaded");
        }
      }else{
        eepromConfig.reportStrainGaugeLowPassFilterTimeConstant(rmc->source);
        rmc->source->println("Syntax: M201 Sx, x is time constant, negative to disable filter. Add P1 to save parameter, Add P2 to load parameter");
      }
      rmc->source->flush();
      break;
    case 202:
      if(rmc->hasS){
        if(rmc->S < 0){
          rmc->source->println("Disable High Pass Filter for Strain Gauge.");
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setHighPassFilterTimeConstant(rmc->S);
          }
        }else{
          rmc->source->print("Set High Pass Filter for Strain Gauge to ");
          rmc->source->println(rmc->S);
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setHighPassFilterTimeConstant(rmc->S);
          }
        }
        if(rmc->hasP && (rmc->P == 1)){
          rmc->source->println("Saved");
          eepromConfig.setStrainGaugeHighPassFilterTimeConstant(rmc->S);
        }else{
          rmc->source->println("Add P1 to save parameter");
        }
      }else if(rmc->hasP){
        if(rmc->P == 1){
          eepromConfig.setStrainGaugeHighPassFilterTimeConstant(strainGaugeFilter[0].getHighPassFilterTimeConstant());
          rmc->source->println("Parameter Saved");
        }else if(rmc->P == 2){
          for(int i = 0; i<4; i++){
            strainGaugeFilter[i].setHighPassFilterTimeConstant(eepromConfig.getStrainGaugeHighPassFilterTimeConstant());
          }
          eepromConfig.reportStrainGaugeHighPassFilterTimeConstant(rmc->source);
          rmc->source->println("Parameter Loaded");
        }
      }else{
        eepromConfig.reportStrainGaugeHighPassFilterTimeConstant(rmc->source);
        rmc->source->println("Syntax: M202 Sx, x is time constant, negative to disable filter. Add P1 to save parameter, Add P2 to load parameter");
      }
      rmc->source->flush();
      break;
    default:
      runExternalCommand(rmc);
      break;
  }
  rmc->clearHasParameter();
  updateOutputState();
}

bool StateFilter::getFilteredState(bool rawState) volatile{
  if(rawState == currentState){//refill counter
    counter = hysteresisLength;
  }else{//rawState != currentState
    counter--;
    if(counter <= 0){//Accept change
      counter = hysteresisLength;
      currentState = rawState;
    }
  }
  return currentState;
}
void OutputState::setRGBLED(byte red, byte green, byte blue) volatile{
  rgbLedState[0] = red;
  rgbLedState[1] = green;
  rgbLedState[2] = blue;
}
void OutputState::setLightLED(byte led) volatile{
  lightLedState = led;
}
void OutputState::setBuzzer(byte buzzer) volatile{
  buzzerState = buzzer;
}
void updateOutputState(){
  for(int i = 0; i<3; i++){
    if(hiddenRuntimeOutputState.rgbLedState[i] != runtimeOutputState.rgbLedState[i]){
      enterCriticalSection();
      outputState.rgbLedState[i] = runtimeOutputState.rgbLedState[i];
      exitCriticalSection();
    }
  }
  if(hiddenRuntimeOutputState.lightLedState != runtimeOutputState.lightLedState){
    enterCriticalSection();
    outputState.lightLedState = runtimeOutputState.lightLedState;
    exitCriticalSection();
  }
  if(hiddenRuntimeOutputState.buzzerState != runtimeOutputState.buzzerState){
    enterCriticalSection();
    outputState.buzzerState = runtimeOutputState.buzzerState;
    exitCriticalSection();
  }
  memcpy((byte*)&hiddenRuntimeOutputState, (byte*)&runtimeOutputState, sizeof(OutputState));
}
//Utility
//CountdownTimer
std::vector<volatile CountdownTimer*> cdtVector;
void registerCountdownTimer(volatile CountdownTimer* cdt){
  cdtVector.push_back(cdt);
}
void CountdownTimer::setFunction(void (*_function)(volatile CountdownTimer* cdt)) volatile{
  enterCriticalSection();
  functionToRun = _function;
  runInRealtime = false;
  exitCriticalSection();
}
void CountdownTimer::setFunction(void (*_function)(volatile CountdownTimer* cdt), bool _runInRealtime) volatile{
  enterCriticalSection();
  functionToRun = _function;
  runInRealtime = _runInRealtime;
  exitCriticalSection();
}
void CountdownTimer::setTimer(uint32_t timeInUS) volatile{//in microseconds
  enterCriticalSection();
  maxCounter = timeInUS/SAMPLING_TIME_US;
  if(maxCounter < 1)
    maxCounter = 1;
  exitCriticalSection();
}
void CountdownTimer::setNumberOfRuns(uint32_t _numberOfRuns) volatile{//set to 0 to stop, 0xFFFFFFFF for infinity
  enterCriticalSection();
  numberOfRuns = _numberOfRuns;
  exitCriticalSection();
}
void CountdownTimer::start() volatile{
  enterCriticalSection();
  if(numberOfRuns < 1){
    exitCriticalSection();
    return;
  }
  numberOfRunsLeft = numberOfRuns;
  counter = maxCounter;
  state = true;
  exitCriticalSection();
}
void CountdownTimer::start(uint32_t _numberOfRuns) volatile{
  enterCriticalSection();
  numberOfRunsLeft = _numberOfRuns;
  if(numberOfRunsLeft < 1){
    exitCriticalSection();
    return;
  }
  counter = maxCounter;
  state = true;
  exitCriticalSection();
}
void CountdownTimer::startImmediately() volatile{
  enterCriticalSection();
  if(numberOfRuns < 1){
    exitCriticalSection();
    return;
  }
  numberOfRunsLeft = numberOfRuns;
  counter = 0;
  state = true;
  exitCriticalSection();
}
void CountdownTimer::startImmediately(uint32_t _numberOfRuns) volatile{
  enterCriticalSection();
  numberOfRunsLeft = _numberOfRuns;
  if(numberOfRunsLeft < 1){
    exitCriticalSection();
    return;
  }
  counter = 0;
  state = true;
  exitCriticalSection();
}
void CountdownTimer::pause() volatile{
  enterCriticalSection();
  state = false;
  exitCriticalSection();
}
void CountdownTimer::resume() volatile{
  enterCriticalSection();
  if(numberOfRunsLeft > 0)
    state = true;
  exitCriticalSection();
}
void CountdownTimer::stop() volatile{
  enterCriticalSection();
  state = false;
  numberOfRunsLeft = 0;
  exitCriticalSection();
}
uint32_t CountdownTimer::getNumberOfRunsLeft() volatile{
  uint32_t tempVal;
  enterCriticalSection();
  tempVal = numberOfRunsLeft;
  if(callingBuffer > 0 && (tempVal + callingBuffer < 0xFFFFFFFF)){
    tempVal = tempVal + callingBuffer;
  }
  exitCriticalSection();
  return tempVal;
}
bool CountdownTimer::isRunning() volatile{
  bool tempVal;
  enterCriticalSection();
  tempVal = state;
  exitCriticalSection();
  return tempVal;
}
void CountdownTimer::onRealtime() volatile{
  enterCriticalSection();
  if(!state){
    exitCriticalSection();
    return;
  }
  if(numberOfRunsLeft > 0)
    counter--;
  if(counter <= 0){
    if(numberOfRunsLeft < RUN_INFINITY)
      numberOfRunsLeft--;
    if(runInRealtime){
      exitCriticalSection();
      functionToRun(this);
      enterCriticalSection();
    }else{
      callingBuffer++;
    }
    if(numberOfRunsLeft > 0)
      counter += maxCounter;
    else
      state = false;
  }
  exitCriticalSection();
}
void CountdownTimer::onRuntime() volatile{
  enterCriticalSection();
  if(runInRealtime){
    exitCriticalSection();
    return;
  }
  if(callingBuffer > 0){
    callingBuffer--;
    exitCriticalSection();
    functionToRun(this);
    enterCriticalSection();
  }
  exitCriticalSection();
}
void plotInt(Stream* stream, int intToPlot){
  byte *byteToPlot;
  byteToPlot = (byte*)&intToPlot;
  for(int i = 0; i<sizeof(int); i++){
    stream->write(*byteToPlot);
    byteToPlot++;
  }
}
void plotFloat(Stream* stream, float floatToPlot){
  byte *byteToPlot;
  byteToPlot = (byte*)&floatToPlot;
  for(int i = 0; i<sizeof(float); i++){
    stream->write(*byteToPlot);
    byteToPlot++;
  }
}
void addMultipleValueToVector(std::vector<byte> &byteVector, byte value, int numberToAdd){
  for(int i = 0; i<numberToAdd; i++){
    byteVector.push_back(value);
  }
}
void addLinearValueToVector(std::vector<byte> &byteVector, int initialValue, int lastValue, int numberToAdd){
  for(int i = 0; i<numberToAdd; i++){
    byteVector.push_back((initialValue*(numberToAdd - i) + lastValue*(i))/numberToAdd);
  }
}
std::vector<Pattern*> patternVector;
void registerPattern(Pattern* signalPattern){
  patternVector.push_back(signalPattern);
}
void Pattern::setPatternLength(unsigned int _patternLength){
  if(redSubPatternToUse.size() > 0){
    while(redSubPatternToUse.size() < _patternLength){
      redSubPatternToUse.push_back(0);
    }
  }
  if(greenSubPatternToUse.size() > 0){
    while(greenSubPatternToUse.size() < _patternLength){
      greenSubPatternToUse.push_back(0);
    }
  }
  if(blueSubPatternToUse.size() > 0){
    while(blueSubPatternToUse.size() < _patternLength){
      blueSubPatternToUse.push_back(0);
    }
  }
  if(buzzerSubPatternToUse.size() > 0){
    while(buzzerSubPatternToUse.size() < _patternLength){
      buzzerSubPatternToUse.push_back(0);
    }
  }
  enterCriticalSection();
  patternLength = _patternLength;
  if(patternLength > MAX_SUB_PATTERN_LENGTH)
    patternLength = MAX_SUB_PATTERN_LENGTH;
  exitCriticalSection();
}
void Pattern::setPatternStepsTime(unsigned int patternTime){
  enterCriticalSection();
  stepsPerPattern = patternTime/SAMPLING_TIME_US;
  if(stepsPerPattern == 0)
    stepsPerPattern = 1;
  exitCriticalSection();
}
void Pattern::setPatternLoops(unsigned int _patternLoops){
  enterCriticalSection();
  patternLoops = _patternLoops;
  exitCriticalSection();
}
void Pattern::start(){
  enterCriticalSection();
  runtimeStopPattern();
  realtimeCounter = 0;
  currentPatternCounter = 0;
  patternLoopsCounter = patternLoops;
  exitCriticalSection();
}
void Pattern::start(unsigned int _patternLoops){
  enterCriticalSection();
  runtimeStopPattern();
  realtimeCounter = 0;
  currentPatternCounter = 0;
  patternLoopsCounter = _patternLoops;
  if(patternLoops == 0)
    patternLoops = _patternLoops;
  exitCriticalSection();
}
void Pattern::stop(){
  enterCriticalSection();
  if(patternLoopsCounter > 0 || realtimeCounter > 0){//still running
    runtimeStopPattern();
  }
  realtimeCounter = 0;
  currentPatternCounter = patternLength;
  patternLoopsCounter = 0;
  exitCriticalSection();
}
void Pattern::realtimeRun(){
  enterCriticalSection();
  if(patternLoopsCounter == 0){
    switch(realtimeCounter){
      case 0:
        break;
      case 1:
        realtimeCounter = 0;
        stopPattern();
        break;
      default:
        realtimeCounter--;
        break;
    }
    exitCriticalSection();
    return;
  }
  if(realtimeCounter == 0){
    realtimeCounter = stepsPerPattern-1;
    applyPattern();
    currentPatternCounter++;
    if(currentPatternCounter >= patternLength){
      currentPatternCounter = 0;
      if(patternLoopsCounter < RUN_INFINITY)
        patternLoopsCounter--;
    }
  }else{
    realtimeCounter--;
  }
  exitCriticalSection();
}
void Pattern::applyPattern(){
  if(redSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[0] = redSubPatternToUse[currentPatternCounter];
  }
  if(greenSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[1] = greenSubPatternToUse[currentPatternCounter];
  }
  if(blueSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[2] = blueSubPatternToUse[currentPatternCounter];
  }
  if(buzzerSubPatternToUse.size() > 0){
    realtimeOutputState.buzzerState = buzzerSubPatternToUse[currentPatternCounter];
  }
}
void Pattern::stopPattern(){
  if(redSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[0] = 0;
  }
  if(greenSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[1] = 0;
  }
  if(blueSubPatternToUse.size() > 0){
    realtimeOutputState.rgbLedState[2] = 0;
  }
  if(buzzerSubPatternToUse.size() > 0){
    realtimeOutputState.buzzerState = 0;
  }
}
void Pattern::runtimeStopPattern(){
  if(redSubPatternToUse.size() > 0){
    outputState.rgbLedState[0] = 0;
  }
  if(greenSubPatternToUse.size() > 0){
    outputState.rgbLedState[1] = 0;
  }
  if(blueSubPatternToUse.size() > 0){
    outputState.rgbLedState[2] = 0;
  }
  if(buzzerSubPatternToUse.size() > 0){
    outputState.buzzerState = 0;
  }
}
//Utility Fliter
LowPassFilter::LowPassFilter(){
  rawValueWeight = 1.0f;
  oldValueWeight = 0.0f;
  initialized = false;
}
LowPassFilter::LowPassFilter(float timeConstant){
  setTimeConstant(timeConstant);
  initialized = false;
}
void LowPassFilter::setTimeConstant(float timeConstant){
  tc = timeConstant;
  if(timeConstant < 0){
    disable = true;
    return;
  }
  disable = false;
  float samplingTime = 0.000001f*SAMPLING_TIME_US;
  rawValueWeight = samplingTime/(timeConstant+samplingTime);
  oldValueWeight = 1.0f-rawValueWeight;
}
float LowPassFilter::getTimeConstant(){
  return tc;
}
float LowPassFilter::filter(float rawValue){
  if(!initialized){
    initialized = true;
    oldValue = rawValue;
    return oldValue;
  }
  if(disable)
    return rawValue;
  oldValue = rawValue*rawValueWeight + oldValue*oldValueWeight;
  return oldValue;
}
void LowPassFilter::reset(){
  initialized = false;
}
HighPassFilter::HighPassFilter(){
  weight = 1.0f;
  initialized = false;
}
HighPassFilter::HighPassFilter(float timeConstant){
  setTimeConstant(timeConstant);
  initialized = false;
}
void HighPassFilter::setTimeConstant(float timeConstant){
  tc = timeConstant;
  if(timeConstant < 0){
    disable = true;
    return;
  }
  disable = false;
  float samplingTime = 0.000001f*SAMPLING_TIME_US;
  weight = timeConstant/(timeConstant+samplingTime);
}
float HighPassFilter::getTimeConstant(){
  return tc;
}
float HighPassFilter::filter(float rawValue){
  if(!initialized){
    initialized = true;
    oldRawValue = rawValue;
    oldFilteredValue = rawValue;
    return oldFilteredValue;
  }
  if(disable)
    return rawValue;
  oldFilteredValue = weight*(oldFilteredValue+rawValue-oldRawValue);
  oldRawValue = rawValue;
  return oldFilteredValue;
}
void HighPassFilter::reset(){
  initialized = false;
}
BandPassFilter::BandPassFilter(){
  lowPassFilter = LowPassFilter();
  highPassFilter = HighPassFilter();
}
BandPassFilter::BandPassFilter(float lowPassTimeConstant, float highPassTimeConstant){
  lowPassFilter = LowPassFilter(lowPassTimeConstant);
  highPassFilter = HighPassFilter(highPassTimeConstant);
}
void BandPassFilter::setTimeConstant(float lowPassTimeConstant, float highPassTimeConstant){
  lowPassFilter.setTimeConstant(lowPassTimeConstant);
  highPassFilter.setTimeConstant(highPassTimeConstant);
}
void BandPassFilter::setLowPassFilterTimeConstant(float lowPassTimeConstant){
  lowPassFilter.setTimeConstant(lowPassTimeConstant);
}
void BandPassFilter::setHighPassFilterTimeConstant(float highPassTimeConstant){
  highPassFilter.setTimeConstant(highPassTimeConstant);
}
float BandPassFilter::getLowPassFilterTimeConstant(){
  return lowPassFilter.getTimeConstant();
}
float BandPassFilter::getHighPassFilterTimeConstant(){
  return highPassFilter.getTimeConstant();
}
float BandPassFilter::filter(float rawValue){
  return highPassFilter.filter(lowPassFilter.filter(rawValue));
}
void BandPassFilter::reset(){
  lowPassFilter.reset();
  highPassFilter.reset();
}
