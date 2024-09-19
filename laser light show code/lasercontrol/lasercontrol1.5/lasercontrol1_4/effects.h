// effects.h

#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>

// Define laser and galvo pins (ensure these match with main.ino)
extern const int RED_LASER_PIN;
extern const int GREEN_LASER_PIN;
extern const int BLUE_LASER_PIN;
extern const int X_GALVO_PIN;
extern const int Y_GALVO_PIN;

// Global laser color variables
extern uint8_t laserRed;
extern uint8_t laserGreen;
extern uint8_t laserBlue;

// Function prototypes for effects with duration parameter
void effect1(unsigned long duration);
void effect2(unsigned long duration);
void effect3(unsigned long duration);
void effect4(unsigned long duration);
void effect5(unsigned long duration);
void effect6(unsigned long duration);
void effect7(unsigned long duration);
void effect8(unsigned long duration);
void effect9(unsigned long duration);
void effect10(unsigned long duration);
void effect11(unsigned long duration);
void effect12(unsigned long duration);
void effect13(unsigned long duration);
void effect14(unsigned long duration);
void effect15(unsigned long duration);
void effect16(unsigned long duration);
void effect17(unsigned long duration);
void effect18(unsigned long duration);
void effect19(unsigned long duration);
void effect20(unsigned long duration);

// Function to set laser color
void setLaserColor();
void setLaserColor(uint8_t red, uint8_t green, uint8_t blue);

#endif // EFFECTS_H

