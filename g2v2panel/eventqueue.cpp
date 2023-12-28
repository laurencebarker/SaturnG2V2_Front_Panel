/////////////////////////////////////////////////////////////////////////
//
// Saturn G2 front panel controller sketch by Laurence Barker G8NJJ
// this sketch provides a knob and switch interface through I2C
// copyright (c) Laurence Barker G8NJJ 2023
//
// the code is written for an Arduino Nano Every module
//
// eventqueue.h
// output event queue operations
/////////////////////////////////////////////////////////////////////////


#include "eventqueue.h"




//
// function to add an event to the output queue
//
void AddEvent2Q(EEventType Event, byte EventData)
{
  switch(Event)
  {
    case eNoEvent:                           // no event present
      Serial.println("BUG: event processed but no event present");
      break;

    case eVFOStep:                           // VFO encoder steps
      Serial.print("VFO encoder: steps = ");
      Serial.println(EventData);
      break;
      
    case eEncoderStep:                       // ordinary encoder steps
      Serial.print("dual encoder: number = ");
      Serial.print(EventData >> 4);
      Serial.print(" steps = ");
      Serial.println(EventData & 0xF);
      break;
      
    case eButtonPress:                       // pushbutton press
      Serial.print("button press: button code = ");
      Serial.println(EventData);
      break;
      
    case eButtonLongpress:                   // pushbutton long press
      Serial.print("button long press: button code = ");
      Serial.println(EventData);
      break;
      
    case eButtonRelease:                     // pushbutton release
      Serial.print("button release: button code = ");
      Serial.println(EventData);
      break;

  }
}