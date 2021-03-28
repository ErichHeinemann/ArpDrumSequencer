// E.Heinemann
// e.heinemann@hman-project.de
// 30.10.2016 - E.Heinemann  Bonn Germany
// 2021-02-07 - E.Heinemann Updated and modified the MIDI-Note-Numbers and their names to map it to the MPX16-Defaults
// 2021-02-07 - E.Heinemann, new attempt to combine Menu-Button as a DN-Button with the Step-Buttons

// Helping sources:
// https://learn.sparkfun.com/tutorials/midi-shield-hookup-guide/example-1-clock-generator--receiver

#include <Wire.h>
#include <MIDI.h>
#include <LiquidCrystal_I2C.h>

/*
 * Encoder
 * 
int messungPin1 = LOW;
int messungPin1Alt = LOW;
int encoderWert = 0;

 void setup() { 
 pinMode(3, INPUT);
 pinMode(4, INPUT);
 Serial.begin(9600);
 } 

 void loop() {
 messungPin1 = digitalRead(3);
 if ((messungPin1 == HIGH) && (messungPin1Alt == LOW)) {
 if (digitalRead(4) == HIGH) {
 encoderWert++;
 } else {
 encoderWert--;
 }
 Serial.println (encoderWert);
 }
 messungPin1Alt = messungPin1;
 }

*/

// http://www.86duino.com/?p=8254
#include "TimerOne.h"


/*
 *  MPX16 receives on Channel 1 but sends on Channel 10 !
 *  Midi-Notes of the 16 Samples by default:
 *  40  41  42  43 | 48 49 50 51 
 *  36  37  38  39 | 44 45 46 47
 *  
 *  Simply Samples from Midi 36 to 51
 *  
 */ 


#include <EEPROM.h>



// Softserial is used to send MIDI via Pin TX 2, RX 3 
#include <SoftwareSerial.h>
SoftwareSerial softSerial( 2, 3 );
MIDI_CREATE_INSTANCE( SoftwareSerial, softSerial, midiA );

// Not implemented yet
uint16_t bpm     = 125;          // Default BPM
uint16_t old_bpm = bpm;      // Default BPM

uint8_t midi_channel      = 10;    // Default Midi Channel - Out
uint8_t midi_channel_arp1 = 1; // Default-Channel for ARP1 fast 
uint8_t midi_channel_arp2 = 1; // Default-Channel for ARP1 fast 

// uint8_t midi_channel_arp2 = 2; // Default-Channel for ARP2 slow 
// uint8_t midi_channel_arp3 = 3; // Default-Channel for ARP3 bass


uint8_t midi_sync    = 0;     // 1 == Slave, 0 = Master - default

uint8_t LCD_Address = 0x27;   // LCD is integrated via PCF8574 on Port 0x27
LiquidCrystal_I2C lcd( LCD_Address, 16, 2 );  // simple LCD with 16x2

uint8_t address1 = 0x3C;   // Address of the PCF8574 for first 8 Buttons and LEDs
uint8_t address2 = 0x38;   // Address of the second PCF for LEDS & Buttons 9 - 16

// Button-Pins
const uint8_t buttonPinS = 4;   // Menu/FN/Select-Button ..near to the POT
const uint8_t buttonPinL = 5;   // Left- or Start-Button
const uint8_t buttonPinR = 6;   // Right- or Stop-Button

// Value of the POT
uint16_t     aPin3 = 3;
uint16_t     aVal3;
uint16_t     old_aVal3;
uint8_t newNote; // Variable for the MidiNote in the Menu

uint8_t count_step =  0;  // Step counter
uint8_t count_bars = 16;  // count steps per Pattern, .. with 2 PCF8574, I am able to define 16 Steps max.... virtually perhaps 32....
uint8_t count_ppqn =- 1;  // 24 MIDI-Clock-Pulse per quart note counter

// Menu
String   Modes[] = { "Instr", "Velo", "Speed", "Bars", "Note", "Scale", "Sync" };
uint8_t  ModesNum[] = { 0,       1,      2,      3 ,     4,       5,      6   };

// Is the Sequencer running or not
boolean  playBeats = true;

// Current Menu Settings
uint8_t curModeNum = 0;
String  curMode = Modes[0]; // Sound or Play

// MIDI-Clock
const byte START = 250;
const byte CONTINUE = 251;
const byte STOP = 252;
const byte CLOCK = 248;
byte clockcounter = 0;

