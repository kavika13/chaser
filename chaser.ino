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
    rainbow();
    FastLED.show();
  }

  if ( radio.available()) {
    uint16_t gotWord;                                       // Dump the payloads until we've gotten everything
    radio.read( &gotWord , 2 );

    delta =  gotWord - synced_time();
    //printf("At: 0x%0.4x, received: 0x%0.4x, delta: %d\n", cycle(), gotWord, delta);

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

