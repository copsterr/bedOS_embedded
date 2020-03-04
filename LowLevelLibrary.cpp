#include "LowLevelLibrary.h"
#include "Arduino.h"
int strainGaugeDataPin[4] = {STRAIN_GAUGE_DATA_0_PIN, STRAIN_GAUGE_DATA_1_PIN, STRAIN_GAUGE_DATA_2_PIN, STRAIN_GAUGE_DATA_3_PIN};
int buttonPin[4] = {BUTTON_0_PIN, BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN};
int rgbLedPin[3] = {RGB_LED_R_PIN, RGB_LED_G_PIN, RGB_LED_B_PIN};
int curtainPin[3] = {LIGHT_CURTAIN_LEFT_PIN, LIGHT_CURTAIN_MID_PIN, LIGHT_CURTAIN_RIGHT_PIN};
void lowLevelInitialize(){//initialize HSPI SS, Touch, ADC, DAC
  pinMode(STRAIN_GAUGE_DATA_0_PIN, INPUT);
  pinMode(STRAIN_GAUGE_DATA_1_PIN, INPUT); 
  pinMode(STRAIN_GAUGE_DATA_2_PIN, INPUT);
  pinMode(STRAIN_GAUGE_DATA_3_PIN, INPUT);
  pinMode(STRAIN_GAUGE_CLK_PIN, OUTPUT);
  digitalWrite(STRAIN_GAUGE_CLK_PIN, LOW);
  pinMode(BUTTON_0_PIN, BUTTON_TYPE);
  pinMode(BUTTON_1_PIN, BUTTON_TYPE);
  pinMode(BUTTON_2_PIN, BUTTON_TYPE);
  pinMode(BUTTON_3_PIN, BUTTON_TYPE);
  pinMode(LIGHT_CURTAIN_LEFT_PIN, LIGHT_CURTAIN_TYPE);
  pinMode(LIGHT_CURTAIN_MID_PIN, LIGHT_CURTAIN_TYPE);
  pinMode(LIGHT_CURTAIN_RIGHT_PIN, LIGHT_CURTAIN_TYPE);
  //PWM
  ledcAttachPin(RGB_LED_R_PIN, RGB_LED_R_PWM_CHANNEL);
  ledcAttachPin(RGB_LED_G_PIN, RGB_LED_G_PWM_CHANNEL);
  ledcAttachPin(RGB_LED_B_PIN, RGB_LED_B_PWM_CHANNEL);
  ledcAttachPin(LIGHT_LED_PIN, LIGHT_LED_PWM_CHANNEL);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
  ledcSetup(RGB_LED_R_PWM_CHANNEL, 1250, 8);
  ledcSetup(RGB_LED_G_PWM_CHANNEL, 1250, 8);
  ledcSetup(RGB_LED_B_PWM_CHANNEL, 1250, 8);
  ledcSetup(LIGHT_LED_PWM_CHANNEL, 1250, 8);
  ledcSetup(BUZZER_PWM_CHANNEL, 2500, 8);
  driveRLed(0);
  driveGLed(0);
  driveBLed(0);
  driveLightLed(50);
  driveBuzzer(0);
  pinMode(POTENTIOMETER_PIN, INPUT);
  analogRead(POTENTIOMETER_PIN);
  pinMode(ACCELEROMETER_PIN, INPUT);
  analogRead(ACCELEROMETER_PIN);
}
void driveRLed(byte val){
#if RGB_LED_R_INVERT == 0
  ledcWrite(RGB_LED_R_PWM_CHANNEL, val);
#else
  ledcWrite(RGB_LED_R_PWM_CHANNEL, 255-val);
#endif
}
void driveGLed(byte val){
#if RGB_LED_G_INVERT == 0
  ledcWrite(RGB_LED_G_PWM_CHANNEL, val);
#else
  ledcWrite(RGB_LED_G_PWM_CHANNEL, 255-val);
#endif
}
void driveBLed(byte val){
#if RGB_LED_B_INVERT == 0
  ledcWrite(RGB_LED_B_PWM_CHANNEL, val);
#else
  ledcWrite(RGB_LED_B_PWM_CHANNEL, 255-val);
#endif
}
void driveLightLed(byte val){
#if LIGHT_LED_INVERT == 0
  ledcWrite(LIGHT_LED_PWM_CHANNEL, val);
#else
  ledcWrite(LIGHT_LED_PWM_CHANNEL, 255-val);
#endif
}
void driveBuzzer(byte val){
#if BUZZER_INVERT == 0
  ledcWrite(BUZZER_PWM_CHANNEL, val);
#else
  ledcWrite(BUZZER_PWM_CHANNEL, 255-val);
#endif
}
bool readStrainGaugeValue(int32_t strainGaugeValue[], int strainGaugeReadMode, int sensorBypass){
  //if(!digitalRead(STRAIN_GAUGE_DATA_0_PIN) && !digitalRead(STRAIN_GAUGE_DATA_0_PIN) && !digitalRead(STRAIN_GAUGE_DATA_0_PIN) && !digitalRead(STRAIN_GAUGE_DATA_0_PIN)){//Strain gauges are ready to read
  if((!digitalRead(STRAIN_GAUGE_DATA_0_PIN) || ((sensorBypass>>0)&1)) && (!digitalRead(STRAIN_GAUGE_DATA_1_PIN) || ((sensorBypass>>1)&1)) && (!digitalRead(STRAIN_GAUGE_DATA_2_PIN) || ((sensorBypass>>2)&1)) && (!digitalRead(STRAIN_GAUGE_DATA_3_PIN) || ((sensorBypass>>3)&1))){
    for(int j = 0; j<4; j++){
      strainGaugeValue[j] = 0;
    }
    for(int i = 0; i<24; i++){
      digitalWrite(STRAIN_GAUGE_CLK_PIN, HIGH);
      delayMicroseconds(STRAIN_GAUGE_CLOCK_DELAY);
      digitalWrite(STRAIN_GAUGE_CLK_PIN, LOW);
      delayMicroseconds(STRAIN_GAUGE_CLOCK_DELAY);
      for(int j = 0; j<4; j++){
        strainGaugeValue[j] <<= 1;
        if(digitalRead(strainGaugeDataPin[j]))
          strainGaugeValue[j]++;
      }
    }
    for(int i = 24; i<strainGaugeReadMode; i++){
      digitalWrite(STRAIN_GAUGE_CLK_PIN, HIGH);
      delayMicroseconds(STRAIN_GAUGE_CLOCK_DELAY);
      digitalWrite(STRAIN_GAUGE_CLK_PIN, LOW);
      delayMicroseconds(STRAIN_GAUGE_CLOCK_DELAY);
    }
    for(int j = 0; j<4; j++){
      if(strainGaugeValue[j] > 0x800000){
        strainGaugeValue[j] |= 0xFF000000;
      }
    }
    return true;
  }
  return false;
}
bool readPushButton(int number){
#if BUTTON_INVERT == 1
  return !(digitalRead(buttonPin[number]));
#else
  return (digitalRead(buttonPin[number]));
#endif
}
bool readLightCurtain(int number){
#if LIGHT_CURTAIN_INVERT == 1
  return !(digitalRead(curtainPin[number]));
#else
  return (digitalRead(curtainPin[number]));
#endif
}
int readAnalogPotentiometer(){
#if POTENTIOMETER_INVERT == 0
  return analogRead(POTENTIOMETER_PIN);
#else
  return 4095-analogRead(POTENTIOMETER_PIN);
#endif
}
int readAnalogAccelerometer(){
#if ACCELEROMETER_INVERT == 0
  return analogRead(ACCELEROMETER_PIN);
#else
  return 4095-analogRead(ACCELEROMETER_PIN);
#endif
}
void PacketBuffer::begin(){
  
}
void PacketBuffer::end(){
  
}
int PacketBuffer::available(){
  return bufferNotRead;
}
int PacketBuffer::peek(){
  return receivingBuffer[receivingBufferPosition];
}
int PacketBuffer::read(){
  if(bufferNotRead == 0)
    return -1;
  byte tempC = receivingBuffer[receivingBufferPosition];
  receivingBufferPosition++;
  if(receivingBufferPosition >= MQTT_BUFFER_SIZE){
    receivingBufferPosition = 0;
  }
  bufferNotRead--;
  return tempC;
}
void PacketBuffer::insertToReceivingBuffer(uint8_t c){
  if(bufferNotRead >= MQTT_BUFFER_SIZE)
    return;
  receivingBuffer[receivingBufferPositionForPolling] = c;
  receivingBufferPositionForPolling++;
  if(receivingBufferPositionForPolling >= MQTT_BUFFER_SIZE){
    receivingBufferPositionForPolling = 0;
  }
  bufferNotRead++;
}
void PacketBuffer::flush(){
  if(onFlush != NULL)
    onFlush();
}
size_t PacketBuffer::write(uint8_t c){
  if(bufferNotSent >= MQTT_BUFFER_SIZE)
    return 0;
  sendingBuffer[sendingBufferPosition] = c;
  sendingBufferPosition++;
  if(sendingBufferPosition >= MQTT_BUFFER_SIZE){
    sendingBufferPosition = 0;
  }
  bufferNotSent++;
  return 1;
}
uint8_t PacketBuffer::readFromSendingBuffer(){
  if(bufferNotSent == 0)
    return 0;
  byte tempC = sendingBuffer[sendingBufferPositionForPolling];
  sendingBufferPositionForPolling++;
  if(sendingBufferPositionForPolling >= MQTT_BUFFER_SIZE){
    sendingBufferPositionForPolling = 0;
  }
  bufferNotSent--;
  return tempC;
  
}
