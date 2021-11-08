#include <LedControl.h>
#include "animation.h"

// A 2D point. Duh.
struct Point {
  int x;
  int y;
};

/* LedControl instance.
 * Pin 12 is DIN, Pin 11 is CS, Pin 10 is CLK
 * Two daisy-chained MAX7221 are attached
 */  
LedControl leds=LedControl(12,10,11,2);  

// Constants for which LED device is the right and left eye.
const int LEFT=0;
const int RIGHT=1;

// Default LED intensity.
const int DEFAULT_INTENSITY=2;

// Whether to invert the canvas when drawing to the LEDs
// My second batch of 8x8 matrices seemd to have 0,0 in
// the bottom right corner instead of top-left.
const bool INVERT=true;

// Number of rows in the matrix
// While the display size is fixed at 8x8, this avoids
// a need to pass array length when we can't use sizeof.
const int MATRIX_ROWS=8;

// Display buffers for the eyes, so we can do rudimentary
// compositing before outputting to the LED arrays.
// Each is in row-major layout with one byte per row.
byte canvas[2][MATRIX_ROWS];

// Flips the bit order in a byte
byte flipByte(byte c){
  char r=0;
  for(byte i = 0; i < 8; i++){
    r <<= 1;
    r |= c & 1;
    c >>= 1;
  }
  return r;
}

// Draws an eyeball (circle) onto the given canvas.
void draw_eyeball(byte* canvas) {
  const byte EYEBALL[MATRIX_ROWS] = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
  };

  for (int row=0; row < MATRIX_ROWS; ++row) {
    canvas[row] = EYEBALL[row];
  }
}

class Eye {
 public:
  // Shows the current state of the eye's canvas on the given
  // LED device.
  void show(int device) {
    draw();
    for (int row=0; row < sizeof(canvas_); ++row) {
      byte value = INVERT ? flipByte(canvas_[row]) : canvas_[row];
      leds.setRow(device, INVERT ? sizeof(canvas_) - row - 1 : row, value);
    }  
  }

  // Draws the current state of the eye to the canvas.
  void draw() {
    // Draw the eyeball and pupil.
    draw_eyeball(canvas_);
    int ax = (3 + focus.x) % 8;
    int ay = (3 + focus.y) % 8;
    if (ax < 0) ax = ax + 6;
    if (ay < 0) ay = ay + 6;
    if (large_pupil_) {
      if (ay - 1 >= 0) {
        canvas_[ay - 1] &= ~(0b11110000 >> (ax - 1));
      }
      if (ay >= 0) {
        canvas_[ay] &= ~(0b11110000 >> (ax - 1));
      }
      if (ay + 1 < MATRIX_ROWS) {
        canvas_[ay + 1] &= ~(0b11110000 >> (ax - 1));      
      }
      if (ay + 2 < MATRIX_ROWS) {
        canvas_[ay + 2] &= ~(0b11110000 >> (ax - 1));      
      }
    } else {
      if (ay >= 0) {
        canvas_[ay] &= ~(0b11000000 >> ax);
      }
      if (ay + 1 < MATRIX_ROWS) {
        canvas_[ay + 1] &= ~(0b11000000 >> ax);
      }
    }
    // Blank out lines according to the eyelid position
    for (int r=0; r<eyelid_row_ - 1; ++r) {
      canvas_[r] = 0;
      canvas_[sizeof(canvas_) - r - 1] = 0;
    }
  }

  // Updates the eye to look at the given point.
  // 0,0 is the middle of the eye, negative values
  // are left/up, positive ones are to the right.
  // Values wrap around if out of bounds.
  void look_at(Point p) {
    focus = p;
  }

  // Sets the position of the eyelids.
  // 0 is fully open, 5 is fully closed.
  void set_eyelid(int eyelid) {
    eyelid_row_ = abs(eyelid) % 6;
  }

  void set_large_pupil(bool value) {
    large_pupil_ = value;
  }
 
  // Where we're currently looking at.
  Point focus = {0,0};
  int eyelid_row_ = 0;
  // Whether we're draing a large or normal size pupil.
  bool large_pupil_ = false;
 private:
  byte canvas_[MATRIX_ROWS];
};


