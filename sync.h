#ifndef __SYNC_H__
#define __SYNC_H__

#include <nRF24L01.h>
#include <RF24.h>

class SyncedCycle {
  
public:
  SyncedCycle();
  void tick();

  uint16_t cycle(uint16_t max, uint16_t rate);

private:
  uint16_t synced_time();
  
  int16_t  delta;
  uint16_t offset;
  RF24 radio;

};

#endif __SYNC_H__
