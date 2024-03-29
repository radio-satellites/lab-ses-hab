/*
LAB SES 1 High Altitude Balloon downlink firmware written by VE3SVF.

Thanks:
https://wiki-content.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
https://reference.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

for the non-blocking camera help!

PINMAP:

3 - RTTY output (audio)
4 - True GPS TX pin (wire is GPS RX)
5 - CW transmitter power switch
6 - FM transmitter power switch
7 - GPS TX pin seems to be wired here!?
8 - Camera pin
9 - Morse code transmitter symbol line
10 - cutdown trigger low amp line
12 - GPS RX pin (wire is GPS TX)
A4 - BMP390 SDA
A5 - BMP390 SCK

*/

#include <string.h>
#include <util/crc16.h>
#include <stdlib.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <morse.h>
#include <avr/wdt.h>


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__ 

//Above: memory stuff




Adafruit_BMP3XX bmp;

#define cutdownpin 10
//#define cutdownpin 9 //TESTING ONLY!!!

/////////////////////////////////// CHANGE THESE //////////////////////////////////////

#define cutdown_alt 11000 //Meters AGL

#define camera_trigger_alt 9000 //Altitude to start taking pictures in meters AGL 

#define lower_cutdown_alt 9000 //Meters AGL lower range of cutdown

#define SEALEVELPRESSURE_HPA (1010.6) //Change depending on location. Here it's about 1000. Can be a decimal. Measured in hmp.

#define stop_camera 6000 //seconds

//////////////////////////////////////////////////////////////////////////////////////

#define maxcycles stop_camera*4

char datastring[90];
char CWdatastring[90];

const char regular_message[] PROGMEM = {"LABSES1 SND RPRT SASHA.NYC09 AT GMAIL.COM\n\n\n\n"}; //Prevent things from getting finicky, i.e SRAM usage i.e regular crashes

const char easter_egg[] PROGMEM = {"EASTER EGG!\n CMD>baguettes received! Transfer 5 baguettes \n\n BAGUETTEBAGUETTEBAGUETTEBAGUETTEBAGUETTE bit.ly/3OztoN2\n\n"};

int cycle_num = 1; //This is used to keep track of what to transmit in the RTTY beacon, telemetry or reception stuff
//unsigned long cycles = 0; //Originally an int object, but it gets long *fast*
const long interval = 100; 
unsigned long previousMillis = 0; 
int frame_num = 0; 
int ledState = LOW;  
const int ledPin = 8; //Camera pin
unsigned long chars;
unsigned short sentences, failed;
bool cutdown_trig = false;
bool camera_on = false;

static const int RXPin = 12, TXPin = 4;

//Servo myservo;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);



void setup() {
  wdt_disable();
  //datastring.reserve(70); //MEMORYYYYYYYYYYYYYYYYYYYYYYY
  pinMode(3,OUTPUT); //RTTY output
  pinMode(ledPin, OUTPUT); //Camera trigger
  pinMode(cutdownpin,OUTPUT); //Cutdown pin
  pinMode(6,OUTPUT); //FM transmiter
  pinMode(9,OUTPUT); //CW...

  Serial.begin(9600);
  if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
  //if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
  //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
    Serial.println("Could not find a valid BMP3 sensor, this is bad!");
    while (1);
  }
    // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  if (! bmp.performReading()) { //First reading is always off, read it once. 
    Serial.println("Failed to perform reading :(");
    return;
  }
  //char testmessage[] = "LAB SES 1 300 BAUD TEST";
  //rtty_txstring_300(testmessage);
  wdt_enable(WDTO_8S);
 
}