// Instruments, Accent is not an Instrument but internally handled as an instrument .. therefore 17 Instruments from 0 to 16
const String shortSounds[ 17 ] ={ "ACC"   , "ARP1L", "ARP1M" , "ARP1H" , "ARP2", "ARPBS", "PAD 1", "PAD 2", "PAD 3", "PAD 4", "PAD 5", "PAD 6", "PAD 12","PAD 13","PAD 14","PAD 15","PAD 16" };
// const String Sounds[ 17 ]      ={ "ACC"   , "ARP 1 L", "ARP 1 M" , "ARP 1 H" , "ARP 2", "ARP Bass", "PAD 1", "PAD 2", "PAD 3", "PAD 4", "PAD 5", "PAD 6", "PAD 12","PAD 13","PAD 14","PAD 15","PAD 16" };
uint8_t iSound[ 17 ]           ={  -1,  0,  0,  0,  0,  0, 41, 42, 43,  44, 45, 46, 47, 48, 49, 50, 51 }; // MIDI-Sound, edited via Menu 50=TOM, 44=closed HH, 
uint8_t iVelo[ 17 ]            ={ 127, 90, 90, 90, 90, 90, 90, 90, 90,  90, 90, 90, 90, 90, 90, 90, 90 }; // Velocity, edited via Menu
uint8_t inotes1[ 17 ]          ={ 255,255,255,255,255,255,255,255,255, 255,255,255,255,255,255,255, 255 };
uint8_t inotes2[ 17 ]          ={ 255,255,255,255,255,255,255,255,255, 255,255,255,255,255,255,255, 255 };
// uint8_t legato_steps1[ 8 ]     ={ 0,0,0,0, 0,0,0,0 }; // Steps which hang over to the next step
// uint8_t legato_steps2[ 8 ]     ={ 0,0,0,0, 0,0,0,0 };
// uint8_t slide_steps1[ 8 ]      ={ 0,0,0,0, 0,0,0,0 }; // Sliding Steps which Slide to the next voice ( max 12 Pitch )
// uint8_t slide_steps2[ 8 ]      ={ 0,0,0,0, 0,0,0,0 };

// uint8_t rnd_velo_steps1[ 8 ]   ={ 0,0,0,0, 0,0,0,0 }; // Random Velocity als Faktor ??
// uint8_t rnd_velo_steps2[ 8 ]   ={ 0,0,0,0, 0,0,0,0 };
uint8_t lowest_note = 127;


uint8_t notelength = 3; // 2 beats maximum

