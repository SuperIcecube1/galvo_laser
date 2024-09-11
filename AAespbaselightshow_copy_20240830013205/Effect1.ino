#include <FastLED.h>

extern float volume_level;  // Volume level from the server
extern CRGB colors[];  // Array of colors for each strip, from the web server
extern CRGB leds1[];
extern CRGB leds2[];
extern CRGB leds3[];
extern CRGB leds4[];
#define NUM_LEDS 60  // Define NUM_LEDS to match the main file

// Variables for smoothing and non-linear ramping
float current_brightness = 0;  // Track the current brightness
float ramp_up_speed = 0.15;    // Speed at which brightness increases (higher = faster)
float ramp_down_speed = 0.05;  // Speed at which brightness decreases (higher = faster)

void Effect1() {
    // Apply threshold: if volume is less than 0.1, map brightness to 0
    float target_brightness;
    if (volume_level < 0.1) {
        target_brightness = 0;
    } else {
        // Map volume level to brightness in the range of 0 to 255
        target_brightness = map(volume_level * 1000, 100, 500, 0, 255);  // 500 corresponds to 0.5 in volume level
    }

    // Adjust brightness more quickly for ramp-up and more slowly for ramp-down
    if (target_brightness > current_brightness) {
        current_brightness += (target_brightness - current_brightness) * ramp_up_speed;
    } else {
        current_brightness += (target_brightness - current_brightness) * ramp_down_speed;
    }

    // Apply the colors from the server to the LED strips
    fill_solid(leds1, NUM_LEDS, colors[0]);  // Use the first color from the server
    fill_solid(leds2, NUM_LEDS, colors[1]);  // Use the second color from the server
    fill_solid(leds3, NUM_LEDS, colors[2]);  // Use the third color from the server
    fill_solid(leds4, NUM_LEDS, colors[3]);  // Use the fourth color from the server

    // Set the smoothed brightness
    FastLED.setBrightness(current_brightness);

    // Show the updated LED strip colors and brightness
    FastLED.show();

    delay(10);  // Control the speed of updates (faster for quick reaction)
}
