// effects.ino

#include "effects.h"
#include <math.h>

// Scaling factor for blue laser intensity (adjust between 0.0 and 1.0)
const float BLUE_LASER_SCALE = 0.5; // Adjust this value as needed

// Global variables for laser colors (set by webserver)
extern uint8_t laserRed;
extern uint8_t laserGreen;
extern uint8_t laserBlue;

// Helper function to set laser colors based on global variables
void setLaserColor() {
  analogWrite(RED_LASER_PIN, laserRed);
  analogWrite(GREEN_LASER_PIN, laserGreen);
  analogWrite(BLUE_LASER_PIN, (uint8_t)(laserBlue * BLUE_LASER_SCALE));
}

// Helper function to set laser colors with specific RGB values
void setLaserColor(uint8_t red, uint8_t green, uint8_t blue) {
  analogWrite(RED_LASER_PIN, red);
  analogWrite(GREEN_LASER_PIN, green);
  analogWrite(BLUE_LASER_PIN, (uint8_t)(blue * BLUE_LASER_SCALE));
}

// Function to turn off lasers
void turnOffLasers() {
  setLaserColor(0, 0, 0);
}

// ----------------------------------------
// Effect 1: Rotating Line
// ----------------------------------------
void effect1(unsigned long duration) {
  Serial.println("Effect 1: Rotating Line");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    for (int angle = 0; angle < 360; angle += 20) {
      int x = 128 + 127 * cos(radians(angle));
      int y = 128 + 127 * sin(radians(angle));

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  // Turn off lasers
  turnOffLasers();
}

// ----------------------------------------
// Effect 2: Expanding Circle
// ----------------------------------------
void effect2(unsigned long duration) {
  Serial.println("Effect 2: Expanding Circle");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    for (int radius = 10; radius <= 127; radius += 30) {
      for (int angle = 0; angle < 360; angle += 15) {
        int x = 128 + radius * cos(radians(angle));
        int y = 128 + radius * sin(radians(angle));

        setLaserColor(); // Use color from webserver

        dacWrite(X_GALVO_PIN, x);
        dacWrite(Y_GALVO_PIN, y);

        delayMicroseconds(1000);

        if (millis() - startTime >= duration) {
          break;
        }
      }
      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 3: Moving Dot
// ----------------------------------------
void effect3(unsigned long duration) {
  Serial.println("Effect 3: Moving Dot");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    for (int x = 0; x < 255; x += 50) {
      int y = 128;

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }

    for (int x = 255; x >= 0; x -= 5) {
      int y = 128;

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 4: Spiral
// ----------------------------------------
void effect4(unsigned long duration) {
  Serial.println("Effect 4: Spiral");

  unsigned long startTime = millis();
  int loops = 5;
  while (millis() - startTime < duration) {
    for (float angle = 0; angle < loops * 360; angle += 10) {
      float radius = (angle / (loops * 360)) * 127;
      int x = 128 + radius * cos(radians(angle));
      int y = 128 + radius * sin(radians(angle));

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 5: Bouncing Line
// ----------------------------------------
void effect5(unsigned long duration) {
  Serial.println("Effect 5: Bouncing Line");

  unsigned long startTime = millis();
  int y = 0;
  int delta = 5;
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (int x = 0; x < 255; x += 20) {
      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }

    y += delta;
    if (y >= 255 || y <= 0) {
      delta = -delta;
    }

    if (millis() - startTime >= duration) {
      break;
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 6: Random Dots
// ----------------------------------------
void effect6(unsigned long duration) {
  Serial.println("Effect 6: Random Dots");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    int x = random(0, 256);
    int y = random(0, 256);

    setLaserColor(); // Use color from webserver

    dacWrite(X_GALVO_PIN, x);
    dacWrite(Y_GALVO_PIN, y);

    delayMicroseconds(1000);

    if (millis() - startTime >= duration) {
      break;
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 7: Laser Grid
// ----------------------------------------
void effect7(unsigned long duration) {
  Serial.println("Effect 7: Laser Grid");

  unsigned long startTime = millis();
  int step = 50;
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (int x = 0; x <= 255; x += step) {
      for (int y = 0; y <= 255; y += step) {
        dacWrite(X_GALVO_PIN, x);
        dacWrite(Y_GALVO_PIN, y);

        delayMicroseconds(1000);

        if (millis() - startTime >= duration) {
          break;
        }
      }
      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 8: Laser Cross
// ----------------------------------------
void effect8(unsigned long duration) {
  Serial.println("Effect 8: Laser Cross");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    // Draw horizontal line
    for (int x = 0; x <= 255; x += 10) {
      int y = 128;
      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }

    // Draw vertical line
    for (int y = 0; y <= 255; y += 5) {
      int x = 128;
      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 9: Two Beams Shooting to Center
// ----------------------------------------
void effect9(unsigned long duration) {
  Serial.println("Effect 9: Two Beams Shooting to Center");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    // Set color from webserver
    setLaserColor();

    // Start positions
    int xLeft = 0;
    int yLeft = 128;
    int xRight = 255;
    int yRight = 128;

    // Number of steps to reach the center
    int steps = 128;

    for (int i = 0; i < steps; i++) {
      if (millis() - startTime >= duration) {
        break;
      }

      // Calculate new positions moving towards the center
      int newXLeft = xLeft + i;
      int newXRight = xRight - i;

      // Optional: Move up and down slightly for visual effect
      int newYLeft = 128 + (int)(20 * sin(radians(i * 5)));
      int newYRight = 128 + (int)(20 * sin(radians(i * 5)));

      // Move left beam
      dacWrite(X_GALVO_PIN, newXLeft);
      dacWrite(Y_GALVO_PIN, newYLeft);
      delayMicroseconds(1000);

      // Move right beam
      dacWrite(X_GALVO_PIN, newXRight);
      dacWrite(Y_GALVO_PIN, newYRight);
      delayMicroseconds(1000);
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 10: Rapid Horizontal Sweep (Solid Line Illusion)
// ----------------------------------------
void effect10(unsigned long duration) {
  Serial.println("Effect 10: Rapid Horizontal Sweep (Solid Line Illusion)");

  unsigned long startTime = millis();

  // Parameters for sweep
  const int STEP_SIZE = 5;               // Step size for x-axis movement
  const unsigned long DELAY_US = 100;    // Delay in microseconds (100 µs)

  while (millis() - startTime < duration) {
    // Sweep Left to Right
    for (int x = 0; x <= 255; x += STEP_SIZE) {
      if (millis() - startTime >= duration) {
        break;
      }

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, 128); // Fixed Y position for horizontal line

      delayMicroseconds(DELAY_US); // Reduced delay for faster sweep
    }

    // Sweep Right to Left
    for (int x = 255; x >= 0; x -= STEP_SIZE) {
      if (millis() - startTime >= duration) {
        break;
      }

      setLaserColor(); // Use color from webserver

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, 128); // Fixed Y position for horizontal line

      delayMicroseconds(DELAY_US); // Reduced delay for faster sweep
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 11: Pulsing Circle
// ----------------------------------------
void effect11(unsigned long duration) {
  Serial.println("Effect 11: Pulsing Circle");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    // Expanding
    for (int radius = 10; radius <= 127; radius += 5) {
      for (int angle = 0; angle < 360; angle += 10) {
        int x = 128 + radius * cos(radians(angle));
        int y = 128 + radius * sin(radians(angle));

        setLaserColor(); // Use color from webserver

        dacWrite(X_GALVO_PIN, x);
        dacWrite(Y_GALVO_PIN, y);

        delay(1); // Fixed delay

        if (millis() - startTime >= duration) {
          break;
        }
      }
      if (millis() - startTime >= duration) {
        break;
      }
    }

    // Contracting
    for (int radius = 127; radius >= 10; radius -= 5) {
      for (int angle = 0; angle < 360; angle += 10) {
        int x = 128 + radius * cos(radians(angle));
        int y = 128 + radius * sin(radians(angle));

        setLaserColor(); // Use color from webserver

        dacWrite(X_GALVO_PIN, x);
        dacWrite(Y_GALVO_PIN, y);

        delayMicroseconds(1000);

        if (millis() - startTime >= duration) {
          break;
        }
      }
      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 12: Expanding Squares
// ----------------------------------------
void effect12(unsigned long duration) {
  Serial.println("Effect 12: Expanding Squares");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    for (int size = 10; size <= 127; size += 5) {
      setLaserColor(); // Use color from webserver

      int x1 = 128 - size;
      int y1 = 128 - size;
      int x2 = 128 + size;
      int y2 = 128 + size;

      // Draw square
      dacWrite(X_GALVO_PIN, x1);
      dacWrite(Y_GALVO_PIN, y1);
      delayMicroseconds(1000);

      dacWrite(X_GALVO_PIN, x2);
      dacWrite(Y_GALVO_PIN, y1);
      delayMicroseconds(1000);

      dacWrite(X_GALVO_PIN, x2);
      dacWrite(Y_GALVO_PIN, y2);
      delayMicroseconds(1000);

      dacWrite(X_GALVO_PIN, x1);
      dacWrite(Y_GALVO_PIN, y2);
      delayMicroseconds(1000);

      dacWrite(X_GALVO_PIN, x1);
      dacWrite(Y_GALVO_PIN, y1);
      delayMicroseconds(1000);
      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 13: Laser Star
// ----------------------------------------
void effect13(unsigned long duration) {
  Serial.println("Effect 13: Laser Star");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (int i = 0; i < 5; i++) {
      float angle1 = radians(i * 360 / 5);
      float angle2 = radians((i + 2) * 360 / 5);

      int x1 = 128 + 100 * cos(angle1);
      int y1 = 128 + 100 * sin(angle1);
      int x2 = 128 + 100 * cos(angle2);
      int y2 = 128 + 100 * sin(angle2);

      // Draw line from (x1, y1) to (x2, y2)
      dacWrite(X_GALVO_PIN, x1);
      dacWrite(Y_GALVO_PIN, y1);
      delayMicroseconds(2000);

      dacWrite(X_GALVO_PIN, x2);
      dacWrite(Y_GALVO_PIN, y2);
      delayMicroseconds(2000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 14: Rotating Squares
// ----------------------------------------
void effect14(unsigned long duration) {
  Serial.println("Effect 14: Rotating Squares");

  unsigned long startTime = millis();
  float angle = 0;
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    int size = 50;
    int x1 = 128 + size * cos(radians(angle));
    int y1 = 128 + size * sin(radians(angle));

    int x2 = 128 + size * cos(radians(angle + 90));
    int y2 = 128 + size * sin(radians(angle + 90));

    int x3 = 128 + size * cos(radians(angle + 180));
    int y3 = 128 + size * sin(radians(angle + 180));

    int x4 = 128 + size * cos(radians(angle + 270));
    int y4 = 128 + size * sin(radians(angle + 270));

    // Draw square
    dacWrite(X_GALVO_PIN, x1);
    dacWrite(Y_GALVO_PIN, y1);
    delayMicroseconds(2000);

    dacWrite(X_GALVO_PIN, x2);
    dacWrite(Y_GALVO_PIN, y2);
    delayMicroseconds(2000);

    dacWrite(X_GALVO_PIN, x3);
    dacWrite(Y_GALVO_PIN, y3);
    delayMicroseconds(2000);

    dacWrite(X_GALVO_PIN, x4);
    dacWrite(Y_GALVO_PIN, y4);
    delayMicroseconds(2000);

    dacWrite(X_GALVO_PIN, x1);
    dacWrite(Y_GALVO_PIN, y1);
    delayMicroseconds(2000);

    angle += 5;
    if (angle >= 360) {
      angle = 0;
    }

    if (millis() - startTime >= duration) {
      break;
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 15: 3D Rotating Cube
// ----------------------------------------
void effect15(unsigned long duration) {
  Serial.println("Effect 15: 3D Rotating Cube");

  // Define the 8 vertices of the cube
  float vertices[8][3] = {
    {-1, -1, -1},
    {1, -1, -1},
    {1, 1, -1},
    {-1, 1, -1},
    {-1, -1, 1},
    {1, -1, 1},
    {1, 1, 1},
    {-1, 1, 1}
  };

  // Define the 12 edges of the cube by connecting vertices
  int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Back face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Front face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
  };

  unsigned long startTime = millis();
  float angleX = 0.02; // Rotation speed around X-axis
  float angleY = 0.02; // Rotation speed around Y-axis
  float angleZ = 0.02; // Rotation speed around Z-axis

  while (millis() - startTime < duration) {
    // Create an array to hold the projected 2D points
    int projected[8][2];

    // Rotate and project each vertex
    for (int i = 0; i < 8; i++) {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];

      // Rotate around X-axis
      float sinX = sin(angleX);
      float cosX = cos(angleX);
      float y1 = y * cosX - z * sinX;
      float z1 = y * sinX + z * cosX;

      // Rotate around Y-axis
      float sinY = sin(angleY);
      float cosY = cos(angleY);
      float x2 = x * cosY + z1 * sinY;
      float z2 = -x * sinY + z1 * cosY;

      // Rotate around Z-axis
      float sinZ = sin(angleZ);
      float cosZ = cos(angleZ);
      float x3 = x2 * cosZ - y1 * sinZ;
      float y3 = x2 * sinZ + y1 * cosZ;

      // Perspective projection
      float distance = 3.0;
      float fov = 1.0;
      float factor = fov / (distance - z2);
      int px = 128 + (int)(x3 * factor * 100); // Scale to fit galvo range
      int py = 128 + (int)(y3 * factor * 100); // Scale to fit galvo range

      // Clamp values to 0-255
      px = constrain(px, 0, 255);
      py = constrain(py, 0, 255);

      projected[i][0] = px;
      projected[i][1] = py;
    }

    // Set laser color from webserver
    setLaserColor();

    // Draw all edges
    for (int i = 0; i < 12; i++) {
      int start = edges[i][0];
      int end = edges[i][1];

      // Draw line from projected[start] to projected[end]
      dacWrite(X_GALVO_PIN, projected[start][0]);
      dacWrite(Y_GALVO_PIN, projected[start][1]);
      delayMicroseconds(1000);

      dacWrite(X_GALVO_PIN, projected[end][0]);
      dacWrite(Y_GALVO_PIN, projected[end][1]);
      delayMicroseconds(1000);
    }

    // Update rotation angles
    angleX += 0.02;
    angleY += 0.02;
    angleZ += 0.02;

    // Keep angles within 0-2*PI
    if (angleX >= TWO_PI) angleX -= TWO_PI;
    if (angleY >= TWO_PI) angleY -= TWO_PI;
    if (angleZ >= TWO_PI) angleZ -= TWO_PI;
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 16: Waving Line
// ----------------------------------------
void effect16(unsigned long duration) {
  Serial.println("Effect 16: Waving Line");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (int x = 0; x <= 255; x += 20) {
      int y = 128 + 50 * sin(radians(x * 3));

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 17: Laser Infinity Symbol
// ----------------------------------------
void effect17(unsigned long duration) {
  Serial.println("Effect 17: Laser Infinity Symbol");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (float t = 0; t <= 2 * PI; t += 0.1) {
      float r = 50 * sin(t);
      int x = 128 + r * cos(t);
      int y = 128 + r * sin(t) * cos(t);

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 18: Laser Flower
// ----------------------------------------
void effect18(unsigned long duration) {
  Serial.println("Effect 18: Laser Flower");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (float angle = 0; angle < 360; angle += 5) {
      float rad = radians(angle);
      float r = 50 * sin(5 * rad);

      int x = 128 + r * cos(rad);
      int y = 128 + r * sin(rad);

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 19: Laser Butterfly
// ----------------------------------------
void effect19(unsigned long duration) {
  Serial.println("Effect 19: Laser Butterfly");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    for (float t = 0; t <= 12 * PI; t += 0.1) {
      float r = exp(cos(t)) - 2 * cos(4 * t) + pow(sin(t / 12), 5);
      int x = 128 + 20 * r * sin(t);
      int y = 128 + 20 * r * cos(t);

      dacWrite(X_GALVO_PIN, x);
      dacWrite(Y_GALVO_PIN, y);

      delayMicroseconds(1000);

      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}

// ----------------------------------------
// Effect 20: Fireworks
// ----------------------------------------
void effect20(unsigned long duration) {
  Serial.println("Effect 20: Fireworks");

  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    setLaserColor(); // Use color from webserver

    int centerX = 128;
    int centerY = 128;

    for (int i = 0; i < 360; i += 10) {
      float angle = radians(i);
      for (int r = 0; r < 128; r += 10) {
        int x = centerX + r * cos(angle);
        int y = centerY + r * sin(angle);

        dacWrite(X_GALVO_PIN, x);
        dacWrite(Y_GALVO_PIN, y);

        delayMicroseconds(1000); // Fixed delay

        if (millis() - startTime >= duration) {
          break;
        }
      }
      if (millis() - startTime >= duration) {
        break;
      }
    }
  }

  turnOffLasers();
}
