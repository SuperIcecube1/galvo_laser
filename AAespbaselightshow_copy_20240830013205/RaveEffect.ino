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

void RaveEffect() {
    static uint8_t brightness = 255;  // Adjust as needed for brightness
    static unsigned long beat_drop_start_time = 0;  // Track when the beat drop started

    // BPM-based timing calculations (ensure they're not too slow/fast)
    int breath_speed = max(10, 3000 / (bpm / 2));  // Breathing speed at half BPM
    int waterfall_speed = max(10, 3000 / bpm);     // Waterfall speed at BPM
    int flash_speed = max(10, 12000 / bpm);        // Flashing speed at BPM (slowed down)
    int strobe_speed = max(10, 6000 / bpm);        // Strobe speed at BPM (slowed down)

    unsigned long current_time = millis();  // Get the current time in milliseconds

    // Check if there's a beat drop, and execute the beat drop effect
    if (is_beat_drop) {
        if (beat_drop_start_time == 0) {
            beat_drop_start_time = current_time;  // Record the start time of the beat drop effect
        }

        if (current_time - beat_drop_start_time <= 2300) {  // Check if 2.3 seconds have passed
            for (int i = 0; i < 10; i++) {  // Flash 10 times
                fill_solid(leds1, NUM_LEDS, CRGB::White);
                fill_solid(leds2, NUM_LEDS, CRGB::White);
                fill_solid(leds3, NUM_LEDS, CRGB::White);
                fill_solid(leds4, NUM_LEDS, CRGB::White);
                FastLED.show();
                delay(strobe_speed);  // Interfering delay

                fill_solid(leds1, NUM_LEDS, CRGB::Black);
                fill_solid(leds2, NUM_LEDS, CRGB::Black);
                fill_solid(leds3, NUM_LEDS, CRGB::Black);
                fill_solid(leds4, NUM_LEDS, CRGB::Black);
                FastLED.show();
                delay(strobe_speed / 2);  // Interfering delay
            }
        } else {
            // Reset the beat drop effect after 2.3 seconds
            is_beat_drop = 0;
            beat_drop_start_time = 0;
        }
    } else {
        beat_drop_start_time = 0;  // Reset the start time when the beat drop is not active

        // Handle different energy levels
        switch (energy_level) {
            case 0:
                // Energy Level 0: Breathing effect
                for (int i = 0; i < 255; i++) {
                    uint8_t breath_brightness = sin8(i);
                    fill_solid(leds1, NUM_LEDS, colors[0]);
                    fill_solid(leds2, NUM_LEDS, colors[1]);
                    fill_solid(leds3, NUM_LEDS, colors[2]);
                    fill_solid(leds4, NUM_LEDS, colors[3]);
                    FastLED.setBrightness(breath_brightness);
                    FastLED.show();
                    delay(breath_speed / 255);  // Interfering delay
                }
                break;

            case 1:
                // Energy Level 1: Waterfall up and down
                for (int i = 0; i < NUM_LEDS; i++) {
                    leds1[i] = colors[0];
                    leds2[NUM_LEDS - 1 - i] = colors[1];
                    leds3[i] = colors[2];
                    leds4[NUM_LEDS - 1 - i] = colors[3];
                    FastLED.show();
                    delay(waterfall_speed / NUM_LEDS);  // Interfering delay
                }
                for (int i = 0; i < NUM_LEDS; i++) {
                    leds1[i] = CRGB::Black;
                    leds2[NUM_LEDS - 1 - i] = CRGB::Black;
                    leds3[i] = CRGB::Black;
                    leds4[NUM_LEDS - 1 - i] = CRGB::Black;
                    FastLED.show();
                    delay(waterfall_speed / NUM_LEDS);  // Interfering delay
                }
                break;

            case 2:
                // Energy Level 2: Alternating flashing between strips
                fill_solid(leds1, NUM_LEDS, colors[0]);
                fill_solid(leds2, NUM_LEDS, CRGB::Black);
                fill_solid(leds3, NUM_LEDS, colors[2]);
                fill_solid(leds4, NUM_LEDS, CRGB::Black);
                FastLED.show();
                delay(flash_speed);  // Interfering delay

                fill_solid(leds1, NUM_LEDS, CRGB::Black);
                fill_solid(leds2, NUM_LEDS, colors[1]);
                fill_solid(leds3, NUM_LEDS, CRGB::Black);
                fill_solid(leds4, NUM_LEDS, colors[3]);
                FastLED.show();
                delay(flash_speed);  // Interfering delay
                break;

            case 3:
                // Energy Level 3: Strobing effect
                for (int i = 0; i < NUM_LEDS; i += 3) {
                    leds1[i] = leds1[i + 1] = leds1[i + 2] = colors[0];
                    leds2[i] = leds2[i + 1] = leds2[i + 2] = colors[1];
                    leds3[i] = leds3[i + 1] = leds3[i + 2] = colors[2];
                    leds4[i] = leds4[i + 1] = leds4[i + 2] = colors[3];
                }
                FastLED.show();
                delay(strobe_speed);  // Interfering delay

                fill_solid(leds1, NUM_LEDS, CRGB::Black);
                fill_solid(leds2, NUM_LEDS, CRGB::Black);
                fill_solid(leds3, NUM_LEDS, CRGB::Black);
                fill_solid(leds4, NUM_LEDS, CRGB::Black);
                FastLED.show();
                delay(strobe_speed);  // Interfering delay
                break;
        }
    }
}
