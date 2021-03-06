/*

  NANOKEYBOARD
  A 1 octave midi keyboard from a TOY
  Copyright (C) 2017/2018 by The KikGen labs.

  MAIN FILE - ARDUINO SKETCH
  
  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.

   Licence : MIT.
*/

#include <HardwareSerial.h>
#include <SoftwareSerial.h>


// When using Sotware serial 
//#define RX 2
//#define TX 3
//SoftwareSerial * midiSerial = new SoftwareSerial(RX, TX); // RX, TX;

HardwareSerial * midiSerial = &Serial ;


// Keys rows and columns to scan (pin no)
uint8_t columnLines[] {13,5,6,7,8,9};
uint8_t rowLines[] {12,11,10,4};

// Midi keyboard C root key
#define ROOTKEY 0x30        // C3

// Buttons used for shift function
#define SHIFT_BUTTON1 -1
#define SHIFT_BUTTON2 -2

// Values assigned to keys
// Positive keys = midi note offset from ROOTKEY
// Negative key = buttons
// 0xFF : not affected.
int keyValues[4][6]={{0  ,1  ,2   ,3  ,4   ,5 }, 
                     {6  ,7  ,8   ,9  ,10  ,11},
                     {12 ,-3 ,-4  ,-5 ,-6  ,-7},
                     {255,255,255 ,255,-1  ,-2}
                    };
// Key current states. 1 = pressed. 
uint8_t keyStates[4][6]={ {0,0,0,0,0,0}, 
                          {0,0,0,0,0,0},
                          {0,0,0,0,0,0},
                          {0,0,0,0,0,0}
                        };

// Midi channels from keyboard keys
// -1 = nothing
int midiChannelKeyMap[]={0,-1,1,-1,2,3,-1,4,-1,5,-1,6,7};

bool shiftButton1Holded = false;
bool shiftButton2Holded = false;

// Current keyboard parameters
uint8_t currentMidiChannel =0;
int currentTranspose = 0;
uint8_t currentVelocity  = 110;
uint8_t noteOnTable[128];

/////////////////////////////////////////////////////////////////
// SETUP
/////////////////////////////////////////////////////////////////
void setup() {

  int r,c  ;
  
  midiSerial->begin(31250);
  for ( r = 0 ; r< sizeof(rowLines) ; r++ ) { pinMode(rowLines[r],OUTPUT);digitalWrite(columnLines[c],HIGH) ;}
  for ( c = 0 ; c< sizeof(columnLines) ; c++ ) { pinMode(columnLines[c],INPUT_PULLUP) ;  }
  
  memset(noteOnTable,0,sizeof(noteOnTable));
    
//utility1();
         
}
/////////////////////////////////////////////////////////////////
// Software reset 
/////////////////////////////////////////////////////////////////
void(* softReset) (void) = 0; //declare reset function @ address 0

/////////////////////////////////////////////////////////////////
// UTILITY - SCANNING KEYS
// --------------------------------------------------------------
// To use with the software serial as hardware serial
// is used to display results
/////////////////////////////////////////////////////////////////

void utility1(){
  
  int r,c  ;
  while (1) {

        //for ( r = 0 ; r< sizeof(rowLines) ; r++ ) { Serial.print (rowLines[r]);Serial.print(" : ");Serial.print(digitalRead(rowLines[r]));Serial.print(" - "); }
       
        for ( c = 0 ; c< sizeof(columnLines) ; c++ ) {                                         
              for ( r = 0 ; r< sizeof(rowLines) ; r++ ) {                           
                digitalWrite(rowLines[r],LOW);
                if ( digitalRead(columnLines[c]) == LOW ) keyStates[r][c] = 1; else keyStates[r][c] = 0;
                digitalWrite(rowLines[r],HIGH);
              }
        }
        
        for ( r = 0 ; r< sizeof(rowLines) ; r++ ) {                                    
            for ( c = 0 ; c< sizeof(columnLines) ; c++ ) { 
              Serial.print(columnLines[c]);Serial.print("/");Serial.print(rowLines[r]);Serial.print(" : ");Serial.print(keyStates[r][c]);Serial.print(" - "); 
            }
        }                    
        Serial.println("");
  }
}

/////////////////////////////////////////////////////////////////
// Send a confirmation note when changing params
/////////////////////////////////////////////////////////////////
void sendConfirmNote(uint8_t note,uint8_t velocity) {
     midiSerial->write(0x90+currentMidiChannel);midiSerial->write(note);midiSerial->write(velocity);
     delay(100);
     midiSerial->write(0x80+currentMidiChannel);midiSerial->write(note);midiSerial->write(velocity);
}

/////////////////////////////////////////////////////////////////
// Panic mode
// --------------------------------------------------------------
// reset everything MIDI related
/////////////////////////////////////////////////////////////////
void panicMode() {

    for ( uint8_t c = 0 ; c<=15 ; c++ ) {
        for ( uint8_t n = 0 ; n <= 127 ; n++ ) {
           midiSerial->write( 0x80 + c );midiSerial->write(n);midiSerial->write(64);
           noteOnTable[n] = 0;         
        }      
    }
  
}

