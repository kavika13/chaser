/*
  // March 2014 - TMRh20 - Updated along with High Speed RF24 Library fork
  // Parts derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads 
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionally as well.
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to if wanting
 * to initiate communication instead of respond to a commmunication.
 */
 


#include <SPI.h>
#include <FastLED.h>

#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
FASTLED_USING_NAMESPACE
// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL };              // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles
role_e role = role_pong_back;                                              // The role of the current running sketch

// A single byte to keep track of the data being sent back and forth
uint16_t cycle = 1;
uint16_t rate = 1 << 8;
uint16_t nominal_rate = 1 << 8;
int16_t  delta = 0;

#define NUM_LEDS    2
CRGB leds[NUM_LEDS];

void setup(){

  Serial.begin(57600);
  printf_begin();
  Serial.print(F("\n\rRF24/examples/pingpair_ack/\n\rROLE: "));
  Serial.println(role_friendly_name[role]);
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
  
  FastLED.addLeds<WS2812 , 7, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(100);

  // Setup and configure rf radio

  radio.begin();
  //radio.setPALevel(RF24_PA_HIGH); 
  //radio.setDataRate(RF24_250KBPS);
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
                                                            
    radio.stopListening();                                  // First, stop listening so we can talk.
    if (!radio.write( &cycle, 2 )){
      Serial.println(F("failed."));      
    }
    radio.startListening();
  }

  // Pong back role.  Receive each packet, dump it out, and send it back
  while( radio.available()){
    uint16_t gotWord;                                       // Dump the payloads until we've gotten everything
    radio.read( &gotWord , 2 );

    delta =  gotWord - cycle;

    printf("At: 0x%0.4x, received: 0x%0.4x, rate: 0x%0.4x, delta: %d\n", cycle, gotWord, rate, delta);
    
 } 
   static unsigned long lastTick = millis();
  if (lastTick + 20 < millis()) {
   lastTick += 20;
   int16_t correction = sqrt(abs(delta /2) ) / 2;
   if(delta < 0) {
     correction = -correction;
   }
    
   rate = nominal_rate + correction;
   delta -= correction;
   cycle += rate;
   rainbow();
   FastLED.show();
 } // slowly cycle the "base color" through the rainbow

}



void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, cycle >> 8, 7);
}