class MoveEye : public Animation {
 public:
  MoveEye(Eye* eye, Point target) : eye_(eye) {
    start_ = eye_->focus;
    delta_ = {target.x - start_.x, target.y - start_.y};
  }
  virtual void step(int pct) {
    Point p = {start_.x + (delta_.x * pct) / 100, start_.y + (delta_.y * pct) / 100};
    eye_->look_at(p);
  }
 private:
  Eye* eye_;
  Point start_;  // Starting position of the eye.
  Point delta_;  // [dx,dy] to move the eye by.
};

// Blinks a single eye.
class Blink : public Animation {
 public:
  Blink(Eye* eye) : eye_(eye) {
  }
  virtual void step(int pct) {
    if (pct <= 50) {
      eye_->set_eyelid((5 * pct) / 50);
    } else {
      pct -= 50;
      eye_->set_eyelid(5 - ((5 * pct) / 50));      
    }
  }
 private:
  Eye* eye_;
};

// Goes "cross-eyed", looking both eyes towards the middle.
class CrossEye : public Animation {
 public:
  CrossEye(Eye* l, Eye* r) :
    left_there_(l, {2,0}),
    right_there_(r, {-2, 0}) { 
  }
  virtual void step(int pct) {
    left_there_.step(pct);
    right_there_.step(pct);
  }
 private:
  MoveEye left_there_;
  MoveEye right_there_;
};

Eye left, right;

// Runs a number of animations on the eyes with the default 10 steps.
void animate(Animation** animations, int length, int step_delay) {
  animate(animations, length, 10, step_delay);
}

// Runs a number of animations on the eyes, showing the results after each step.
// Runs the animation in 1-100 |steps| with |step_delay| between them.
void animate(Animation** animations, int length, int steps, int step_delay) {
  steps %= 100;
  for (int pct=0; pct<=100; pct+=(100/steps)) {
    for (int i=0; i<length; ++i) {
      animations[i]->step(pct);
      left.show(LEFT);
      right.show(RIGHT);
    }
    delay(step_delay);
  }  
}

// Makes both eyes look at the same point.
void look(Point p) {
  MoveEye l(&left, p);
  MoveEye r(&right, p);
  Animation* a[] = {&l, &r};
  animate(a, 2, 10);
}

// Cross eyes, moving both pupils towards the middle.
void cross_eyes() {
  CrossEye e(&left, &right);
  Animation* a[] = {&e};
  animate(a, 1, 100);
}

// Blink eyes right, then left.
void crazy_blink() {
  {
    Blink b(&right);
    Animation* a[] = {&b};
    animate(a, 1, 80);
  }
  {
    Blink b(&left);
    Animation* a[] = {&b};
    animate(a, 1, 80);
  }
}

// Make the eyes wander horizontally, wrapping around
// from right to left.
void horizontal_spin() {
  // Center eyes
  {
    MoveEye l(&left, {0, 0});
    MoveEye r(&right, {0, 0});
    Animation* a[] = {&l, &r};
    animate(a, 2, 50);
  }
  // Move eyes to a coordinate far on the right.
  // The eye class wraps out of bounds coordinates
  // back to the other side. This needs more
  // than the usual 10 animation steps, otherwise
  // it's too jerky a motion.
  {
    MoveEye l(&left, {40, 0});
    MoveEye r(&right, {40, 0});
    Animation* a[] = {&l, &r};
    animate(a, 2, 40, 40);
  }  
  // Center eyes again without any animation.
  // Needed to ensure the eye isn't left with
  // a focus that's way out of bounds (which then
  // messes with upcoming animations).
  {
    left.look_at({0,0});
    right.look_at({0,0});
  }
}


// Moves left eye only.
void lazy_eye() {
  // Move left eye off center.
  {
    MoveEye l(&left, {-2, -2});
    Animation* a[] = {&l};
    animate(a, 1, 500);
  }
  // Quickly move back to where right eye is.
  {
    MoveEye l(&left, right.focus);
    Animation* a[] = {&l};
    animate(a, 1, 20);
  }
}

