/*
LAB SES 1 High Altitude Balloon downlink firmware written by VE3SVF.

***NEED TO FINISH*** camera trigger system. It has not been tested yet. 

This is not quite yet tested on real hardware (YET!)

Thanks:
https://wiki-content.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
https://reference.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

for the non-blocking camera help!!!

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

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__ 

//Above: memory stuff

#define SEALEVELPRESSURE_HPA (1014) //Change depending on location. Here it's about 1000. Can be a decimal. Measured in hmp.

Adafruit_BMP3XX bmp;

char datastring[90];

const char regular_message[] PROGMEM = {"LAB SES 1 CALLING PLEASE SEND REPORTS TO SASHA.NYC09 AT GMAIL.COM\n"}; //Prevent things from getting finicky, i.e SRAM usage i.e regular crashes

int cycle_num = 1; //This is used to keep track of what to transmit in the RTTY beacon, telemetry or reception stuff
unsigned long cycles = 0; //Originally an int object, but it gets long *fast*
const long interval = 100; 
unsigned long previousMillis = 0;  
int ledState = LOW;  
const int ledPin = 8; //Camera pin
unsigned long chars;
unsigned short sentences, failed;

static const int RXPin = 12, TXPin = 4;

//Servo myservo;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


void setup() {
  //datastring.reserve(70); //MEMORYYYYYYYYYYYYYYYYYYYYYYY
  pinMode(3,OUTPUT); //RTTY output
  pinMode(ledPin, OUTPUT); //Camera trigger
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
}

void loop() {
  boolean newData = false; //Sorry, but this has to be here
  
  if (cycle_num == 1){
    //Serial.print("Start loop 1");
    sprintf_P(datastring,regular_message);
    rtty_txstring (datastring);
    char datastring[80];
    //Serial.print("Finished loop 1");
    
  }
  if (cycle_num == 2){
    char alt_string[20];
    char temp_string[20];
    char pressure_string[20];
    char lat_string[20];
    char long_string[20];
    float latitude_SES = 0.000000;
    float longitude_SES = 0.000000;
      // For three seconds we parse GPS data and report some key values
    bool read_data = false;
  ss.begin(9600);
  while (read_data == false)
  
  {    //Begin GPS comms
      //Serial.print("GOOD.");
      if (ss.available() > 0){
        //Serial.print("Reading data");
        if (gps.encode(ss.read())){
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
  
    //Serial.print("Start.");
    sprintf(datastring,"TLM: ");
    //Serial.print("Good.");
    char voltage_string[10]; //Just to be safe!
    //Serial.print("Good.");
    int voltage = readVcc();
    //Serial.print("Good.");
    dtostrf(voltage, 4, 0, voltage_string);
    //Serial.print("Good.");
    long temp = read_temp()*100;
    long alt = read_alt()*100;
    long pressure = read_pressure()*100;
    latitude_SES = latitude_SES*1000000;
    longitude_SES = longitude_SES * -1000000; //ONLY WORKS FOR TORONTO, ONTARIO!!!!
    //Serial.print(pressure);
    //Serial.print("\n");
    ltoa(temp,temp_string,10);
    ltoa(alt,alt_string,10);
    ltoa(pressure,pressure_string,10);
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
    sprintf(datastring, "AAAA,%s,%s,%s,%s,%s,111\n",pressure_string,alt_string,temp_string,lat_string,long_string);
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
  cycles++;
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
   //Trigger camera
  unsigned long currentMillis = millis(); //Camera
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
      if (ledState == LOW) {
          ledState = HIGH;
      } 
      else {
        ledState = LOW;
      }
   //digitalWrite(ledPin, ledState);
   //Serial.print(ledState);
   //Serial.print("\n");
        
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