void loop() {
  boolean newData = false; //Sorry, but this has to be here

  
  if (cycle_num == 1){
    //sendmsg("E");
    wdt_reset();
    //Serial.print("Start loop 1");
    wdt_reset();
    sprintf_P(datastring,regular_message);
    //sendmsg("E");
    wdt_reset();
    rtty_txstring (datastring);
    //sendmsg("E");
    wdt_reset();
    //sendmsg("E");
    char datastring[80];
    //Serial.print("Finished loop 1");
    
  }
  if (cycle_num == 2){
    wdt_reset();
    char alt_string[20];
    char temp_string[20];
    char pressure_string[20];
    char lat_string[20];
    char long_string[20];
    char frame_num_string[20];
    float latitude_SES = 0.000000;
    float longitude_SES = 0.000000;
    bool read_data = false;
    //sendmsg("E");
  ss.begin(9600);
  //sendmsg("E");
  while (read_data == false)
  
  {    //Begin GPS comms
      //Serial.print("GOOD.");
      if (ss.available() > 0){
        //Serial.print("Reading data");
        if (gps.encode(ss.read())){
              wdt_reset();
             
              if (gps.location.isValid())
              {
                //Serial.print("Valid location!");
                latitude_SES = gps.location.lat();
                longitude_SES = gps.location.lng();
                read_data = true;
                
              }
              
          
        }
      }
      
  }
    read_data = false; //Prepare for next time we run
    ss.end(); //Very important


    sprintf(datastring,"TLM: ");

    long temp = read_temp();
    long alt = read_alt();
    long pressure = read_pressure();
    //sendmsg("E");
    if (alt <= 0){
      alt = 0; // problems
    }
    if (temp <= 0){
      temp = 0; 
    }
    latitude_SES = latitude_SES*1000000;
    longitude_SES = longitude_SES * -1000000; //ONLY WORKS FOR TORONTO, ONTARIO!!!!

    ltoa(temp,temp_string,10);
    ltoa(alt,alt_string,10);
    ltoa(pressure,pressure_string,10);
    itoa(frame_num,frame_num_string,10);

    dtostrf(latitude_SES, 7, 0, lat_string);
    dtostrf(longitude_SES, 7, 0, long_string);

    sprintf(datastring, "AAAA,%s,%s,%s,%s,%s,%s,EEE\n\n\n\n\n",pressure_string,alt_string,temp_string,lat_string,long_string,frame_num_string); //No need to send sync on this one, because it's already sent
    rtty_txstring (datastring); //transmit it
     noTone(3); //Needed to free up timer
    
    // Now, send fast 100bd telemetry twice
    //Send receiver clock synchronization string first
    
     rtty_txstring_300("LSES1\n\n");
     sprintf(CWdatastring, "AABA?%s?%s?%sRRR\n\n",lat_string,long_string,alt_string);
      rtty_txstring_300(CWdatastring);
      rtty_txstring_300(CWdatastring);

     noTone(9); //Stop 100bd
   
    if (cutdown_trig == false){
      cycle_num = 0;
    }
    else{
      cycle_num = 1;
    }

    frame_num++;

    //Check for cutdown
    if (int(alt) >= cutdown_alt and cutdown_trig == false){
      Serial.print("INIT CUTDOWN!");
      cutdown_trig = true;
      //Cutdown loop - if not below 9km, continue cutdown
      bool below_limit = false;
      while (below_limit == false){
        //Serial.println("WHILE LOOP!");
        //cutdown();
        digitalWrite(cutdownpin,HIGH); //Trigger cutdown
        wdt_reset();
        long alt = read_alt(); //Read altitude again
         if (alt <= lower_cutdown_alt){
            below_limit = true;
         }
        
      }
      
      digitalWrite(cutdownpin,LOW); //End burn wire
        
    }

    if (int(alt) >= camera_trigger_alt){
      camera_on = true; //Turn on camera
    }
    else{
      camera_on = false;
    }
    
  }
  
  cycle_num++;

  if (frame_num == 50){
    rtty_txstring(easter_egg);
  }

    
}


//Functions for transmitting high speed 100bd telemetry

