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
#include <wire.h>
#include "led.h"
#include "iopins.h"
#include "globalinclude.h"
#include "button.h"

#define VI2CSLAVEADDR 0x15

//
// the event queue is a simple circular buffer implemented using an array.
// entries are held as 16 bit integers; event data = LSB
//
byte WritePtr;                                // entry number to write at
byte ReadPtr;                                 // entry number to read from 
unsigned int EventQ[VQUEUESIZE];              // declare event queue
unsigned int CommandWord;                     // new 2 byte command word from pi
bool GSendVersion;                            // true if commanded to send ot version not data.


//
// function to get the number of entries in the queue
//
byte GetQEntries()
{
  signed int Entries;                         // no. entries held 

  noInterrupts();
  Entries = WritePtr - ReadPtr;               // if not wrapped
  if(Entries < 0)                             //
    Entries += VQUEUESIZE;
  interrupts();
  
  return (byte)Entries;
}



//
// function to get an event from the queue
// return true if successful; entry returned into passed param
//
bool GetEventFromQ(unsigned int *Event)
{
  unsigned int Entry;                 // entry pulled from queue
  bool Success = false;
  if (GetQEntries() >= 0)
  {
    Success = true;
    *Event = EventQ[ReadPtr];
//
// update pointers; disable interrupts while testing
//
    noInterrupts();
    if(++ReadPtr >= VQUEUESIZE)
      ReadPtr = 0;
    interrupts();

  }
}



//
// function to add an event to the output queue
//
void AddEvent2Q(EEventType Event, byte EventData)
{
  unsigned int Entry;         // entry to add to queue
  byte Count;

  Entry = (unsigned int)EventData;
  Entry |= (((byte)Event) << 8);
//
// add entry to queue if there is space
//
  Count = GetQEntries();
  if(Count < VMAXQUEUEENTRIES)
  {
    EventQ[WritePtr] = Entry;
//
// update write pointer
//
    noInterrupts();
    if(++WritePtr >= VQUEUESIZE)
      WritePtr = 0;
    interrupts();
//
// set interrupt out, by enabling pin as output (data pre-set to zero)
//
  pinMode(VPINPIINTERRUPT, OUTPUT);                     // interrupt output
  }

  signed char Steps;
  // debug
  switch(Event)
  {
    case eNoEvent:                           // no event present
      Serial.println("BUG: event processed but no event present");
      break;

    case eEvVFOStep:                           // VFO encoder steps
      Steps = (signed int)EventData;
      Steps |= ((Steps&0b00001000)<<1);        // sign extend to bit 4
      Steps |= ((Steps&0b00001000)<<2);        // sign extend to bit 5
      Steps |= ((Steps&0b00001000)<<3);        // sign extend to bit 6
      Steps |= ((Steps&0b00001000)<<4);        // sign extend to bit 7
      Serial.print("VFO encoder: steps = ");
      Serial.println(Steps);
      break;
      
    case eEvEncoderStep:                       // ordinary encoder steps
      Serial.print("dual encoder: number = ");
      Serial.print(EventData >> 4);
      Serial.print(" steps = ");
      Steps = (signed int)(EventData & 0xF);
      Steps |= ((Steps&0b00001000)<<1);        // sign extend to bit 4
      Steps |= ((Steps&0b00001000)<<2);        // sign extend to bit 5
      Steps |= ((Steps&0b00001000)<<3);        // sign extend to bit 6
      Steps |= ((Steps&0b00001000)<<4);        // sign extend to bit 7
      Serial.println(Steps);
      break;
      
    case eEvButtonPress:                       // pushbutton press
      Serial.print("button press: button code = ");
      Serial.println(EventData);
      break;
      
    case eEvButtonLongpress:                   // pushbutton long press
      Serial.print("button long press: button code = ");
      Serial.println(EventData);
      break;
      
    case eEvButtonRelease:                     // pushbutton release
      Serial.print("button release: button code = ");
      Serial.println(EventData);
      break;

  }
}



//
// interrupt handler when data requested from I2C slave read
// response is 16 bits: either an event queue entry + length; or 0
//
void requestEvent() 
{
  bool Success;
  unsigned int Entries;
  unsigned int Response = 0;                  // response code to I2C
  Entries = (unsigned int)GetQEntries();
  if(GSendVersion)
    Response = (PRODUCTID << 8) | SWVERSION;
  else if(Entries)
  {
    Success = GetEventFromQ(Response);
    Response |= ((Entries & 0xF)<<12);
  }
  Wire.write(Response & 0xFF);            // respond with message of 2 bytes, low byter 1st
  Wire.write((Response >> 8) & 0xFF);     // respond with message of 2 bytes high byte
//
// clear interrupt out if last one read
//
  if(Entries <= 1)
    pinMode(VPINPIINTERRUPT, INPUT);                     // interrupt output deasserted
}


//
// interrupt handler for for I2C slave write operation
// simply store new command word for processing in normal sequence
// GSendVersion set if command is to send out versino information
// GShiftOverride set if all 11 LEDs should be set by the software
// commands are stored here; processed by 2ms tick code. 
//
void receiveEvent(int Count)
{
  byte Data;
  byte Cntr = 0;
  while(Wire.available()) // loop through all but the last
  {
    Data = Wire.read(); // receive byte as a character
    if(Cntr++ == 0)
      CommandWord = Data;
    else
      CommandWord = (CommandWord & 0xFF) | (Data << 8);
  }
  GSendVersion = (bool)((CommandWord >> 14)&1);
  GShiftOverride = (bool)((CommandWord >> 15)&1);
}

//
// initialise I2C slave
//
void InitialiseI2CSlave(void)
{
  Wire.begin(VI2CSLAVEADDR);                // join i2c bus with address 0x15
  Wire.onRequest(requestEvent);             // register event
  Wire.onReceive(receiveEvent);             // register slave event handler
}


//
// eventqueue Tick
// process any command word provided
// simply set LEDs to required state from CommandWord, when not in test mode
//
void EventQueueTick(void)
{
  byte LED;
  bool State;
  unsigned int Word;
  byte LEDCount = VMAXINDICATORS;

  if(LEDTestComplete)
  {
// if override bit not set, we don't set the two highest LED bits
//(these are controlled locally by the "shift" buttons)
//
    if(!GShiftOverride)
      LEDCount -= 2;
    Word = CommandWord;                       // get LED settings
    for(LED=0; LED < LEDCount; LED++)
    {
      State = (bool)(Word &1);                // get LED state
      SetLED(LED, State);
      Word = Word >> 1;
    }
  }
}

