#include <SPI.h>
#include <FastLED.h>

#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
FASTLED_USING_NAMESPACE
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL };              // Radio pipe addresses for the 2 nodes to communicate.

int16_t  delta = 0;
uint16_t offset = 0;

#define NUM_LEDS    30
CRGB leds[NUM_LEDS];

void setup(){

  Serial.begin(57600);
  printf_begin();
  
  FastLED.addLeds<WS2812 , 7, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(150);

  // Setup and configure rf radio

  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(0,0);                  // Smallest time between retries, max no. of retries
  radio.setPayloadSize(2);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void) {
  static unsigned long lastSent = millis();
  if (lastSent + 1000 < millis()) {
    lastSent += 1000;
    //radio.printDetails();                   // Dump the configuration of the rf unit for debugging
        
    //printf("0x%0.4x: Now sending; ",counter);
    unsigned long time = micros();                          // Take the time, and send it.  This will block until complete   
                                                            //Called when STANDBY-I mode is engaged (User is finished sending)
                                                            
    radio.stopListening();
    uint16_t toSend = cycle();
    if (!radio.write( &toSend, 2 )){
      Serial.println(F("failed."));      
    }
    radio.startListening();
  }
  static unsigned long lastTick = millis();
  if (lastTick + 20 < millis()) {
   lastTick += 20;
   int16_t correction = constrain((int16_t)delta/3, (int16_t)-255, (int16_t)255);

   printf("At: 0x%0.4x, delta: %d, offset: %d\n", cycle(), delta, offset);
   offset += correction;
   delta -= correction;
   rainbow();
   FastLED.show();
 }
 
  if( radio.available()){
    uint16_t gotWord;                                       // Dump the payloads until we've gotten everything
    radio.read( &gotWord , 2 );

    delta =  gotWord - cycle();

    printf("At: 0x%0.4x, received: 0x%0.4x, delta: %d\n", cycle(), gotWord, delta);
    
 } 
}



void rainbow() 
{
  int cycle_center = (cycle()  ) %  NUM_LEDS;
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, cycle(), 7);
  for(int i = 0;i < NUM_LEDS;++i) {
    int16_t delta = cycle_center - i;
    delta = abs(delta);
    leds[i] = leds[i].nscale8_video(((255 - 100) / delta) + (100 / delta));
  }
}



uint16_t cycle() {
  uint16_t cycle_size = 0xFFFF;
  uint32_t mic = micros();
  uint16_t loc = cycle_size & (mic >> 16);
  uint16_t cyc = loc+ offset;
  return cyc;
}