void rtty_txstring_300 (char * string)
{
 
  /* Simple function to sent a char at a time to 
     ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    Also a camera trigger function. This is because it runs fast enough, but not TOO fast.
    */

  
  char c;
  
  
 
  c = *string++;
 
  while ( c != '\0')
  {
    rtty_txbyte_300 (c);
    c = *string++;
    wdt_reset();
    //cycles++; //Camera stuff
      

  }
}
 
 
void rtty_txbyte_300 (char c)
{
  /* Simple function to sent each bit of a char to 
    ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    */
  
  int i;
 
  rtty_txbit_300 (0); // Start bit
 
  // Send bits for for char LSB first 
 
  for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
  {
    if (c & 1) rtty_txbit_300(1); 
 
    else rtty_txbit_300(0); 
 
    c = c >> 1;
 
  }
 
  rtty_txbit_300 (1); // Stop bit
  rtty_txbit_300 (1); // Stop bit
  wdt_reset(); // reset the watchdog

  
}
 
void rtty_txbit_300 (int bit)
{
  if (bit)
  {
    // high
    tone(9,870);
  }
  else
  {
    // low
    tone(9,445);
 
  }
 
  delayMicroseconds(10075); // 100 baud

 
}

//Functions for transmitting low rate (50bd) telemetry


void rtty_txstring (char * string)
{
 
  /* Simple function to sent a char at a time to 
     ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    Also a camera trigger function. This is because it runs fast enough, but not TOO fast.
    */
 
  char c;
  
  
 
  c = *string++;
 
  while ( c != '\0')
  {
    rtty_txbyte (c);
    c = *string++;
    //cycles++; //Camera stuff
      

  }
}
 
 
void rtty_txbyte (char c)
{
  /* Simple function to sent each bit of a char to 
    ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    */
 
  int i;
 
  rtty_txbit (0); // Start bit
 
  // Send bits for for char LSB first 
 
  for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
  {
    if (c & 1) rtty_txbit(1); 
 
    else rtty_txbit(0); 
 
    c = c >> 1;
 
  }
 
  rtty_txbit (1); // Stop bit
  rtty_txbit (1); // Stop bit
  wdt_reset(); // reset the watchdog
   //Trigger camera
  if (camera_on == false){
    unsigned long currentMillis = millis(); //Camera
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
      if (ledState == LOW) {
          ledState = HIGH;
      } 
      else {
        ledState = LOW;
      }
   digitalWrite(ledPin, ledState);
   //Serial.print(ledState);
   //Serial.print("\n");
        
    }
    
  }
  
}
 
void rtty_txbit (int bit)
{
  if (bit)
  {
    // high
    tone(3,870);
  }
  else
  {
    // low
    tone(3,700);
 
  }
 
  //                  delayMicroseconds(3370); // 300 baud
  delayMicroseconds(10000); // For 50 Baud uncomment this and the line below. 
  delayMicroseconds(10150); // You can't do 20150 it just doesn't work as the
                            // largest value that will produce an accurate delay is 16383
                            // See : http://arduino.cc/en/Reference/DelayMicroseconds
 
}

/*

void cutdown(){
  //Cutdown function
  tone(3,760);
  wdt_reset();
  delay(300);
  wdt_reset();
  tone(3,760);
  wdt_reset();
  delay(300);
  wdt_reset();
  tone(3,760);
  wdt_reset();
  delay(300);
  //Finish downlink warning
  //Start cutdown
  wdt_disable();
  digitalWrite(cutdownpin,HIGH); //Trigger cutdown
  delay(30000); //thirty seconds
  digitalWrite(cutdownpin,LOW); //End burn wire
  wdt_enable(WDTO_8S);
  tone(3,700);
  wdt_reset();
  delay(300);
  wdt_reset();
  tone(3,760);
  wdt_reset();

  
  
}

*/

// sensor reading functions

long read_pressure(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return 0;
  }
  return bmp.pressure / 100.0;
}

long read_alt(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return 0;
  }
  return bmp.readAltitude(SEALEVELPRESSURE_HPA);
}

long read_temp(){
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return 0;
  }
  return bmp.temperature;
}
