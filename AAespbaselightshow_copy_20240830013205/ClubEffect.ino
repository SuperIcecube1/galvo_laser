  #include <FastLED.h>

  extern int bpm;
  extern int energy_level;
  extern int is_beat_drop;
  extern CRGB colors[];  // Array of colors for each strip
  extern CRGB leds1[];
  extern CRGB leds2[];
  extern CRGB leds3[];
  extern CRGB leds4[];
  #define NUM_LEDS 60  // Define NUM_LEDS to match the main file

  void ClubEffect() {
      static uint8_t brightness = 255;  // Base brightness level

      // Calculate the duration of a beat in milliseconds
      float beat_duration = 60000.0 / bpm;

      // Relative speed factors based on BPM
      int matrix_speed = beat_duration / 8;     // Matrix speed relative to BPM
      int waterfall_speed = beat_duration / 4;  // Waterfall speed relative to BPM
      int scanner_speed = beat_duration / 128;  // Scanner speed relative to BPM
      int lightning_speed = beat_duration / 32; // Lightning effect speed relative to BPM

      // Handle Beat Drop effect
      if (is_beat_drop) {
          for (int i = 0; i < 10; i++) {  // Flash 10 times
              fill_solid(leds1, NUM_LEDS, CRGB::White);
              fill_solid(leds2, NUM_LEDS, CRGB::White);
              fill_solid(leds3, NUM_LEDS, CRGB::White);
              fill_solid(leds4, NUM_LEDS, CRGB::White);
              FastLED.show();
              delay(beat_duration / 8);  // Short delay for flash effect

              fill_solid(leds1, NUM_LEDS, CRGB::Black);
              fill_solid(leds2, NUM_LEDS, CRGB::Black);
              fill_solid(leds3, NUM_LEDS, CRGB::Black);
              fill_solid(leds4, NUM_LEDS, CRGB::Black);
              FastLED.show();
              delay(beat_duration / 16);  // Short delay for flash effect
          }
          is_beat_drop = 0;  // Reset beat drop after effect
      }

      switch (energy_level) {
          case 0: {
              // Energy Level 0: Waterfall effect
              for (int i = 0; i < NUM_LEDS; i++) {
                  leds1[i] = colors[0];
                  leds2[NUM_LEDS - 1 - i] = colors[1];
                  leds3[i] = colors[2];
                  leds4[NUM_LEDS - 1 - i] = colors[3];
                  FastLED.show();
                  delay(waterfall_speed / NUM_LEDS);  // Relative delay
              }
              for (int i = 0; i < NUM_LEDS; i++) {
                  leds1[i] = CRGB::Black;
                  leds2[NUM_LEDS - 1 - i] = CRGB::Black;
                  leds3[i] = CRGB::Black;
                  leds4[NUM_LEDS - 1 - i] = CRGB::Black;
                  FastLED.show();
                  delay(waterfall_speed / NUM_LEDS);  // Relative delay
              }
              break;
          }

          case 1: {
              // Energy Level 1: Larson Scanner effect with 3 LEDs per dot and fading trail
              static int scanner_position1 = 0;
              static int scanner_position2 = NUM_LEDS / 3;
              static int direction1 = 1;
              static int direction2 = -1;

              // Fade out all LEDs slightly
              for (int i = 0; i < NUM_LEDS; i++) {
                  leds1[i].fadeToBlackBy(50);
                  leds2[i].fadeToBlackBy(50);
                  leds3[i].fadeToBlackBy(50);
                  leds4[i].fadeToBlackBy(50);
              }

              // First scanner dot with 3 LEDs
              leds1[scanner_position1] = leds1[scanner_position1 + 1] = leds1[scanner_position1 + 2] = colors[0];
              leds2[scanner_position1] = leds2[scanner_position1 + 1] = leds2[scanner_position1 + 2] = colors[1];

              // Second scanner dot (offset) with 3 LEDs
              leds3[scanner_position2] = leds3[scanner_position2 + 1] = leds3[scanner_position2 + 2] = colors[2];
              leds4[scanner_position2] = leds4[scanner_position2 + 1] = leds4[scanner_position2 + 2] = colors[3];

              FastLED.show();

              // Move the scanner positions forward
              scanner_position1 += direction1;
              scanner_position2 += direction2;

              if (scanner_position1 == NUM_LEDS - 3 || scanner_position1 == 0) {
                  direction1 = -direction1;  // Reverse direction at the ends
              }
              if (scanner_position2 == NUM_LEDS - 3 || scanner_position2 == 0) {
                  direction2 = -direction2;  // Reverse direction at the ends
              }

              delay(scanner_speed);  // Relative delay
              break;
          }

          case 2: {
              // Energy Level 2: Matrix effect with different patterns on each strip
              for (int i = 0; i < NUM_LEDS; i++) {
                  leds1[i] = (random8() > 200) ? colors[0] : CRGB::Black;
                  leds2[i] = (random8() > 150) ? colors[1] : CRGB::Black;
                  leds3[i] = (random8() > 100) ? colors[2] : CRGB::Black;
                  leds4[i] = (random8() > 50) ? colors[3] : CRGB::Black;
              }
              FastLED.show();
              delay(matrix_speed);  // Relative delay
              break;
          }

          case 3: {
              // Energy Level 3: Lightning effect with random portions lighting up
              for (int i = 0; i < 5; i++) {  // Flash multiple times for effect
                  int start_led1 = random16(NUM_LEDS - 10);
                  int start_led2 = random16(NUM_LEDS - 10);
                  int start_led3 = random16(NUM_LEDS - 10);
                  int start_led4 = random16(NUM_LEDS - 10);

                  for (int j = 0; j < 10; j++) {  // Light up random portions (10 LEDs)
                      leds1[start_led1 + j] = colors[0];
                      leds2[start_led2 + j] = colors[1];
                      leds3[start_led3 + j] = colors[2];
                      leds4[start_led4 + j] = colors[3];
                  }
                  FastLED.show();
                  delay(lightning_speed);  // Short flash duration

                  fill_solid(leds1, NUM_LEDS, CRGB::Black);
                  fill_solid(leds2, NUM_LEDS, CRGB::Black);
                  fill_solid(leds3, NUM_LEDS, CRGB::Black);
                  fill_solid(leds4, NUM_LEDS, CRGB::Black);
                  FastLED.show();
                  delay(random8(0, 10));  // Short pause between flashes
              }

              break;
          }
      }
  }
