// effects.h

#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>

// Define DAC channels based on ESP32 pins
#define DAC1_CHANNEL 0 // GPIO25
#define DAC2_CHANNEL 1 // GPIO26

// Declare external variables for laser control pins
extern const int X_GALVO_PIN;
extern const int Y_GALVO_PIN;
extern const int RED_LASER_PIN;
extern const int GREEN_LASER_PIN;
extern const int BLUE_LASER_PIN;

// Function prototypes for effects
void effect1(); // Draws a square with predefined colors
void effect2(); // Draws a heart with predefined colors

// Empty effects from effect3 to effect20
void effect3();
void effect4();
void effect5();
void effect6();
void effect7();
void effect8();
void effect9();
void effect10();
void effect11();
void effect12();
void effect13();
void effect14();
void effect15();
void effect16();
void effect17();
void effect18();
void effect19();
void effect20();

#endif
