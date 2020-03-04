#ifndef ESP32_OS_H
#define ESP32_OS_H
#include "ReducedMCommand.h"
#include "LowLevelLibrary.h"
#include "Esp32OSConfig.h"
void Esp32OSInitialize();
void Esp32OSRun();
extern PacketBuffer mqttWifiSerial;
extern ReducedMCommand serialRMC;
extern void mainLoop();//Must be declared in .ino
extern void realtimeFunction();
void enterCriticalSection();
void exitCriticalSection();
void waitUntilRealtimeFunctionFinished();
void runCommand(ReducedMCommand* rmc);
extern void runExternalCommand(ReducedMCommand* rmc);
extern char externalCommandIndex[];
extern void mqttCallback(char* topic, byte* payload, unsigned int length);
void onMqttWifiFlush();
//Hysteresis
class StateFilter{
  public:
    int hysteresisLength = 0;//in loops
    int counter = 0;
    bool currentState = false;
    bool getFilteredState(bool rawState) volatile;
};
extern volatile StateFilter buttonFilter[4];
extern volatile StateFilter curtainFilter[3];
//Timer interrupt
class InputState{
  public:
    bool buttonPressed[4];
    bool oldButtonPressed[4];
    byte buttonStatus = 0;
    byte oldButtonStatus = 0;
    int buttonGestureTimerCounter;
    int buttonGestureAutoTriggerCounter;
    byte buttonGestureTimerCounterFlag = 0;
    byte buttonGestureAutoTriggerCounterFlag = 0;
    bool readyToAcceptGesture = false;
    byte currentGesture = 0;
    bool curtainState[3] = {0, 0, 0};
    bool oldCurtainState[3] = {0, 0, 0};
    bool curtainRiseFlag[3] = {0, 0, 0};
    bool curtainFallFlag[3] = {0, 0, 0};
    int realtimeCountsOfLastCurtainEvent[3];
    int realtimeCounterOfCurtain[3];
    int32_t strainGaugeValue[4];
    float strainGaugeFilteredValue[4];
    bool strainGaugeReadFlag;
    uint16_t potentiometerValue;
    uint16_t accelerometerValue;
};
extern volatile InputState realtimeInputState;//use within realtime function only
extern volatile InputState inputState;//Should be copied from realtimeInputState inside realtime's critical section. must access in critical section from any function.
extern volatile InputState runtimeInputState;//should be copied from inputState inside non-realtime critical section.

class OutputState{
  public:
    byte rgbLedState[3] = {0, 0, 0};
    byte lightLedState = 0;
    byte buzzerState = 0;
    void setRGBLED(byte red, byte green, byte blue) volatile;
    void setLightLED(byte led) volatile;
    void setBuzzer(byte buzzer) volatile;
};
extern volatile OutputState hiddenOutputState;//saved old state
extern volatile OutputState realtimeOutputState;//manipulate this inside realtime function only
extern volatile OutputState outputState;//before enter user realtime calculation(), os will copy this into realtimeOutputState
extern volatile OutputState runtimeOutputState;//after main loop, os will copy this into realtime output state, inside run-time critical section
extern volatile OutputState hiddenRuntimeOutputState;//use to check if runtimeOutputState has changed by user.
void updateOutputState();


//Flag
extern void onButtonEvent(byte buttonGesture, bool automaticTriggering);//call in runtime
extern void onCurtainChange(int number, bool isRising, int timeSinceLastEvent);//call in runtime
extern void onStrainGaugeRead();
extern void setupSignalPattern();

