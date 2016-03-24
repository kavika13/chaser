#include "sync.h"

const uint64_t pipes[2] = { 0xABCDABCD71LL };              // Radio pipe addresses for the 2 nodes to communicate.

SyncedCycle::SyncedCycle() : radio(9,10), delta(0), offset(0) {
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(0, 0);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(2);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void SyncedCycle::tick() {
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
  }

  if ( radio.available()) {
    uint16_t gotWord;                                       // Dump the payloads until we've gotten everything
    radio.read( &gotWord , 2 );

    delta =  gotWord - synced_time();
    //printf("At: 0x%0.4x, received: 0x%0.4x, delta: %d\n", cycle(), gotWord, delta);

  }  
}

uint16_t SyncedCycle::synced_time() {
  uint16_t cycle_size = 0xFFFF;
  uint32_t mic = micros();
  uint16_t loc = cycle_size & (mic >> 10);
  uint16_t cyc = loc + offset;
  return cyc;
}

uint16_t SyncedCycle::cycle(uint16_t max, uint16_t rate) {
  uint16_t rate_mask = 0xFFFF >> rate;
  float corrected_cycle = synced_time() % rate_mask;
  corrected_cycle = corrected_cycle / rate_mask;

  return (uint16_t)(corrected_cycle * max) ;
}


