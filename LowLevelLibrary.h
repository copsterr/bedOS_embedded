//This is for ESP32
 /* Function
 * INPUT: 11 (36, 39, 34, 35, 25, 26, 27, 04, 18, 19, 21), first 4 is for DATA of strain gauge, other 4 are for buttons, last 3 for light curtain
 * OUTPUT: 6(05, 13, 14, 15, 22, 23) 1 CLK for strain gauge, 3 for LED RGB, 1 for LED light, 1 for buzzer
 * ADC: 2(32, 33) for Poten and Accel
 * UART: 1 (Serial0) same as USB
 * WIFI: 1 (UDP, TCP, MQTT)
 * EEPROM: 512 bytes
 * 
 * Unused pin
 * GPIO 12: boot fail if pulled high
 * 
 */

#ifndef LOW_LEVEL_LIBRARY_H
#define LOW_LEVEL_LIBRARY_H
#include <PubSubClient.h>
#define STRAIN_GAUGE_DATA_0_PIN 36
#define STRAIN_GAUGE_DATA_1_PIN 39
#define STRAIN_GAUGE_DATA_2_PIN 34
#define STRAIN_GAUGE_DATA_3_PIN 35
#define STRAIN_GAUGE_CLK_PIN 05
#define BUTTON_0_PIN 25
#define BUTTON_1_PIN 26
#define BUTTON_2_PIN 27
#define BUTTON_3_PIN 04
#define LIGHT_CURTAIN_LEFT_PIN 18
#define LIGHT_CURTAIN_MID_PIN 19
#define LIGHT_CURTAIN_RIGHT_PIN 21
#define RGB_LED_R_PIN 14
#define RGB_LED_G_PIN 15
#define RGB_LED_B_PIN 13
#define LIGHT_LED_PIN 22
#define BUZZER_PIN 23
#define POTENTIOMETER_PIN 32
#define ACCELEROMETER_PIN 33

#define RGB_LED_R_PWM_CHANNEL 1
#define RGB_LED_G_PWM_CHANNEL 2
#define RGB_LED_B_PWM_CHANNEL 3
#define LIGHT_LED_PWM_CHANNEL 4
#define BUZZER_PWM_CHANNEL 5

#define BUTTON_INVERT 1
#define BUTTON_TYPE INPUT_PULLUP
#define LIGHT_CURTAIN_INVERT 0
#define LIGHT_CURTAIN_TYPE INPUT_PULLUP
#define RGB_LED_R_INVERT 0
#define RGB_LED_G_INVERT 0
#define RGB_LED_B_INVERT 0
#define LIGHT_LED_INVERT 0
#define BUZZER_INVERT 0
#define POTENTIOMETER_INVERT 1
#define ACCELEROMETER_INVERT 1
#define STRAIN_GAUGE_CLOCK_DELAY 1//us


void lowLevelInitialize();//initialize StrainGaugeReader, Buttons, RGB_LED, LED light, Buzzer, poten, Accel
extern int strainGaugeDataPin[4];
extern int buttonPin[4];
extern int rgbLedPin[3];
extern int curtainPin[3];

void driveRLed(byte val);
void driveGLed(byte val);
void driveBLed(byte val);
void driveLightLed(byte val);
void driveBuzzer(byte val);
bool readStrainGaugeValue(int32_t strainGaugeValue[], int strainGaugeReadMode, int sensorBypass);
bool readPushButton(int number);
bool readLightCurtain(int number);
int readAnalogPotentiometer();
int readAnalogAccelerometer();

#define MQTT_BUFFER_SIZE 1024

class PacketBuffer : public Stream{
  public:
    void begin();
    void end();
    virtual int available();
    virtual int peek();
    virtual int read();
    void insertToReceivingBuffer(uint8_t c);
    
    virtual void flush();
    void (*onFlush)();
    virtual size_t write(uint8_t);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write; // pull in write(str) and write(buf, size) from Print
    uint8_t readFromSendingBuffer();
    byte sendingBuffer[MQTT_BUFFER_SIZE];
    byte receivingBuffer[MQTT_BUFFER_SIZE];
    unsigned int sendingBufferPosition = 0;
    unsigned int receivingBufferPosition = 0;
    unsigned int bufferNotSent = 0;
    unsigned int bufferNotRead = 0;
    unsigned long sendingBufferPositionForPolling = 0;
    unsigned long receivingBufferPositionForPolling = 0;
};


#endif
