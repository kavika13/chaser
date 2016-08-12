#include <Arduino.h>

#include <SPI.h>
#include <FastLED.h>

#include "sync.h"

#include "printf.h"
FASTLED_USING_NAMESPACE

// Topology

#define NUM_LEDS  30
CRGB leds[NUM_LEDS];

SyncedCycle *sync;
void rainbow();
void setup() {

  Serial.begin(57600);
  printf_begin();

  FastLED.addLeds<WS2812 , 7, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(50);

  sync = new SyncedCycle();
  
}

void loop(void) {
  sync->tick();
  static unsigned long lastTick = millis();
  if (lastTick + 20 < millis()) {
    lastTick += 20;

    rainbow();
    FastLED.show();
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

  uint16_t now = sync->cycle(255, 0);
  static uint16_t lastAt = now;
  if (now < lastAt) {
    random16_set_seed(0);
  }
  lastAt = now;
    
  uint8_t hue = sync->cycle(255, 3);
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

  uint16_t cycle_center = sync->cycle(NUM_LEDS * 2, 4);
  //uint16_t cycle_center = corrected_cycle %  (NUM_LEDS*2);
  //printf("corrected_cycle: %0.4x, cycle_center: %d\n", corrected_cycle, cycle_center);
  if(cycle_center >= NUM_LEDS) {
    cycle_center = NUM_LEDS - (cycle_center % (NUM_LEDS)) - 1;
  }

  CRGB color;
  fill_rainbow(&color, 1,  sync->cycle(256, 5), 12);
  leds[cycle_center] = leds[cycle_center] + color;
}



