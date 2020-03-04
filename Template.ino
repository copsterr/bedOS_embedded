#include "EEPROMConfig.h"
#include "EEPROM.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "LowLevelLibrary.h"
#include "Esp32OS.h"
#include "Esp32OSConfig.h"
#include "AIS_NB_BC95.h" // include AIS NB-IoT Shield Lib

#define DEBUG 1
#define NB_DUTY_CYCLE 1000000
String bedSerial = "5930123456";

/* NB-IoT Variables */
String apnName = "devkit.nb";
String serverIP = "178.128.16.9"; // Your Server IP
String serverPort = "41234"; // Your Server Port
String udpData = "HelloWorld";
AIS_NB_BC95 AISnb;
String payload = "";

/* Strain Gauge Offset Values */
int32_t s0_offset = 0;
int32_t s1_offset = 0;
int32_t s2_offset = 0;
int32_t s3_offset = 0;

/* Counter Timers */
volatile CountdownTimer transmitCdt;
uint8_t doTransmitNB = 0;

/* Calibration Flags */
uint8_t calibrate_flag = 1;


void setup() {
  // put your setup code here, to run once:
  Esp32OSInitialize();//Already initialize Serial, Wifi, MQTT

  // NB-IoT Params
  AISnb.debug = false;
  AISnb.setupDevice(serverPort);
  String ip1 = AISnb.getDeviceIP();  
  delay(1000);
  pingRESP pingR = AISnb.pingIP(serverIP);

  // register cdt timers
  registerCountdownTimer(&transmitCdt);
  transmitCdt.setFunction(transmitNB);
  transmitCdt.setTimer(NB_DUTY_CYCLE); // 1s delay
  transmitCdt.setNumberOfRuns(RUN_INFINITY); // run forever
  transmitCdt.start(); // start timer
}

void loop() {
  //do not put anycode here.
  Esp32OSRun();
}

void mainLoop(){//User main loop, automatically run after non-realtime OS
  //Only use runtimeInputState and runtimeOutputState
  //Use variable synchronization protocol to send/receive variable value between realtime and runtime
  
#if 0
  if (calibrate_flag) {
    Serial.println("############# Calibrating Sensors #############");
    int32_t acc[4] = {0};
    for (int i = 0; i < 10; i++) {
      acc[0] += runtimeInputState.strainGaugeValue[0];
      acc[1] += runtimeInputState.strainGaugeValue[1];
      acc[2] += runtimeInputState.strainGaugeValue[2];
      acc[3] += runtimeInputState.strainGaugeValue[3];
  
      Serial.print("Calibration Data Collection round: "); Serial.println(i);
      Serial.print(acc[0]); Serial.print(" ");
      Serial.print(acc[1]); Serial.print(" "); 
      Serial.print(acc[2]); Serial.print(" "); 
      Serial.print(acc[3]); Serial.println(" "); 
      
      delay(100);
    }
  
    s0_offset = (int32_t) -1*acc[0]/10;
    s1_offset = (int32_t) -1*acc[1]/10;
    s2_offset = (int32_t) -1*acc[2]/10;
    s3_offset = (int32_t) -1*acc[3]/10;
    Serial.println("Calibration Completed.");
    Serial.print("S0_offset: "); Serial.print(s0_offset);
    Serial.print("S1_offset: "); Serial.print(s1_offset);
    Serial.print("S2_offset: "); Serial.print(s2_offset);
    Serial.print("S3_offset: "); Serial.print(s3_offset);
    calibrate_flag = 0;
  }
#endif
 
  // get strain gauge values
  int32_t strain0 = runtimeInputState.strainGaugeValue[0] + s0_offset;
  int32_t strain1 = runtimeInputState.strainGaugeValue[1] + s1_offset;
  int32_t strain2 = runtimeInputState.strainGaugeValue[2] + s2_offset;
  int32_t strain3 = runtimeInputState.strainGaugeValue[3] + s3_offset;

#if DEBUG
  // prompt strain gauge values
  Serial.print("Strain Guage Vals: ");
  Serial.print(strain0); Serial.print(" ");
  Serial.print(strain1); Serial.print(" ");
  Serial.print(strain2); Serial.print(" ");
  Serial.print(strain3); Serial.println(" ");
#endif

  // transmit data every 1s
  if (doTransmitNB) {
    payload = "{\"bedSerial\":" + bedSerial +\
              ",\"s0\":" + String(strain0) +\
              ",\"s1\":" + String(strain1) +\
              ",\"s2\":" + String(strain2) +\
              ",\"s3\":" + String(strain3) + "}";
    UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, payload);
    UDPReceive resp = AISnb.waitResponse();

    doTransmitNB = 0;
  }
  
}

void realtimeFunction(){//This function is realtime
  //Only use realtimeInputState and realtimeOutputState
  //Use variable synchronization protocol to send/receive variable value between realtime and runtime
}

char externalCommandIndex[] = "External M Command Index \n\r\
No Index Available \n\r";

void runExternalCommand(ReducedMCommand* rmc){//this function is non-realtime
  //M0 is OS M Command List
  switch(rmc->M){
    default://Echo
      rmc->source->print("Invalid Command:");
      rmc->source->print(" M");
      rmc->source->print(rmc->M);
      if(rmc->hasP){
        rmc->source->print(" P");
        rmc->source->print(rmc->P);
      }
      if(rmc->hasP){
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
      rmc->source->flush();
      break;
  }
}

void onButtonEvent(byte buttonGesture, bool automaticTriggering){//This is runtime function, button gesture is bit based, automaticTriggering is false for normal event, true for automaticTriggering from long pressing.
  switch(buttonGesture){
    case B00000001://Only 1st button
      if(automaticTriggering){
      }else{
      }
      break;
    case B00000010://Only 2nd button
      if(automaticTriggering){
      }else{
      }
      break;
    case B00000100://Only 3rd button
      if(automaticTriggering){
      }else{
      }
      break;
    case B00001000://Only 4th button
      if(automaticTriggering){
      }else{
      }
      break;
    case B00001111://All 4 buttons
      if(automaticTriggering){
      }else{
      }
      break;
  }
}

void onCurtainChange(int number, bool isRising, int timeSinceLastEvent){//This is runtime function
  switch(number){
    case 0://LEFT
      if(isRising){
      }else{
      }
      break;
    case 1://MID
      if(isRising){
      }else{
      }
      break;
    case 2://RIGHT
      if(isRising){
      }else{
      }
      break;
  }
}

void onStrainGaugeRead(){
}

void setupSignalPattern(){
  //Setup example
  //This function is automatically called from OS during initialization
  //Must not called by user.
}

/* User Defined Fn ----------------------------------- */
void transmitNB(volatile CountdownTimer* cdt)
{
  /* When counter elapsed set doTransmitNB */
  doTransmitNB = 1;
}