uint8_t noteoff[16][16]={ {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{} }; // 16 Steps, in denen 16 Noten deaktiviert werden könnten, dsa sollte reichen.

uint8_t arp_in_notes_list[5]  = { 0,0,0,0,0 }; // 5 Noten sollten als Speicher reichen, die ersten Noten werden durchsortiert und in ARP1, ARP2, ARP Bass eingefügt.

uint8_t arp_in_notes_count = 5; // nach fünf Noten fällt die älteste wieder raus, Arp1 LMH, Arp 2, ARPBass
                                // Nach 2 Takten sollte man eh Noten rauswerfen. Sind weniger Noten im Array, werden Noten widerholt 
// Current Instrument .. the first selected
int curIns = 1; // 1 = PAD  1, 0= Accent

uint16_t timer_time=5000; //Time in microsecond of the callback fuction
uint32_t tempo_delay;

uint8_t bar_value[]={ 1, 2, 3, 4, 6, 8, 12, 16};

// array of notes would be better, 2 bytes in notes for every instrument
uint8_t notes1; 
uint8_t notes2;

// Old MIDI-Tricks, HH-Sounds first, Cymbals first, Snare, BD at least,  ... to keep a tight beat
uint8_t oldStatus1=B00000000;
uint8_t oldStatus2=B00000000;
uint8_t bits1 = 0;
uint8_t bits2 = 0;

// Scale, Menu to change Scale is yet not implemented
uint8_t scale=1 ;//Default scale  1/16
uint8_t      scale_value[]  = {     3,      6,    12,     24,       8,       16 };
const String scale_string[] = { "1/32", "1/16", "1/8", "1/4",  "1/8T",   "1/4T" };
  

// BPM to MS Conversion
// http://www.sengpielaudio.com/Rechner-bpmtempotime.htm
  
// myRefresh is only a counter .. the higher the lower the BPM! To display a good BPM this value has to be translated
int myRefresh = 500;
int myStep  =  0;

uint8_t veloAccent  = 100;
uint8_t velocity    = 100;

uint8_t step_position = 0;
uint8_t b = 10;

uint8_t count_instr = 00;

boolean oldStateS=1;
boolean buttonStateS=1;
boolean oldStateL=1;
boolean buttonStateL=1;
boolean oldStateR=1;
boolean buttonStateR=1;

String curPattern1="xxxxxxxxx";
String curPattern2="xxxxxxxxx";

// ### Base-Function for PCF8574 and WIRE ### 
// PCF8574 Explosion Demo (using same pin for Input AND Output)
// Hari Wiguna, 2016
void WriteIo( uint8_t bits,uint8_t thisAddress ){
  Wire.beginTransmission(thisAddress);
  Wire.write(bits);
  Wire.endTransmission();
}

//  ### Base-Function for PCF8574 and WIRE ### 
// PCF8574 Explosion Demo (using same pin for Input AND Output)
// Hari Wiguna, 2016
uint8_t ReadIo( uint8_t address ){  
  WriteIo( B11111111, address );        // PCF8574 require us to set all outputs to 1 before doing a read.
  Wire.beginTransmission( address );
  // Wire.write(B11111111 );
  Wire.requestFrom( (int) address, 1 );  // Ask for 1 byte from slave
  uint8_t bits = Wire.read();         // read that one byte
  Wire.requestFrom( (int) address1, 1 ); // Ask for 1 byte from slave
  Wire.endTransmission(); 
  return bits;
}


void handleNoteOff( byte inChannel, byte inNote, byte inVelocity ){
  midiA.sendNoteOff( inNote, inVelocity, inChannel ); 
}

// Plug MIDI-Out of MPX16 into MIDI-In of the arduino 
// Press the PAD to select the PAD
void handleNoteOn( byte inChannel, byte inNote, byte inVelocity ){
  // has the user pressed a PAD on the MPX16??

  // Note Through
  midiA.sendNoteOn( inNote, inVelocity, inChannel ); 
  
  if( inChannel == 10 && inNote >=36 && inNote <= 51){
    Select_Instr( inNote - 35 );  
  }else{
    byte notex = 0;
    if( inVelocity > 0 && inNote >=12 ){
      for( uint8_t i=arp_in_notes_count-1; i>0; i-- ){
        notex =  arp_in_notes_list[i-1];
        arp_in_notes_list[ i ] = notex;
      }
      arp_in_notes_list[ 0 ] = inNote;
      byte tmp_arr[6] = {};
      tmp_arr[0] = arp_in_notes_list[ 0 ];
      tmp_arr[1] = arp_in_notes_list[ 1 ];
      tmp_arr[2] = arp_in_notes_list[ 2 ];
      tmp_arr[3] = arp_in_notes_list[ 3 ];
      tmp_arr[4] = arp_in_notes_list[ 4 ];
      tmp_arr[5] = 0;

      lowest_note = 127; // will be the bass-note
       
      for( uint8_t j = 0; j < 4; j++) {
        for( uint8_t i = 1; i < 4; i++) {
          notex = tmp_arr[i-1];
          if( tmp_arr[i] > 0 && tmp_arr[i] < notex ){
            tmp_arr[i-1] = tmp_arr[i];
            tmp_arr[i] = notex;
            if( notex != 0 && notex < lowest_note ){
              lowest_note = notex;
            }
          }
        }        
      } 

      iSound[ 1 ] = tmp_arr[ 0 ];
      iSound[ 2 ] = tmp_arr[ 1 ];
      iSound[ 3 ] = tmp_arr[ 2 ];
      iSound[ 4 ] = tmp_arr[ 3 ];
      /*
      //if( tmp_arr[ 1 ] > 0 ){ 
        iSound[ 2 ] = tmp_arr[ 1 ];
      // }else{
      //   iSound[ 2 ] = tmp_arr[ 0 ] + 12 * random(2);
      //}
      
      //if( tmp_arr[ 2 ] > 0 ){ 
        iSound[ 3 ] = tmp_arr[ 2 ];
      //}else{
      //  iSound[ 3 ] = tmp_arr[ 0 ] - 12 * random(2);
      //}
      
      //if( tmp_arr[ 3 ] > 0 ){ 
        iSound[ 4 ] = tmp_arr[ 3 ];
      //}else{
      //  iSound[ 3 ] = tmp_arr[ 0 ] - 12 * random(1);
      //}
      */ 
      iSound[ 5 ] = lowest_note;

      // lcd.setCursor(0,1);
      // lcd.print( arp_in_notes_list[ 0 ] );
      // lcd.setCursor(5,1);
      // lcd.print( arp_in_notes_list[ 1 ] );  
      // lcd.setCursor(10,1);
      // lcd.print( arp_in_notes_list[ 2 ] );

    } 
  }    
}


// Callback from Timer1
void callback(){ 
  if( old_bpm != bpm ){
    Timer1.initialize(tempo_delay); old_bpm = bpm;
  }
  count_ppqn++;    
  if( count_ppqn >= scale_value[scale] ){  

    process_step();
    /*
    step_position++;
    if( step_position >= count_bars ){
      step_position=0;
    }
    Update_Midi();
    */
    count_ppqn = 0;
    // digitalWrite(13, digitalRead(13) ^ 1);
  }     

  // lcd.setCursor(0,1);
  // lcd.print ("Step:" + String(step_position) + " " + scale_value[scale] );
        
}

void process_step(){
  Update_Midi();
  step_position++;
  if( step_position >= count_bars ){
    step_position=0;
  }
  // digitalWrite(13, digitalRead(13) ^ 1);
}


// ########################### Show Notes and current Step via LEDs #########################
void showStep ( uint8_t &patternstep, uint8_t address1, uint8_t address2, uint8_t notes1, uint8_t notes2 ){
  uint8_t bitdp1 = notes1;
  uint8_t bitdp2 = notes2;

  // Current Step would be only shown if played
  if( playBeats==true ){ 
    if( patternstep <  8 ){ bitClear( bitdp1,   patternstep ); }
    if( patternstep >= 8 ){ bitClear( bitdp2, ( patternstep-8 ) ); }
  }
   
  WriteIo( bitdp1, address1 );
  WriteIo( bitdp2, address2 );
}


// ############################ Play the Midi-Notes ##################################### 
void Update_Midi(){

  if( playBeats==true ){
    
    veloAccent = 100; // Normal Velocity by default
    // first Byte or first 8 Hits
    // "song_position" is the current step 
    if( step_position<8 ){ // play notes1
      // Accent set?
      if( bitRead( inotes1[0], step_position ) == 0 ){
        // Accent is set
        veloAccent = iVelo[0];
      }else if( bitRead( inotes2[0], step_position ) == 0 ){
        // Accent is set
        veloAccent = iVelo[0];
      }
    }
            
    // loop through all instruments .. But ignore Accent with 0       
    for( uint8_t i = 1; i < count_instr; i++ ){
      velocity = 0;
      if( i <= 5 && noteoff[ i ][ step_position ]>0 && noteoff[ i ][ step_position ] < 127 ){
        midiA.sendNoteOff( noteoff[ i ][ step_position ],         0, midi_channel_arp1 ); 
        noteoff[ i ][ step_position ] = 0;  
      }
      if( step_position<8 ){  
        // Note ON?
        if( bitRead( inotes1[i],step_position ) == 0 ){
          velocity  = round( iVelo[i] * veloAccent / 100);
        }
      }else{
        // Note ON?
        if( bitRead( inotes2[i],step_position-8 ) == 0 ){
          velocity  = round( iVelo[i] * veloAccent / 100);
        }
      }  
      if( velocity > 127 ){
        velocity = 127; 
      }
      if( velocity >0 ){
        if(i <= 5 ){
          // ARP-Notes
          // Secure Note OFf
          // midiA.sendNoteOff( iSound[i],         0, midi_channel_arp1 );   
          if( iSound[i] > 0 ){
            // register the note-off:
            noteoff[ i ][ ( step_position + notelength )%count_bars ] = iSound[i];
            midiA.sendNoteOff(  iSound[i], 0 , midi_channel_arp1 );
            // Play Note
            midiA.sendNoteOn(  iSound[i], velocity , midi_channel_arp1 );
          }    
        }else{
          // Drum-Instruments
          midiA.sendNoteOff( iSound[i],         0, midi_channel );       
          midiA.sendNoteOn(  iSound[i], velocity , midi_channel );
        }
      }  
    }
  }
    
} 


// ############################## Select another Instrument and update all variables and LCD ###########
void Select_Instr( int newIns ){
 //  if ( newIns != curIns ){
  curIns = newIns;
  if( newIns >= count_instr ){
    curIns = count_instr-1; 
  }   
  lcd.setCursor(0,0);
  lcd.print( curMode  + " " + shortSounds[ curIns ]  + " " + iVelo[ curIns ] +"    ");
  lcdShowPattern();
}

void lcdShowPattern(){
  notes1 = inotes1[curIns];
  notes2 = inotes2[curIns]; 
  curPattern1 = "";
  curPattern2 = "";
  // Show new Pattern on the Display
  for( uint8_t i = 0; i < 8; i++){
    if( bitRead( notes1, i ) == 0 ){ 
      curPattern1 = curPattern1 + "X";
    }else{ 
      curPattern1 = curPattern1 + "-";
    } 
    if( bitRead( notes2, i ) == 0 ){ 
      curPattern2 = curPattern2 + "X";
    }else{ 
      curPattern2 = curPattern2 + "-";
    } 
  }
  lcd.setCursor( 0, 1 );
  lcd.print( curPattern1 + curPattern2 );
}


// ################################ Check the Potentiometer ##############################
void Check_POT() {
      // Check the POT -- Menu-Functions
  aVal3 = round( analogRead( aPin3 ) /8 );
  if ( aVal3 != old_aVal3  ){ 
    // Correction
    myStep = myStep + 120; 

      // Select Instrument Menu
      if( curModeNum==0 ){
        Select_Instr( round(aVal3/8) );      
      }
      
      // Velocity
      if( curModeNum==1 ){
        if ( aVal3 > 127 ) { aVal3=127;}
        lcd.setCursor(0,0);
        lcd.print( curMode + " " + String(aVal3) + " " + shortSounds[ curIns ] + "     ");
        iVelo[ curIns ] = aVal3;  
      }

      // Speed-Menu
      if( curModeNum==2 ){
        old_bpm = bpm;
        bpm = round(analogRead(aPin3)/4 + 40);
        lcd.setCursor(0,0);
        lcd.print( curMode + " " + String(bpm) +" bpm   ");
        tempo_delay = 60000000 / bpm / 24;
      }
      
      // Bars
      if( curModeNum==3 ){
        if ( aVal3 > 127 ) { aVal3=127;}
        count_bars = round(aVal3 / 8)+1;
        if ( count_bars>16 ) 
          { count_bars=16; } // max of this setup
           lcd.setCursor(0,0);
        lcd.print( curMode + "  " + String( count_bars ) +"      ");
      }

      // Midi-Notes, perhaps we find a good Midi-Reference to replace Note and Name
      if( curModeNum==4  && curIns>0 ){   // 30 to 70 make sense
        newNote = round(aVal3/3) + 25;
        if ( newNote > 70 ) { newNote =70;}
        if ( newNote < 30 ) { newNote =30;}
        lcd.setCursor(0,0);
        lcd.print( curMode + " " + String(newNote) + " " + shortSounds[ curIns ] + "     ");
        iSound[ curIns ] = newNote; 
      }

      // Scale
      if( curModeNum==5 ){
        if( aVal3 > 127 ) { aVal3=127; }
        scale = round(aVal3 / 16 );
        if( scale >= sizeof( scale_string ) ){ scale = sizeof( scale_string )-1 ; }
        lcd.setCursor(0,0);
        lcd.print( curMode + "  " + scale_string[ scale ] + "      " );
      }
      
      old_aVal3 = aVal3;    
    }
}

// ################################ Check Menu - Buttons --#######################
void Check_MENU() {
    // Check if Menu-Button or Start or Stop was pressed
    buttonStateS = digitalRead( buttonPinS ); // OLD: Menu changes on buttonpress .. not on release. NEW: It has to change the Menu on "Release" of the button
                                              // Acting by release offers to do something while the button is pressed. ... faster Change 
    buttonStateL = digitalRead( buttonPinL );
    buttonStateR = digitalRead( buttonPinR );
    
    if( oldStateS==1 && buttonStateS == 0 ){
      curModeNum = curModeNum +1;
      if( curModeNum >(5-playBeats) ){ curModeNum = 0 ;} 
      curMode= Modes[curModeNum];
      lcd.setCursor(0,0);
      if( curModeNum != 3 && curModeNum != 2 && curModeNum != 5 ){
        lcd.print( curMode  + " " + shortSounds[ curIns ] +" " + buttonStateS  + "      ");
      }else{
        if( curModeNum == 3 ){
          lcd.print( curMode + "  " + String( count_bars ) +"      " );
        } 
        if( curModeNum == 2 ){ // Speed
          lcd.print(  curMode + " " + String(bpm) +" bpm    "   );
        }
        if( curModeNum == 5 ){
          lcd.setCursor(0,0);
          lcd.print( curMode + "  " + scale_string[ scale ] +"      "  );
        }
      } 
    }
    
    if( playBeats == false && buttonStateL != oldStateL && buttonStateL == LOW ){
      // Start-Button
      startBeating();
      
      // step_position=0;
      // playBeats = true;
      // lcd.setCursor(0,1);
      // lcd.print( "Started Beating " );
      // MIDI-Clock
      // midiA.sendRealTime( MIDI_NAMESPACE::Start );
    }
    if( playBeats == true && buttonStateR != oldStateR && buttonStateR == LOW ){
      // Stop-Button
      stopBeating();
      // step_position=0;
      // playBeats = false;
      // lcd.setCursor(0,1);
      // lcd.print( "Stopped Beating ");
      // midiA.sendRealTime(MIDI_NAMESPACE::Stop);
    }
    oldStateS = buttonStateS;
    oldStateL = buttonStateL;
    oldStateR = buttonStateR;
}

void startBeating(){
  step_position=0;
  playBeats = true;
  lcd.setCursor(0,1);
  lcd.print( "Started Beating " );
  // MIDI-Clock
  midiA.sendRealTime( MIDI_NAMESPACE::Start );
}

void stopBeating(){
  // Stop-Button
  step_position=0;
  playBeats = false;
  lcd.setCursor(0,1);
  lcd.print( "Stopped Beating ");
  midiA.sendRealTime(MIDI_NAMESPACE::Stop);  
}

// ############################## Check the Buttons for Notes #################################
void Check_DrumButtons() {
  
  //-- Don't do anything unless they press a switch --
  
   // Unless they're all high...  
  if( bits1 != B11111111 && oldStatus1 != bits1 ){  
    //-- Find lowest pressed switch --
    for( byte bitIndex = 0; bitIndex < 8; bitIndex++ ){
      if( bitRead(bits1, bitIndex) == 0) {
        if( oldStateL == HIGH ) {
          if( bitRead(notes1, bitIndex) == 1) {
            bitClear( notes1, bitIndex ); 
          }else{
            bitSet( notes1, bitIndex );
          } 
          inotes1[curIns] = notes1;
        }else{
          // Instrument_Select
          Select_Instr( bitIndex );
        }
        exit;
      }    
    }
    lcdShowPattern();
  }

  // Unless they're all high...
  if ( bits2 != B11111111  && oldStatus2 != bits2 ){
    //-- Find lowest pressed switch --
    for( byte bitIndex = 0; bitIndex < 8; bitIndex++){
      if( bitRead( bits2, bitIndex) == 0 ){
        //  Both Buttons L and S are not pressed!
        
        if( oldStateL == HIGH ){ 
          // && oldStateS == HIGH )
          if( bitRead(notes2, bitIndex ) == 1){
            bitClear(notes2, bitIndex ); 
          }else{
            bitSet( notes2, bitIndex ); 
          }  
          inotes2[curIns] = notes2;          
        }else{
          // left 2 Bits to change the Scale
          if( bitIndex == 0 ){ 
            if( scale > 0 ){ scale = scale-1; }
            lcd.setCursor(0,0);
            lcd.print( Modes[5] + "  " + scale_string[ scale ] +"      "  );
          }
          // left 2 Bits to change the Scale
          if( bitIndex == 1 ){ 
            scale = scale+1;
            if ( scale >=4){ scale=0; }// only llop through straight scales 
            lcd.setCursor(0,0);
            lcd.print( Modes[5] + "  " + scale_string[ scale ] +"      "  );
          }

          // left 4 Bits to change the Scale
          if( bitIndex==6 ){
            old_bpm = bpm;
            bpm = bpm - 2;
            if( bpm < 40 ){ bpm=40; }
            lcd.setCursor(0,0);
            lcd.print( Modes[2] + " " + String(bpm) +" bpm   "  );
            tempo_delay = 60000000 / bpm / 24;
          }
          
          // add some speed to BPM
          if( bitIndex==7 ){
            old_bpm = bpm;
            bpm = bpm + 2;
            if (bpm > 300){ bpm=300; }
            lcd.setCursor(0,0);
            lcd.print( Modes[2] + " " + String(bpm) +" bpm   "  );
            tempo_delay = 60000000 / bpm / 24;
          }
        } 
        exit;
      }    
    }
    lcdShowPattern();
  }
  oldStatus1 = bits1;
  oldStatus2 = bits2; 
}



// #### Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup #####

void setup(){

  // for Debugging
  // Serial.begin( 115200 );

  pinMode(13, OUTPUT);
  
  lcd.init(); // initialize the lcd 
  // Print a message to the LCD
  lcd.backlight(); // I have not enabled that pin... at my box, the backlight is always on, via hardwire.
  lcd.print( "ArpSeq" ); // << put in Your prefered Name for this Sequencer into this variable:)

  count_instr = sizeof(iSound);

  Wire.begin(); // Arduino UNO uses (SDA=A4,SCL=A5)
  // Wire.begin(0,2); // ESP8266 needs (SDA=GPIO0,SCL=GPIO2)

  pinMode(buttonPinS, INPUT); // Selecct Button
  pinMode(buttonPinL, INPUT); // Left- or Start-Button
  pinMode(buttonPinR, INPUT); // Right- or Stop-Button
  
  // virtual Pull-Up-Resitor activated, Pins are by default "HIGH"
  digitalWrite( buttonPinS, HIGH); // Selecct Button
  digitalWrite( buttonPinL, HIGH); // Left- or Start-Button
  digitalWrite( buttonPinR, HIGH); // Right- or Stop-Button

  notes1 = B11111111; // Bitwise per Instrument, No Note equals "1", note to play equals "0" !
  notes2 = B11111111;
  midiA.setHandleNoteOn( handleNoteOn );
  midiA.setHandleNoteOff( handleNoteOff );

  midiA.setHandleStart( startBeating ); // Start at 0
  midiA.setHandleClock( beatClock );
  midiA.setHandleStop( stopBeating );
  midiA.setHandleContinue( startBeating );
  
  // midiA.setHandleRealTimeSystem( beatClock1 ); //Betonung auf 1. – hier "beatClock2" eintragen

  midiA.begin( MIDI_CHANNEL_OMNI );   // Midi-Input for Sync
  midiA.turnThruOff(); // this prevents the MPX16 to collaps
  
  tempo_delay = 60000000/bpm/24;      // delay in Microseconds ....
  Timer1.initialize( tempo_delay );   // initialize timer1, and set a 1/2 second period
  Timer1.attachInterrupt( callback );  


  // Select Sounds and disable Reverb

  // https://nickfever.com/Music/midi-cc-list
  // CC 91
  midiA.sendControlChange( 7, 255, 1 );
  midiA.sendControlChange( 91, 10, 1 );
  midiA.sendControlChange( 93, 0, 1 );
  midiA.sendControlChange( 95, 0, 1 );
  midiA.sendControlChange( 0x3A, 0,1 );
  midiA.sendControlChange( 0x3B, 0,1 );
  // Disable Post-Effects for GM
  midiA.sendControlChange( 0x62, 0,1 );
  // Disable Post-Effects for Reverb/Chorus
  midiA.sendControlChange( 0x62, 0,1 );

 
  midiA.sendProgramChange( 22, 1);

  
  midiA.sendControlChange ( 7, 127, 10 );
  midiA.sendControlChange ( 91, 10, 10 );

  // midiA.beginNrpn ( 0x3707, 1);
  // midiA.sendNrpnValue (0x7f, 1);
  // midiA.endNrpn (1);
  // Volume: 3707h DataByte bis 7fh
  // GM-Volume: 3722h 0 bis 7fh 
  // Send a Control Change message. More...
  
}


void Read_Switches(){
  bits1 = ReadIo( address1 ); // Read all switches
  bits2 = ReadIo( address2 ); // Read all switches
}



// ############################################ void loop #################################################

void loop() {
  // b is a simple Counter to do nothing  
  b ++;
  midiA.read();
  
  // read the IO-Pins every 5 cycles
  if( b > 4 ){ 
    // Update the LEDs
    showStep( step_position, address1, address2, notes1, notes2 );
    Check_MENU();
    b = 0;
  }
  
  midiA.read();
  
  myStep ++; // This is second counter to find the right time to get the new values from the BUTTONS and the POT
  
  if( myStep >= myRefresh ){
    myStep = 0;
    Read_Switches();
    midiA.read();
    Check_POT();
    midiA.read();
    Check_MENU();
    Check_DrumButtons(); 
    // lcd.setCursor(0,1);
    // lcd.print ("Step:" + String(step_position) + " " + scale_value[scale] + " " + String(scale) + "   " );
  } 

  // Check MIDI IN
  // Is there a MIDI message incoming ?
  midiA.read();



  
  /*
    switch( midiA.getType()){      // Get the type of the message we caught
      // case midi::ProgramChange:       // If it is a Program Change,
      
        // BlinkLed(MIDI.getData1());  // blink the LED a number of times
                                            // correponding to the program number
                                            // (0 to 127, it can last a while..)
      // case midi::                                            
      break;
        // See the online reference for other message types
        default:
        break;
     }
  */
}

   


//////////////////////////////////////////////////////////////////
//This function is call by the timer depending Sync mode and BPM//
//////////////////////////////////////////////////////////////////
void Count_PPQN(){
  
//-----------------Sync SLAVE-------------------//  
/*  if(midi_sync){
    timer_time=5000;
    if (midiA.read())                // Is there a MIDI message incoming ?
     {
      byte data = midiA.getType();
      if(data == midi::Start ){
        if(playBeats==true) //mode==MODE_PATTERN_PLAY || mode==MODE_PATTERN_WRITE || mode==MODE_INST_SELECT)
        {
          playBeats=true;
          // play_pattern = 1;
          count_ppqn=-1;
        }
        
        //if(mode==MODE_SONG_PLAY || mode==MODE_SONG_WRITE){
        //  play_song = 1;
        //  count_ppqn=-1;
        //  song_position=0;
        // }
      }
      else if(data == midi::Stop ) {
        playBeats=false;
        // play_pattern = 0;
        // play_song = 0;
        // count_step=0;
        step_position=0;
        count_ppqn=-1;
        // song_position=0;
      }
      else if(data == midi::Clock && playBeats==true) //(play_pattern == 1 || play_song == 1))    case midi::Clock
      {
        count_ppqn++;
        count_step=count_ppqn/scale_value[scale];
        if(count_ppqn>=(count_bars * scale_value[scale])-1) {
          count_ppqn=-1; 
          step_position++;     
          // song_position++;
          // if (song_position==16) song_position=0;
          if ( step_position >= count_bars ) { step_position=0; }
          // Play Notes!!
          Update_Midi();
          // Request news from Drum-Buttons
          Check_DrumButtons(); 
          // Update the LEDs 
          showStep (step_position, address1, address2, notes1, notes2 );  
        }
        // if (count_ppqn>1) led_flag=0;//Led clignote reste ON 1 count sur 6
        // if (count_ppqn<=1) led_flag=1; 
        // led_flag=!led_flag;  
      }
      // if (data==MIDI_CLOCK && (play_pattern == 0 || play_song==0)){
      //  count_led++;
      //  if(count_led==12){
      //    count_led=0;
      //    led_flag=!led_flag;
      //  }
      //}
      
    }
  }
  //-----------------Sync MASTER-------------------//
  if(!midi_sync){
  */
    // timer_time=2500000/bpm;
    // midiA(MIDI_CLOCK);
    //  digitalWrite(13, digitalRead(13) ^ 1);
    /* 
    lcd.setCursor(0,1);
    lcd.print( " Timer" + String( count_ppqn ) ); 

    
    // midiA.sendRealTime(MIDI_NAMESPACE::Clock);
    if( playBeats==true ) //play_pattern||play_song)
    {   
      count_ppqn++;    
      count_step=count_ppqn/scale_value[scale];   
      if(count_ppqn>=(count_bars*scale_value[scale])-1){
        count_ppqn=-1;
        step_position++;
        // if (song_position==16) song_position=0;
        if ( step_position>= count_bars ) { step_position=0; }
        // Play Notes
        Update_Midi();
        Check_DrumButtons(); 
        // Update the LEDs 
        showStep (step_position, address1, address2, notes1, notes2 );
      }
      // if (count_ppqn>1) led_flag=0;//Led blink 1 count on 6
      // if (count_ppqn<=1) led_flag=1; 
      // led_flag=!led_flag;
    }
    else if(playBeats==false) // !play_pattern &&!play_song)
    {
      count_ppqn=-1;
      step_position=0;
      // count_led++;
      // song_position=0;
      // if(count_led==12){
      //  count_led=0;
      //  led_flag=!led_flag;
      // }
      
    }
//   }
*/
}

void beatClock(){
  clockcounter++;
  process_step();
  if(clockcounter == 25) {clockcounter = 1;}
  // if(clockcounter == 1) {digitalWrite(13, HIGH);}
  // if(clockcounter == 2) {digitalWrite(13, LOW);}
}

/*
void xxx_beatClock(byte realtimebyte) {
  if(realtimebyte == START || realtimebyte == CONTINUE) {zaehler = 0;}
  if(realtimebyte == STOP) { digitalWrite(13, LOW); }
  if(realtimebyte == CLOCK) {    
    clockcounter++;
    process_step();
    if(clockcounter == 25) {clockcounter = 1;}
    if(clockcounter == 1) {digitalWrite(13, HIGH);}
    if(clockcounter == 2) {digitalWrite(13, LOW);}
  }
}
*/
/*
void beatClock2(byte realtimebyte) {
  if(realtimebyte == START || realtimebyte == CONTINUE) {clockcounter = 0;} 
  if(realtimebyte == STOP) {digitalWrite(13, LOW);}
  if(realtimebyte == CLOCK) {
    clockcounter++; 
    if(clockcounter == 97) {clockcounter = 1;}
    if(clockcounter == 1 || clockcounter == 24 || clockcounter == 48 || clockcounter == 72) { 
      digitalWrite(13, HIGH);
    } 
    if(clockcounter == 5 || clockcounter == 25 || clockcounter == 49 ||  clockcounter == 73) { 
      digitalWrite(13, LOW);
    }
  }
}
*/