// Enlarges one pupil.
void big_eye() {
  // Center eyes
  {
    MoveEye l(&left, {0, 0});
    MoveEye r(&right, {0, 0});
    Animation* a[] = {&l, &r};
    animate(a, 2, 50);
  }  
  right.set_large_pupil(true);
  left.show(LEFT);
  right.show(RIGHT);
  delay(2000);
  right.set_large_pupil(false);
  left.show(LEFT);
  right.show(RIGHT);
}

// Flash leds to high intensity, slowly decline back to normal.
void glow_eyes() {
  for (int i=8; i>0; --i) {
    leds.setIntensity(LEFT,DEFAULT_INTENSITY + i); 
    leds.setIntensity(RIGHT,DEFAULT_INTENSITY + i);
    delay(150);      
  }
  leds.setIntensity(LEFT,DEFAULT_INTENSITY); 
  leds.setIntensity(RIGHT,DEFAULT_INTENSITY);
}

// Make the eyes spin vertically (wrapping around).
// Slowly come to a stop in the center.
void slot_machine() {
  // Center eyes
  {
    MoveEye l(&left, {0, 0});
    MoveEye r(&right, {0, 0});
    Animation* a[] = {&l, &r};
    animate(a, 2, 50);
  }
  // Move eyes to a coordinate far down.
  // The eye class wraps out of bounds coordinates
  // back to the other side. This needs more
  // than the usual 10 animation steps, otherwise
  // it's too jerky a motion.
  // Make the left eye stop center (10x4), but keep
  // the right eye off center.
  {
    MoveEye l(&left, {0, 40});
    MoveEye r(&right, {0, 64 - 3});
    Animation* a[] = {&l, &r};
    animate(a, 2, 40, 10);
  }
  // Slowly make the right eye land.
  {
    MoveEye r(&right, {0, 64});
    Animation* a[] = {&r};
    animate(a, 1, 50);
  }  
  // Center eyes  without any animation.
  // Otherwise the eye would animate the whole
  // journey back from 0, 80 or so.
  {
    left.look_at({0,0});
    right.look_at({0,0});
  }
  // Flash eyes. Jackpot!
  for (int i=0; i<2; ++i) {
    leds.setIntensity(LEFT,DEFAULT_INTENSITY + 5); 
    leds.setIntensity(RIGHT,DEFAULT_INTENSITY + 5);
    delay(150);      
    leds.setIntensity(LEFT,DEFAULT_INTENSITY); 
    leds.setIntensity(RIGHT,DEFAULT_INTENSITY);
    delay(150);      
  }
}


void setup() {
  // Wake up the MAX72XX from power-saving mode
  leds.shutdown(LEFT,false); 
  leds.shutdown(RIGHT,false); 
  
  // Set a low brightness for the Leds
  leds.setIntensity(LEFT,DEFAULT_INTENSITY); 
  leds.setIntensity(RIGHT,DEFAULT_INTENSITY); 
  
  // Play some startup animation.
  // Center eyes.
  left.look_at({0,0});
  right.look_at({0,0});
  left.show(LEFT);
  right.show(RIGHT);
  delay(2000);
  // Blink left then right.
  {
    Blink l(&left);
    Animation* a[] = {&l};
    animate(a, 1, 80);  
  }
  {
    Blink r(&right);
    Animation* a[] = {&r};
    animate(a, 1, 80);  
  }
  delay(1000);
}

void loop() {
  // Animate eyes to look in a new random direction
  {
    look({(int)random(-2, 3), (int)random(-2, 3)});
    delay(1500);
  }
  // 1 in 5 chance of Blinking both eyes
  if (random() % 5 == 0) {
    delay(500);
    Blink l(&left);
    Blink r(&right);
    Animation* a[] = {&l, &r};
    animate(a, 2, 50);
    delay(500);
  }
  // Randomly perform some animation some of the time.
  switch(random() % 21) {
    case 0:
      cross_eyes();
      break;
    case 1:
      horizontal_spin();
      break;
    case 2:
      big_eye();
      break;
    case 3:
      lazy_eye();
      break;
    case 4:
      crazy_blink();
      break;
    case 5:
      glow_eyes();
      break;
    case 6:
      slot_machine();
      break;
    default:
      // skip animation this cycle.
      break;    
  }
  delay(1000);  
}
