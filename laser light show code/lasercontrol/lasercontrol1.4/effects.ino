// effects.ino

#include "effects.h"

// Effect 1: Draw a square with predefined colors for 1 second
void effect1() {
  Serial.println("Effect 1: Drawing Square with Predefined Colors");

  // Set predefined laser colors (Example: Red, Green, Blue)
  analogWrite(RED_LASER_PIN, 255);    // Full intensity red
  analogWrite(GREEN_LASER_PIN, 0);    // No green
  analogWrite(BLUE_LASER_PIN, 0);     // No blue

  // Define square corners (example values; adjust based on your hardware)
  struct Point {
    int x;
    int y;
  };

  Point squareCorners[4] = {
    {0, 0}, // Top-left
    {0, 255}, // Bottom-left
    {255, 255}, // Bottom-right
    {255, 0}  // Top-right
  };

  // Get the current time
  unsigned long startTime = millis();

  // Loop until 1 second has passed
  while (millis() - startTime < 1000) {
    for (int i = 0; i < 4; i++) {
      // Move galvos to each corner of the square
      dacWrite(X_GALVO_PIN, squareCorners[i].x); // X-axis
      dacWrite(Y_GALVO_PIN, squareCorners[i].y); // Y-axis
      vTaskDelay(3 / portTICK_PERIOD_MS); // Adjust delay as needed for smooth movement
    }

    // Return to starting position
    dacWrite(X_GALVO_PIN, squareCorners[0].x);
    dacWrite(Y_GALVO_PIN, squareCorners[0].y);
    vTaskDelay(3 / portTICK_PERIOD_MS);
  }

  // Turn off the lasers after 1 second
  analogWrite(RED_LASER_PIN, 0);
  analogWrite(GREEN_LASER_PIN, 0);
  analogWrite(BLUE_LASER_PIN, 0);
}

// Effect 2: Draw a heart in predefined colors with pulsing effect for 1 second
void effect2() {
  Serial.println("Effect 2: Drawing Heart with Predefined Pulsing Colors");

  // Laser control pins
  int redLaserPin = RED_LASER_PIN;
  int blueLaserPin = BLUE_LASER_PIN;
  int greenLaserPin = GREEN_LASER_PIN;

  // Set initial laser color to purple
  analogWrite(redLaserPin, 255);    // Full intensity red
  analogWrite(greenLaserPin, 0);    // No green
  analogWrite(blueLaserPin, 255);   // Full intensity blue

  // Heart drawing parameters
  int maxDacValue = 255;    // Maximum DAC value (for 3.3V)
  int minDacValue = 0;      // Minimum DAC value (for 0V)
  int numPoints = 35;       // Number of points to draw the heart
  float baseScaleFactor = 9.0; // Base scale factor for the heart shape
  int delayTime = 1;        // Delay time between drawing points (in milliseconds)
  int beatInterval = 1000;  // 60 BPM -> 1 beat per second (1000ms)

  // Initialize last beat time
  static unsigned long lastBeatTime = 0;

  // Get the current time
  unsigned long currentTime = millis();

  // Calculate elapsed time since last beat
  float elapsedTime = (currentTime - lastBeatTime) % beatInterval;
  float beatProgress = elapsedTime / beatInterval;  // Beat progress from 0.0 to 1.0
  float pulseScaleFactor = baseScaleFactor + 2.0 * sin(beatProgress * 2.0 * PI); // Adjusted pulse

  // Loop to draw the heart shape continuously with the pulsing effect for 1 second
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    for (int i = 0; i < numPoints; i++) {
      float t = (float)i / numPoints * 2.0 * PI;  // Parametric variable t from 0 to 2π

      // Parametric equations for the heart shape
      float x = 16 * pow(sin(t), 3);
      float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);

      // Apply a 90-degree counterclockwise rotation
      float rotatedX = -y;
      float rotatedY = x;

      // Scale the heart shape to fit the DAC range and include the pulsing effect
      int dacXValue = constrain(map(rotatedX * pulseScaleFactor, -27 * baseScaleFactor, 27 * baseScaleFactor, minDacValue, maxDacValue), minDacValue, maxDacValue);
      int dacYValue = constrain(map(rotatedY * pulseScaleFactor, -26 * baseScaleFactor, 26 * baseScaleFactor, minDacValue, maxDacValue), minDacValue, maxDacValue);

      // Change laser colors over time based on the point index
      if (i < numPoints / 3) {
        // First third of the heart: Red laser
        analogWrite(redLaserPin, 255);    // Full intensity red
        analogWrite(blueLaserPin, 0);     // No blue
        analogWrite(greenLaserPin, 0);    // No green
      } else if (i < (2 * numPoints) / 3) {
        // Middle third of the heart: Green laser
        analogWrite(redLaserPin, 0);      // No red
        analogWrite(blueLaserPin, 0);     // No blue
        analogWrite(greenLaserPin, 255);  // Full intensity green
      } else {
        // Last third of the heart: Blue laser
        analogWrite(redLaserPin, 0);      // No red
        analogWrite(blueLaserPin, 255);   // Full intensity blue
        analogWrite(greenLaserPin, 0);    // No green
      }

      // Write the X and Y values to the DAC
      dacWrite(X_GALVO_PIN, dacXValue);  // X-axis (DAC1, GPIO25)
      dacWrite(Y_GALVO_PIN, dacYValue);  // Y-axis (DAC2, GPIO26)

      // Delay for smooth drawing
      vTaskDelay(delayTime / portTICK_PERIOD_MS);
    }

    // Optionally, you can add a small pause before the next iteration
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  // Turn off all lasers after drawing the heart
  analogWrite(redLaserPin, 0);
  analogWrite(blueLaserPin, 0);
  analogWrite(greenLaserPin, 0);

  // Update last beat time
  lastBeatTime = currentTime;
}

