#include "ReducedMCommand.h"
#include "Esp32OSConfig.h"//get DEBUG_MODE
void ReducedMCommand::clearHasParameter(){
  hasM = false;
  hasP = false;
  hasS = false;
  hasString = false;
}
ReducedMCommand::ReducedMCommand(Stream* _sourceStream, int _sourceIndex){
  source = _sourceStream;
  sourceIndex = _sourceIndex;
}
bool ReducedMCommand::insertToString(char c){
  if(c == 0 || charReceived == (STRING_BUFFER_LENGTH-1)){
    *mainStringBufferPointer = 0;
    mainStringBufferPointer++;
    charReceived++;
    processMCode();
    return true;
  }else{
    *mainStringBufferPointer = c;
    mainStringBufferPointer++;
    charReceived++;
    return false;
  }
}
void ReducedMCommand::processMCode(){
  #if DEBUG_MODE == 1
  Serial.println("Processing M Code");
  #endif
  char* processingCharPointer;
  processingCharPointer = mainStringBuffer;
  while(*processingCharPointer != 0){
    #if DEBUG_MODE == 1
    Serial.println((int)*processingCharPointer);
    #endif
    switch(*processingCharPointer){
      case 'M':
        #if DEBUG_MODE == 1
        Serial.println("Code M");
        #endif
        processingCharPointer++;
        if(isInteger(*processingCharPointer)){
          hasM = true;
          M = strtol(processingCharPointer, &processingCharPointer, 10);
          #if DEBUG_MODE == 1
          Serial.print("Code M");
          Serial.println(M);
          #endif
        }
        break;
      case 'P':
        processingCharPointer++;
        if(isInteger(processingCharPointer[0]) || (processingCharPointer[0] == '-' && isInteger(processingCharPointer[1]))){
          hasP = true;
          P = strtol(processingCharPointer, &processingCharPointer, 10);
        }
        break;
      case 'S':
        processingCharPointer++;
        if(isInteger(processingCharPointer[0]) || (processingCharPointer[0] == '-' && (isInteger(processingCharPointer[1]) || (processingCharPointer[1] == '.' && isInteger(processingCharPointer[2])) ) ) ){
          hasS = true;
          S = strtod(processingCharPointer, &processingCharPointer);
        }
        break;
      case '"':
        processingCharPointer++;
        stringBufferPointer = stringBuffer;
        while(*processingCharPointer != 0 && *processingCharPointer != '"'){
          *stringBufferPointer = *processingCharPointer;
          stringBufferPointer++;
          processingCharPointer++;
        }
        if(*processingCharPointer == '"'){
          *stringBufferPointer = 0;
          stringBufferPointer++;
          processingCharPointer++;
          hasString = true;
        }
        break;
      default:
        processingCharPointer++;
        break;
    }
  }
  mainStringBufferPointer = mainStringBuffer;
  charReceived = 0;
}
bool isInteger(char c){
  return (c>='0' && c<='9')? true:false;
}
