#ifndef REDUCED_M_COMMAND_H
#define REDUCED_M_COMMAND_H
#define STRING_BUFFER_LENGTH 100
#include "Arduino.h"
class ReducedMCommand{
  public:
    ReducedMCommand(Stream* _sourceStream, int _sourceIndex);
    void clearHasParameter();
    Stream* source;
    int sourceIndex = -1;
    bool insertToString(char c);//return true if string is ready to process
    char mainStringBuffer[STRING_BUFFER_LENGTH];
    char* mainStringBufferPointer = mainStringBuffer;
    int charReceived = 0;

    //Interpreted variable
    int M;
    int P;
    float S;
    char stringBuffer[STRING_BUFFER_LENGTH];
    bool hasM = false;
    bool hasP = false;
    bool hasS = false;
    bool hasString = false;
  private:
    void processMCode();
    char* stringBufferPointer = stringBuffer;
};
bool isInteger(char c);
#endif
