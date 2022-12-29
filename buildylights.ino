 #include <ArduinoJson.h>
#include <FastLED.h>

const size_t capacity = JSON_OBJECT_SIZE(3) + 40;
const char* json = "{\"command\":\"start\",\"seconds\":10,\"hue\":255}";

//this is for octomonk
#define NUM_LEDS 17

#define BEEP_PIN 8
#define DATA_PIN 5
#define FORMAT GRB
#define DURATION 10

#define MIN_PASS 0
#define MAX_PASS 3
#define MIN_FAIL 3
#define MAX_FAIL 7
#define MIN_PROD 7
#define MAX_PROD 13
#define FIRE_IDX 13

CRGB leds[NUM_LEDS];
CHSV currentColor = CHSV(200, 255, 255);


void setup() {
  delay(3000);
  FastLED.addLeds<WS2812B, DATA_PIN, FORMAT>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.show();
  
  pinMode(9, INPUT_PULLUP);
  Serial.begin(9600);
  while (!Serial) continue;
}

byte on = 1;
byte beepage = 0;
unsigned long stopAfter = 5000; // allow for a 5 second start
unsigned long startTime = 0;
typedef void (*SimplePatternList[])();
SimplePatternList buildPatterns = { pulse, chaserman, backNForth, pulse, chaserman, backNForth, strobe, gradient, glitterRainbow, confetti, juggle, bpm, rainbowBackNForth, Fire2012 };
uint8_t patternIdx = 0;
unsigned long seconds = DURATION;
uint8_t gHue = 0;
int buttonState = 0;
int lastState = 0;
int placeInPhase = 0;
int step = 0;
int lastStep = 0;
int boundaries[] = {300,550,755,920,1050,1150,1225,1280,1320,1350,1375,1400,1430,1470,1525,1600,1700,1830,1995,2200,2450,2750,3000};
//22 of these
void loop() {
  buttonState = digitalRead(9);

  //two states... if button is off, then turn it off
  // if the button is on (and was off before) then turn on
  // if the button is still on, dont change anything.

 // if (buttonState == LOW) {
    loop_octomonk();
 // }
 // else {
 //   loop_nightlight();
 // }
  lastState = buttonState;
}

// original loop... 
void loop_octomonk() {
  if (on) {
    if (millis() > stopAfter) {
       turnOff();
    }
    else {
       buildPatterns[patternIdx]();
       if (patternIdx >= MIN_PROD) {
          gHue++;
          FastLED.show();
          FastLED.delay(10);
       }
    }
  }
  else if (beepage) {
    // problems:   beepage should listen to beep off's.  maybe check if we have legit input, then do something???
    if (millis() > stopAfter) {
      noBeepage();
    }
    else {
    // determine phase
      placeInPhase = (millis() - startTime) % 6000;
      int step =-1;
      int duration = 0;
      for (byte i = 0; i < 22; i++) {
        if (placeInPhase < boundaries[i] ) {
          step = i;
          duration = boundaries[i+1] - boundaries[i];
          break;
        }
      }
      if (step > -1 && step != lastStep) {
        noTone(BEEP_PIN);
        int freq = random16(2500) + 300;
        tone(BEEP_PIN, freq, duration);
        lastStep = step;
      }
      else if (step == -1) {
        lastStep = -1;
        noTone(BEEP_PIN);

        // try to read here... 
        DynamicJsonDocument doc(capacity);
        DeserializationError err = deserializeJson(doc, Serial);
        if (!err) {
          const char* command = doc["command"]; // "start"
          if (strcmp("beep-off",command) == 0) {
            noBeepage();
          }   
        }
      }
    }
  }
  else {
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, Serial);
    if (err) {
      if (strcmp(err.c_str(), "InvalidInput") == 0) {
          printUsage();
      }
    }
    else {
      int jsonSeconds = doc["seconds"];
      if ( jsonSeconds > 0) {
        seconds = jsonSeconds;
      }
      else {
        seconds = DURATION;
      }
      
      const char* command = doc["command"]; // "start"
      if (strcmp("prod",command) == 0) {
         patternIdx = random8(MIN_PROD, MAX_PROD);
         turnOn();
      }
      else {
        if (strcmp("passed", command) == 0) {
          patternIdx = random8(MIN_PASS, MAX_PASS);
          int hue = doc["hue"];
          if (hue != 0) {
            currentColor = CHSV(hue, 225, 192);
          }
          else {
            currentColor = CHSV(90, 225, 192);
          }
          turnOn();
        }
        else if (strcmp("failed",command) == 0) {
          patternIdx = random8(MIN_FAIL, MAX_FAIL);
          currentColor = CHSV(0, 255, 255);
          turnOn();
        }
        else if (strcmp("fire",command) == 0) {
          patternIdx = FIRE_IDX;
          turnOn();
        }
        else if (strcmp("beep-on",command) == 0) {
           beepinate();
        }
        else if (strcmp("beep-off",command) == 0) {
           noBeepage();
        }
        else {
           printUsage();
        }
      }
    }  
  } 
} // end loop