/////////////////////////////////////////////////////////////////
// Process a note or a button event ON / OFF
// --------------------------------------------------------------
// 
/////////////////////////////////////////////////////////////////
void processEvent(int value,bool isOn){

 uint8_t command = currentMidiChannel + ( isOn ? 0x90 : 0x80 );     
 
 if (value == SHIFT_BUTTON1) { shiftButton1Holded = isOn; }
 if (value == SHIFT_BUTTON2) { shiftButton2Holded = isOn; }

 // Buttons
 if ( value < 0 ) {
  
    if (isOn && shiftButton1Holded && !shiftButton2Holded ) {
       // Transpose from C-1 to C8
       if ( value == -3 ) {
          if ( (ROOTKEY + currentTranspose - 12)  >= 0 ) currentTranspose -= 12 ;
          sendConfirmNote(ROOTKEY + currentTranspose,120);
       } else
    
       if ( value == -4  ) {
          if ( (ROOTKEY + currentTranspose + 12) <= 0x6C ) currentTranspose += 12 ;
          sendConfirmNote(ROOTKEY + currentTranspose,120);
       } else

       if ( shiftButton1Holded && value == -5  ) {
          currentVelocity = ( currentVelocity - 1 < 10 ? 10 : currentVelocity - 1 ) ;
          sendConfirmNote(ROOTKEY + currentTranspose,currentVelocity);
       } else

       if ( shiftButton1Holded && value == -6  ) {
          currentVelocity = ( currentVelocity + 1 > 127 ? 127 : currentVelocity + 1 ) ;
          sendConfirmNote(ROOTKEY + currentTranspose,currentVelocity);          
       } else

       // Reset to default / Reset all controllers     
       if ( value == -7 ){ 
          currentTranspose   = 0; 
          currentMidiChannel = 0;   
          currentVelocity    = 110;
       }           
    } else     
    
    if (isOn && !shiftButton1Holded && shiftButton2Holded ) {

       // Reset NanoKeyboard
       if ( value == -7 ){ 
          softReset();
       }    

      // Panic
       if ( value == -6 ){ 
          panicMode();
       }                                           
    } else if ( !shiftButton1Holded && !shiftButton2Holded ) {

        if ( value == -3 ){
            midiSerial->write(command);midiSerial->write(12);midiSerial->write(currentVelocity);
            noteOnTable[12] = isOn ;
         }    

    }

 } 
 else // Notes
 {

     
     // SHIFT1+SHIFT2 + KEY = Midi Channel. 
     if ( shiftButton1Holded && shiftButton2Holded  ) {
          if (isOn && midiChannelKeyMap[value] >=0) {
            currentMidiChannel = ( currentMidiChannel == midiChannelKeyMap[value] ? midiChannelKeyMap[value]+8 : midiChannelKeyMap[value]) ;      
            sendConfirmNote(ROOTKEY,120);
          }
          return;
     }

     uint8_t  note = value+ROOTKEY+ currentTranspose;
     int      noteOffset = 0;     
     
     // SHIFT1 | SHIFT2  + KEY : Midi instant transpose
     
     // Manage note off when shift keys were pressed
     if ( !isOn ) {

          if ( (note-12) >= 0 && (note+12) <= 0x78 ) {
             if ( noteOnTable[note-12] == 2) noteOffset = -12;
             else if ( noteOnTable[note+12] == 2) noteOffset = 12;
             if (noteOffset) {
                  noteOnTable[note + noteOffset] = 0;
                  midiSerial->write(command);midiSerial->write(note+noteOffset);midiSerial->write(currentVelocity);
                  return;
             }
          }      
     } else

     // Note ON               
     {
         if ( shiftButton1Holded && ! shiftButton2Holded ) {
                noteOffset = note - 12 < 0 ? 0 : - 12;
         }
        
         if ( !shiftButton1Holded && shiftButton2Holded ) {
                noteOffset = note + 12 > 0x78 ? 0:12;     
         }
     }

     midiSerial->write(command);midiSerial->write(note+noteOffset);midiSerial->write(currentVelocity);
     noteOnTable[note+noteOffset] = noteOffset ? 2 : isOn ;
 }
}  

/////////////////////////////////////////////////////////////////
// MAIN LOOP
// --------------------------------------------------------------
// Scan keyboard rows and columns and send on/off events.
/////////////////////////////////////////////////////////////////
void loop() {

  int r,c  ;

  for ( c = 0 ; c< sizeof(columnLines) ; c++ ) {                                         
        for ( r = 0 ; r< sizeof(rowLines) ; r++ ) {                           
          digitalWrite(rowLines[r],LOW);
          if ( digitalRead(columnLines[c]) == LOW ) {                          
              if ( keyStates[r][c] == 0 ) {
                keyStates[r][c] = 1;
                processEvent(keyValues[r][c],true);
              }
          } else {
            if ( keyStates[r][c] == 1 ) {
                keyStates[r][c] = 0;
                processEvent(keyValues[r][c],false);
              }               
          }
          digitalWrite(rowLines[r],HIGH);
        }
  }       
}
