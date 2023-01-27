#include <Adafruit_NeoPixel.h>
#include <RH_ASK.h>
#include <SPI.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define NUM_CYCLES 10 // Number of cycles to activate visual beacon

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 6 // On Trinket or Gemma, suggest changing this to 1
#define LED_PIN 12
#define BUZZER_PIN 5
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 30 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

bool light_on = false;
bool sound_on = false;
struct t_mtab {
    char c, pat;
};

struct t_mtab morseTab[] = {
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
};

#define N_MORSE  (sizeof(morseTab)/sizeof(morseTab[0]))

#define SPEED  (12)
#define DOTLEN  (1200/SPEED)
#define DASHLEN  (3*(1200/SPEED))

void dash() {
    checkMessage();
    digitalWrite(LED_PIN, HIGH);
    if (light_on) {
        beacon_on();
    }
    delay(DASHLEN);
    digitalWrite(LED_PIN, LOW);
    if (light_on) {
        beacon_off();
    }
    delay(DOTLEN);
}

void dit() {
    checkMessage();
    digitalWrite(LED_PIN, HIGH);
    if (light_on) {
        beacon_on();
    }
    delay(DOTLEN);
    digitalWrite(LED_PIN, LOW);
    if (light_on) {
        beacon_off();
    }
    delay(DOTLEN);
}

void send(char c) {
    int i;
    if (c == ' ') {
        Serial.print(c);
        delay(7 * DOTLEN);
        return;
    }
    for (i = 0; i < N_MORSE; i++) {
        if (morseTab[i].c == c) {
            unsigned char p = morseTab[i].pat;
            Serial.print(morseTab[i].c);

            while (p != 1) {
                if (p & 1)
                    dash();
                else
                    dit();
                p = p / 2;
            }
            delay(2 * DOTLEN);
            return;
        }
    }
    /* if we drop off the end, then we send a space */
    Serial.print("?");
}

void sendMsg(char *str) {
    while (*str)
        send(*str++);
    Serial.println("");
}


//Radio stuff
RH_ASK driver(2000, 7, LED_PIN, BUZZER_PIN);
//Which will initialise the driver at 2000 bps, receive on GPIO7, transmit on GPIO12, PTT on GPIO5.
// This is very poorly documented. PTT is not needed, but whatever.

uint8_t buf[12];
uint8_t buflen = sizeof(buf);

void checkMessage() {
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        int i;
        // Message with a good checksum received, dump it.
        Serial.print("Message: "); // Listen for a message from ground control, if anything dump it (for now)
        Serial.println((char *) buf);
    }
}

//We can just as easily run Morse only, but the library is used to use the same transmitter to do morse AND packet. 

void beacon_on() {
    for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));

        pixels.show();   // Send the updated pixel colors to the hardware.
    }
}

void beacon_off() {
    pixels.clear();
    pixels.show();
}

int cycles = 0;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.begin(9600);
    Serial.println("Visual Beacon Start...");
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.clear();
    for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));

        pixels.show();   // Send the updated pixel colors to the hardware.
    }
    delay(3000);
    pixels.clear();
    pixels.show();
    if (!driver.init())
        Serial.println("init failed");
}

void loop() {
    if (cycles >= NUM_CYCLES) {
        light_on = true; //We're nearing the end of the flight, start up the recovery beacon!
    }
    if (cycles == NUM_CYCLES) {
        digitalWrite(LED_PIN, HIGH);
        delay(4000);
        digitalWrite(LED_PIN, LOW);
        sendMsg("LB ACTIVE");

    }
    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    char char_cyc = cycles;
    sendMsg("THIS IS LAB SES 1");
    cycles++;

}