// Empty effect functions from effect3 to effect20
void effect3() {
  unsigned long startTime = millis();  // Get the current time when the effect starts

  while (millis() - startTime < 2000) {  // Loop for 2000 milliseconds (2 seconds)
    unsigned long currentTime = millis();
    
    // Parameters for wave motion
    float waveFrequency = 0.05; // Adjust for wave speed
    float waveAmplitude = 100;  // Adjust for wave height

    // Colors cycle through rainbow spectrum
    int redIntensity = sin(currentTime * 0.001) * 127 + 128;
    int greenIntensity = sin(currentTime * 0.001 + 2.0) * 127 + 128;
    int blueIntensity = sin(currentTime * 0.001 + 4.0) * 127 + 128;

    // X and Y wave motion based on sine function for smooth wave-like movement
    int xPos = waveAmplitude * sin(waveFrequency * currentTime) + 128;
    int yPos = waveAmplitude * cos(waveFrequency * currentTime) + 128;

    // Set laser color and galvo position
    analogWrite(RED_LASER_PIN, redIntensity);
    analogWrite(GREEN_LASER_PIN, greenIntensity);
    analogWrite(BLUE_LASER_PIN, blueIntensity);
    
    dacWrite(X_GALVO_PIN, xPos);
    dacWrite(Y_GALVO_PIN, yPos);

    delay(3); // Adjust for smooth transitions
  }

  // After 2 seconds, turn off lasers
  analogWrite(RED_LASER_PIN, 0);
  analogWrite(GREEN_LASER_PIN, 0);
  analogWrite(BLUE_LASER_PIN, 0);
}


void effect4() {
  // Define the X positions for the 6 beams
  int beamXPositions[6] = {10, 50, 90, 130, 170, 210};

  // Define the colors for each beam
  int beamColors[6][3] = {
    {255, 0, 0},    // Red
    {0, 255, 0},    // Green
    {0, 0, 255},    // Blue
    {255, 255, 0},  // Yellow
    {255, 0, 255},  // Magenta
    {0, 255, 255}   // Cyan
  };

  // Loop to display beams for 5 seconds
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {  // Run the effect for 5 seconds
    unsigned long currentTime = millis();
    
    // Calculate Y position to pan up and down (sin wave)
    int yPos = 128 + 127 * sin(0.005 * currentTime); // Y moves between 0 and 255
    
    // Loop through all 6 beams
    for (int i = 0; i < 6; i++) {
      // Set the color for the current beam
      analogWrite(RED_LASER_PIN, beamColors[i][0]);
      analogWrite(GREEN_LASER_PIN, beamColors[i][1]);
      analogWrite(BLUE_LASER_PIN, beamColors[i][2]);

      // Set the X position for the current beam
      dacWrite(X_GALVO_PIN, beamXPositions[i]);

      // Set the Y position for the current beam
      dacWrite(Y_GALVO_PIN, yPos);

      delay(1);  // Adjust delay to control the speed of switching beams
    }
  }

  // Turn off lasers after 5 seconds
  analogWrite(RED_LASER_PIN, 0);
  analogWrite(GREEN_LASER_PIN, 0);
  analogWrite(BLUE_LASER_PIN, 0);
}


void effect5() {
  // Empty effect
}

void effect6() {
  // Empty effect
}

void effect7() {
  // Empty effect
}

void effect8() {
  // Empty effect
}

void effect9() {
  // Empty effect
}

void effect10() {
  // Empty effect
}

void effect11() {
  // Empty effect
}

void effect12() {
  // Empty effect
}

void effect13() {
  // Empty effect
}

void effect14() {
  // Empty effect
}

void effect15() {
  // Empty effect
}

void effect16() {
  // Empty effect
}

void effect17() {
  // Empty effect
}

void effect18() {
  // Empty effect
}

void effect19() {
  // Empty effect
}

void effect20() {
  // Empty effect
}
