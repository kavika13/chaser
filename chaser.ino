#include <Arduino.h>

#include <SPI.h>
#include <FastLED.h>

#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
FASTLED_USING_NAMESPACE
RF24 radio(9, 10);

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL };              // Radio pipe addresses for the 2 nodes to communicate.

int16_t  delta = 0;
uint16_t offset = 0;

#define NUM_LEDS  30
CRGB leds[NUM_LEDS];

void rainbow();
uint16_t cycle(uint16_t max, uint16_t rate);
uint16_t synced_time();

void setup() {

  Serial.begin(57600);
  printf_begin();

  FastLED.addLeds<WS2812 , 7, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(150);

  // Setup and configure rf radio

  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(0, 0);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(2);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  
}

void loop(void) {
  static unsigned long lastSent = millis();
  static unsigned long frames = 0;
  frames++;
  if (lastSent + 1000 < millis()) {
    lastSent += 1000;
    printf("frames: %d\n", frames);
    frames = 0;
    //radio.printDetails();                   // Dump the configuration of the rf unit for debugging

    radio.stopListening();
    uint16_t toSend = synced_time();
    if (!radio.write( &toSend, 2 )) {
      Serial.println(F("failed."));
    }
    radio.startListening();
  }

  static unsigned long lastTick = millis();
  if (lastTick + 20 < millis()) {
    lastTick += 20;
    int16_t correction = constrain((int16_t)delta / 2, (int16_t) - 5, (int16_t)5);

    printf("At: 0x%0.4x, delta: %d, offset: %d\n", synced_time(), delta, offset);
    offset += correction;
    delta -= correction;
    if (cycle(256, 0) > 128) {
      rainbow();
    } else {
      fire();
    }
    FastLED.show();
  }

  if ( radio.available()) {
    uint16_t gotWord;                                       // Dump the payloads until we've gotten everything
    radio.read( &gotWord , 2 );

    delta =  gotWord - synced_time();
    //printf("At: 0x%0.4x, received: 0x%0.4x, delta: %d\n", cycle(), gotWord, delta);

  }
}

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  70

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 60

void fire()
{
  fadeToBlackBy(leds, NUM_LEDS, 20);

  uint16_t now = cycle(255, 0);
  static uint16_t lastAt = now;
  if (now < lastAt) {
    random16_set_seed(0);
  }
  lastAt = now;
    
  uint8_t hue = cycle(255, 3);
  hue++;
  CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
  CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
  CRGBPalette16 gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);
 
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);

      leds[j] = leds[j] + ColorFromPalette( gPal, colorindex);
    }
}

void rainbow()
{
  blur1d(leds, NUM_LEDS, 64);
  fadeToBlackBy(leds, NUM_LEDS, 2);

  uint16_t cycle_center = cycle(NUM_LEDS * 2, 4);
  //uint16_t cycle_center = corrected_cycle %  (NUM_LEDS*2);
  //printf("corrected_cycle: %0.4x, cycle_center: %d\n", corrected_cycle, cycle_center);
  if(cycle_center >= NUM_LEDS) {
    cycle_center = NUM_LEDS - (cycle_center % (NUM_LEDS)) - 1;
  }

  CRGB color;
  fill_rainbow(&color, 1,  cycle(256, 5), 12);
  leds[cycle_center] = leds[cycle_center] + color;
}

uint16_t synced_time() {
  uint16_t cycle_size = 0xFFFF;
  uint32_t mic = micros();
  uint16_t loc = cycle_size & (mic >> 10);
  uint16_t cyc = loc + offset;
  return cyc;
}

uint16_t cycle(uint16_t max, uint16_t rate) {
  uint16_t rate_mask = 0xFFFF >> rate;
  float corrected_cycle = synced_time() % rate_mask;
  corrected_cycle = corrected_cycle / rate_mask;

  return (uint16_t)(corrected_cycle * max) ;
}

