/*
LAB SES 1 High Altitude Balloon downlink firmware written by VE3SVF.

***NEED TO FINISH*** camera trigger system. It has not been tested yet. 

This is not quite yet tested on real hardware (YET!)

Thanks:
https://wiki-content.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
https://reference.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

for the non-blocking camera help, plus brainwagon for the morse table and clever encoding algorithm!

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
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__ 

//Above: memory stuff

#define SEALEVELPRESSURE_HPA (1014) //Change depending on location. Here it's about 1000. Can be a decimal. Measured in hmp.

#define morse_WPM 18 //Morse code WPM

#define maxcycles 24000

Adafruit_BMP3XX bmp;

//#define cutdownpin 10
#define cutdownpin 9 //TESTING ONLY!!!

char datastring[90];
char CWdatastring[90];

const char regular_message[] PROGMEM = {"LABSES1 SND RPRT SASHA.NYC09 AT GMAIL.COM\n\n\n\n"}; //Prevent things from getting finicky, i.e SRAM usage i.e regular crashes

int cycle_num = 1; //This is used to keep track of what to transmit in the RTTY beacon, telemetry or reception stuff
//unsigned long cycles = 0; //Originally an int object, but it gets long *fast*
const long interval = 100; 
unsigned long previousMillis = 0; 
int frame_num = 0; 
int ledState = LOW;  
const int ledPin = 8; //Camera pin
unsigned long chars;
unsigned short sentences, failed;
int minute_time;
int hour_time;
int minute_time_start;
int hour_time_start;
bool cutdown_trig = false;
const int cutdown_time = 3; //hours to cutdown time

unsigned long cameraCycles = 0; 

static const int RXPin = 12, TXPin = 4;

RH_ASK driver(2000, 1, 9, 5);

//CW stuff

struct t_mtab { char c, pat; } ;

struct t_mtab morsetab[] = {
    {'.', 106},
  {',', 115},
  {'?', 76},
  {'/', 41},
  {'A', 6},
  {'B', 17},
  {'C', 21},
  {'D', 9},
  {'E', 2},
  {'F', 20},
  {'G', 11},
  {'H', 16},
  {'I', 4},
  {'J', 30},
  {'K', 13},
  {'L', 18},
  {'M', 7},
  {'N', 5},
  {'O', 15},
  {'P', 22},
  {'Q', 27},
  {'R', 10},
  {'S', 8},
  {'T', 3},
  {'U', 12},
  {'V', 24},
  {'W', 14},
  {'X', 25},
  {'Y', 29},
  {'Z', 19},
  {'1', 62},
  {'2', 60},
  {'3', 56},
  {'4', 48},
  {'5', 32},
  {'6', 33},
  {'7', 35},
  {'8', 39},
  {'9', 47},
  {'0', 63}
} ;

#define N_MORSE  (sizeof(morsetab)/sizeof(morsetab[0]))

#define SPEED  (18) //Nice speed to TX some basic telemetry
#define DOTLEN  (1200/SPEED)
#define DASHLEN  (3*(1200/SPEED))

//Servo myservo;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

//Setup Morse TX

void dash()
{
  //digitalWrite(9, HIGH);
  tone(9,440);
  wdt_reset();
  delay(DASHLEN);
  wdt_reset();
  //digitalWrite(9, LOW) ;
  noTone(9);
  wdt_reset();
  delay(DOTLEN) ;
  wdt_reset();
}

void dit()
{
  //digitalWrite(9, HIGH) ;
  tone(9,440);
  wdt_reset();
  delay(DOTLEN);
  wdt_reset();
  //digitalWrite(9, LOW) ;
  noTone(9);
  wdt_reset();
  delay(DOTLEN);
  wdt_reset();
}

void send(char c)
{
  int i ;
  if (c == ' ') {
    Serial.print(c) ;
    wdt_reset();
    delay(7*DOTLEN) ;
    return ;
  }
  for (i=0; i<N_MORSE; i++) {
    if (morsetab[i].c == c) {
      unsigned char p = morsetab[i].pat ;
      Serial.print(morsetab[i].c) ;

      while (p != 1) {
          if (p & 1)
            dash() ;
          else
            dit() ;
          p = p / 2 ;
      }
      wdt_reset();
      delay(2*DOTLEN) ;
      return ;
    }
  }
}

void sendmsg(char *str)
{
  while (*str)
    send(*str++) ;
  //Serial.println("");
}



void setup() {
  wdt_disable();
  //datastring.reserve(70); //MEMORYYYYYYYYYYYYYYYYYYYYYYY
  pinMode(3,OUTPUT); //RTTY output
  pinMode(ledPin, OUTPUT); //Camera trigger
  //pinMode(5,OUTPUT); //CW transmitter
  pinMode(6,OUTPUT); //FM transmiter
  //pinMode(9,OUTPUT); //CW...

  //Setup transmitters
  digitalWrite(5,HIGH);
  digitalWrite(6,HIGH); //Enable all transmitters

  if (!driver.init()){
         Serial.println("Backup telemetry transmitter init failed");
  }

 

  //Start sending CW!

  //sender.setMessage(String("G00DBOY"));

  //sender.startSending(); //Push to buffer
  //sender.sendBlocking();
  
  //myservo.attach(9); 
  //myservo.write(0);
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
  ss.begin(9600);
  if (ss.available() > 0){
    if (gps.encode(ss.read())){
      minute_time_start = gps.time.minute();
      hour_time_start = gps.time.hour();
    }
  }
   ss.end(); // End GPS comms
  wdt_enable(WDTO_8S);
  //sendmsg("G00DBOY"); //Send start message!
 
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
              //sendmsg("E");
              
              if (gps.time.isValid()){
                minute_time = gps.time.minute();
                hour_time = gps.time.hour();
                //Serial.println(hour_time);
                //Serial.println(minute_time);
                /*
                if (hour_time >= hour_time_start+cutdown_time and minute_time >= minute_time_start and cutdown_trig == false){
                  Serial.print("CUTDOWN");
                  //CUTDOOOOOOOOOOOOOOOOOOOWWWNNNNNNNNN
                  cutdown();
                  cutdown_trig == true; // no need to cutdown a zillion times
                }
                */
              }
              
            //Serial.print("Encoded data!");
              if (gps.location.isValid())
              {
                //Serial.print("Valid location!");
                latitude_SES = gps.location.lat();
                longitude_SES = gps.location.lng();
                //Serial.print(latitude_SES);
                //Serial.print(longitude_SES);
                read_data = true;
                //Serial.print(F("VALID LOCATION FOUND!!!"));
                
              }
              
          
        }
      }
      
  }
    read_data = false; //Prepare for next time we run
    ss.end(); //Very important
    //sendmsg("E");

    //Serial.print("Start.");
    sprintf(datastring,"TLM: ");
    //Serial.print("Good.");
    char voltage_string[10]; //Just to be safe!
    //Serial.print("Good.");
    int voltage = readVcc();
    //Serial.print("Good.");
    dtostrf(voltage, 4, 0, voltage_string);
    //Serial.print("Good.");
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
    //Serial.print(pressure);
    //Serial.print("\n");
    ltoa(temp,temp_string,10);
    ltoa(alt,alt_string,10);
    ltoa(pressure,pressure_string,10);
    itoa(frame_num,frame_num_string,10);
    //ltoa(latitude_SES,lat_string,10);
    //ltoa(longitude_SES,long_string,10);
    //Serial.print(pressure_string);
    //Serial.print("Good.");
    //Serial.print(voltage);
    //Serial.print(voltage_string);
    //strcat(datastring,voltage_string);
    //Serial.println(datastring);
    dtostrf(latitude_SES, 7, 0, lat_string);
    dtostrf(longitude_SES, 7, 0, long_string);
    
    //Send data to CW transmitter

    if (cameraCycles % 1 == 0){
      sprintf(CWdatastring, "%s?%s?%s",lat_string,long_string,alt_string);
      Serial.println(CWdatastring);
      //sendmsg(CWdatastring);
      driver.send((uint8_t *)CWdatastring, strlen(CWdatastring));
    }
    sprintf(datastring, "AAAA,%s,%s,%s,%s,%s,%s,111\n\n\n\n\n",pressure_string,alt_string,temp_string,lat_string,long_string,frame_num_string);
    //Serial.print("Good. DATASTRING: ");
    Serial.print(datastring);
    rtty_txstring (datastring); //transmit it
    //char datastring[80];
    cycle_num = 0;
    //delete alt_string;
    //delete temp_string;
    //delete pressure_string;
    //delete lat_string;
    //delete long_string;
    frame_num++;
    
  }
  /*
  if (cycles == 2000){
    //CUTDOWN TIMEEEEEEEE
    //Um... cutdown!
    sprintf(datastring,"CUTDOWN IS A GO");
    rtty_txstring (datastring);
    
    myservo.write(60);
    delay(500);
    myservo.write(0);
    delay(500);
    myservo.write(70);
    delay(500);
    myservo.write(0);
    delay(500);
    myservo.write(80);
    delay(500);
    myservo.write(0);
    delay(500);
    //Above: let's make sure everything works! :) :)
    sprintf(datastring,"DEPLOY PARACHUTE SUCCESS");
    rtty_txstring (datastring);
  }
  */
  
  cycle_num++;
  //cycles++;
  //Serial.print(cycles);
  //Serial.print("\n");
    
}



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
  if (cameraCycles <= maxcycles){
    unsigned long currentMillis = millis(); //Camera
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
      if (ledState == LOW) {
          ledState = HIGH;
          cameraCycles++;
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
 
uint16_t gps_CRC16_checksum (char *string)
{
  size_t i;
  uint16_t crc;
  uint8_t c;
 
  crc = 0xFFFF;
 
  // Calculate checksum ignoring the first two $s
  for (i = 2; i < strlen(string); i++)
  {
    c = string[i];
    crc = _crc_xmodem_update (crc, c);
  }
 
  return crc;
}  


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
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

float average (int * array, int len)  // assuming array is int.
{
  long sum = 0L ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array [i] ;
  return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
}

long readVccaverage(){
  int adjustment_factor = 300; //This is for THE SPECIFIC HARDWARE running LAB SES 1. Change when switching to other arduinos!!
  int average_array[255]; //Averaging array
  long result;
  for (int i = 0; i <= 255; i++) {
    //Average 255 times
    average_array[i] = readVcc();
  }
  result = average(average_array,255);
  return result+adjustment_factor;
}


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

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
