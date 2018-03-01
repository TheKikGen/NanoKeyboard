#include <SoftwareSerial.h>
#define RX 2
#define TX 3

SoftwareSerial midiSerial(RX, TX); // RX, TX

uint8_t columnLines[] {13,5,6,7,8,9};
uint8_t rowLines[] {12,11,10,4};

#define ROOTKEY 0x30        // C3
#define SHIFT_BUTTON1 -1
#define SHIFT_BUTTON2 -2

// Positive keys = midi note offset from ROOTKEY
// Negative key = buttons.
int keyValues[4][6]={{0  ,1  ,2   ,3  ,4   ,5 }, 
                     {6  ,7  ,8   ,9  ,10  ,11},
                     {12 ,-3 ,-4  ,-5 ,-6  ,-7},
                     {255,255,255 ,255,-1  ,-2}
                    };
uint8_t keyStates[4][6]={ {0,0,0,0,0,0}, 
                          {0,0,0,0,0,0},
                          {0,0,0,0,0,0},
                          {0,0,0,0,0,0}
                        };

int midiChannelKeyMap[]={0,-1,1,-1,2,3,-1,4,-1,5,-1,6,7};
bool shiftButton1Holded = false;
bool shiftButton2Holded = false;
uint8_t currentMidiChannel =0;
int currentTranspose = 0;
uint8_t currentVelocity  = 110;


void setup() {

  int r,c  ;
  Serial.begin(115200);
  midiSerial.begin(31250);
  for ( r = 0 ; r< sizeof(rowLines) ; r++ ) { pinMode(rowLines[r],OUTPUT);digitalWrite(columnLines[c],HIGH) ;}
  for ( c = 0 ; c< sizeof(columnLines) ; c++ ) { pinMode(columnLines[c],INPUT_PULLUP) ;  }
    
utility1();

         
}
/////////////////////////////////////////////////
// Software reset 
/////////////////////////////////////////////////
void(* softReset) (void) = 0; //declare reset function @ address 0

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

void sendConfirmNote(uint8_t note,uint8_t velocity) {
     midiSerial.write(0x90+currentMidiChannel);midiSerial.write(note);midiSerial.write(velocity);
     delay(100);
     midiSerial.write(0x80+currentMidiChannel);midiSerial.write(note);midiSerial.write(velocity);
}

void processEvent(int value,bool isOn){
  
 if (value == SHIFT_BUTTON1) { shiftButton1Holded = isOn; return;}
 if (value == SHIFT_BUTTON2) { shiftButton2Holded = isOn; return;}

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
       if ( value = -7 ){ 
          currentTranspose   = 0; 
          currentMidiChannel = 0;   
          currentVelocity    = 110;
       }           
    } else     
    
    if (isOn && !shiftButton1Holded && shiftButton2Holded ) {

       // Reset NanoKeyboard
       if ( value = -7 ){ 
          softReset();
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

     uint8_t noteOffset = 0;
     
     // SHIFT1 | SHIFT2  + KEY : Midi instant transpose
     if ( shiftButton1Holded && ! shiftButton2Holded ) {
          noteOffset = value+ROOTKEY+ currentTranspose - 12 < 0 ? 0 : - 12;
     }

     if ( !shiftButton1Holded && shiftButton2Holded ) {
          noteOffset = value+ROOTKEY+ currentTranspose + 12 > 0x78 ? 12:0;
     }

     uint8_t command = currentMidiChannel + ( isOn ? 0x90 : 0x80 );
     midiSerial.write(command);midiSerial.write(value+ROOTKEY+ currentTranspose+noteOffset);midiSerial.write(currentVelocity);
 }
}  


void loop() {
  // put your main code here, to run repeatedly:

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