// button loop
void loop_nightlight() {
  if (buttonState != lastState) {
    seconds = DURATION;
    currentColor = CHSV(random8(1, 254), 225, 192);
    patternIdx = random8(MIN_PASS, MAX_PROD);
    turnOn();
  }
  else {
    if (millis() > stopAfter) {
         //turnOff();
         seconds = DURATION;
         currentColor = CHSV(random8(1, 254), 225, 192);
         patternIdx = random8(MIN_PASS, MAX_PROD);
         turnOn();
      }
      else {
         buildPatterns[patternIdx]();
         if (patternIdx >= MIN_PROD) {
            gHue++;
            FastLED.show();
            FastLED.delay(10);
         }
      }
  }
} // end loop

void printUsage() {
   Serial.println("you silly carrot head, that's not how you use this!");
   Serial.println("Usage:");
   Serial.println("   {\"command\":\"passed\",seconds:10,hue:90} ");
   Serial.println("       or... ");
   Serial.println("   {\"command\":\"failed\",seconds:20} ");
   Serial.println("       or... ");
   Serial.println("   {\"command\":\"prod\",seconds:10} ");
   Serial.println("       or... ");
   Serial.println("   {\"command\":\"fire\"} ");
   Serial.println(" seconds are optional (defaults to 10), hue ignored on failed commands");
}

void beepinate() {
  beepage = 1;
  startTime = millis();
  stopAfter = startTime + 60000;
}

void noBeepage() {
  beepage = 0;
  noTone(BEEP_PIN);
}

void turnOn() {
  on = 1;
  stopAfter = millis() + (seconds * 1000);
}

void turnOff() {
  on = 0;
  FastLED.clear(true);
  FastLED.show();
}

void gradient() {
  for (int i = 0; i<NUM_LEDS; i++) {
     fill_gradient_RGB(leds, NUM_LEDS, CHSV(255 - gHue, 192, 192), CHSV(gHue, 192, 192));  
  }
}

void chaserman() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(30, 0, NUM_LEDS - 1);
  leds[pos] += currentColor;
  FastLED.show();
  FastLED.delay(10);
}

void pulse() {
  static uint8_t bright = 0;
  static int sign = 1;
  currentColor = CHSV(currentColor.h, currentColor.s, bright);
  fill_solid(leds, NUM_LEDS, currentColor);
  bright = bright + (2 * sign);
  if (bright <= 0 || bright >= 150) {
    sign = sign * -1;
  }
  if (bright >= 150) {
    FastLED.delay(100);
  }
 
  FastLED.delay(10);
}

void strobe() {
  static int sign = 1;
  if (sign < 0) {
     FastLED.clear(true);
     FastLED.show();
     FastLED.delay(100);
  }
  else {
     fill_solid(leds, NUM_LEDS, currentColor);
     FastLED.show();
     delay(150);
  }
  sign = sign * -1;
}

void glitterRainbow() {
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
  if (random8() < 80) {
    leds[ random16(NUM_LEDS) ]  += CRGB::White;
  }
}

void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void backNForth() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = currentColor;
    FastLED.show();
    fadeAll();
    delay(10);
  }
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    leds[i] = currentColor;
    FastLED.show();
    fadeAll();
    delay(10);
  }
}

void rainbowBackNForth() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(gHue, 255, 255);
    FastLED.show();
    fadeAll();
    delay(10);
  }
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    leds[i] = CHSV(gHue++, 255, 255);
    FastLED.show();
    fadeAll();
    delay(10);
  }
}

void fadeAll() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}


// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING  100

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120


void Fire2012()
{
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
      CRGB color = HeatColor( heat[j]);
      leds[j] = color;
    }
}