//Utility
#define RUN_ONCE 1
#define RUN_INFINITY 0xFFFFFFFF
class CountdownTimer{
  public:
    void setFunction(void (*_function)(volatile CountdownTimer* cdt)) volatile;
    void setFunction(void (*_function)(volatile CountdownTimer* cdt), bool _runInRealtime) volatile;
    void setTimer(uint32_t timeInUS) volatile;//in microseconds
    void setNumberOfRuns(uint32_t _numberOfRuns) volatile;//set to 0 to stop, 0xFFFFFFFF for infinity
    void start() volatile;//run using predefined setNumberOfRuns
    void start(uint32_t _numberOfRuns) volatile;//start with _numberOfRuns without changing predefined numberOfRuns
    void startImmediately() volatile;
    void startImmediately(uint32_t _numberOfRuns) volatile;
    void pause() volatile;
    void resume() volatile;
    void stop() volatile;
    uint32_t getNumberOfRunsLeft() volatile;
    bool isRunning() volatile;
    void onRealtime() volatile;
    void onRuntime() volatile;
  private:
    void (*functionToRun)(volatile CountdownTimer* cdt);
    uint32_t numberOfRuns = 0;
    uint32_t numberOfRunsLeft = 0;
    int32_t maxCounter;
    int32_t counter;
    bool state = false;
    bool runInRealtime = false;
    int callingBuffer = 0;
    
};
void registerCountdownTimer(volatile CountdownTimer* cdt);
extern std::vector<volatile CountdownTimer*> cdtVector;

//Utility, BIT plot
void plotInt(Stream* stream, int intToPlot);
void plotFloat(Stream* stream, float floatToPlot);

//Utility Signal Pattern
void addMultipleValueToVector(std::vector<byte> &byteVector, byte value, int numberToAdd);
void addLinearValueToVector(std::vector<byte> &byteVector, int initialValue, int lastValue, int numberToAdd);
class Pattern{
  public:
    void setPatternLength(unsigned int _patternLength);
    void setPatternStepsTime(unsigned int patternTime);
    void setPatternLoops(unsigned int _patternLoops);
    std::vector<byte> redSubPatternToUse;
    std::vector<byte> greenSubPatternToUse;
    std::vector<byte> blueSubPatternToUse;
    std::vector<byte> buzzerSubPatternToUse;
    void realtimeRun();
    void start();
    void start(unsigned int _patternLoops);//calling this will not overwrite patternLoops set by setPatternLoops. Except that patternLoops is zero or not set.
    void stop();
  private:
    volatile unsigned int patternLength = MAX_SUB_PATTERN_LENGTH;//Should not more than MAX_SUB_PATTERN_LENGTH
    volatile unsigned int stepsPerPattern = 1000000/SAMPLING_TIME_US;//initial value is 1 sec
    volatile unsigned int patternLoops = 0;//0xFFFFFFFF for infinity
    volatile unsigned int realtimeCounter = 0;
    volatile unsigned int currentPatternCounter = 0;
    volatile unsigned int patternLoopsCounter = 0;
    void applyPattern();
    void stopPattern();
    void runtimeStopPattern();
};
void registerPattern(Pattern* signalPattern);
extern std::vector<Pattern*> patternVector;
//Utility Filter
class LowPassFilter{
  public:
    LowPassFilter();
    LowPassFilter(float timeConstant);
    float filter(float rawValue);
    void setTimeConstant(float timeConstant);
    float getTimeConstant();
    void reset();
  private:
    float tc;
    float rawValueWeight;
    float oldValueWeight;
    float oldValue;
    bool initialized = false;
    bool disable = true;
};
class HighPassFilter{
  public:
    HighPassFilter();
    HighPassFilter(float timeConstant);
    float filter(float rawValue);
    void setTimeConstant(float timeConstant);
    float getTimeConstant();
    void reset();
  private:
    float tc;
    float weight; 
    float oldRawValue;
    float oldFilteredValue;
    bool initialized = false;
    bool disable = true;
};
class BandPassFilter{
  public:
    BandPassFilter();
    BandPassFilter(float lowPassTimeConstant, float highPassTimeConstant);
    float filter(float rawValue);
    void setTimeConstant(float lowPassTimeConstant, float highPassTimeConstant);
    void setLowPassFilterTimeConstant(float lowPassTimeConstant);
    void setHighPassFilterTimeConstant(float highPassTimeConstant);
    float getLowPassFilterTimeConstant();
    float getHighPassFilterTimeConstant();
    void reset();
  private:
    LowPassFilter lowPassFilter;
    HighPassFilter highPassFilter;
};


#endif
