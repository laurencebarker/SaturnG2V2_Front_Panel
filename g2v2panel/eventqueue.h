/////////////////////////////////////////////////////////////////////////
//
// Saturn G2 front panel controller sketch by Laurence Barker G8NJJ
// this sketch provides a knob and switch interface through I2C
// copyright (c) Laurence Barker G8NJJ 2023
//
// the code is written for an Arduino Nano Every module
//
// eventqueue.h
// header for output event queue operations
/////////////////////////////////////////////////////////////////////////
#ifndef __eventqueue_h
#define __eventqueue_h

#include <Arduino.h>



//
// enum for the event types
//
enum EEventType 
{
  eNoEvent,                           // no event present
  eVFOStep,                           // VFO encoder steps
  eEncoderStep,                       // ordinary encoder steps
  eButtonPress,                       // pushbutton press
  eButtonLongpress,                   // pushbutton long press
  eButtonRelease                      // pushbutton release
};

//
// function to add an event to the output queue
//
void AddEvent2Q(EEventType Event, byte EventData);



#endif