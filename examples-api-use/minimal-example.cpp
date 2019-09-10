// Small example how to use the library.
// By: Ramin Sangesari
#include "led-matrix.h"
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
uint8_t buffer[32][32][3] = { 0 };
unsigned long step = 0;
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
 interrupt_received = true;
}
static void setPixelp(uint8_t x, uint8_t y, float r, float g, float b) {
 buffer[y][x][0] = uint8_t(std::max(0, std::min(255, int(r))));
 buffer[y][x][1] = uint8_t(std::max(0, std::min(255, int(g))));
 buffer[y][x][2] = uint8_t(std::max(0, std::min(255, int(b))));
}
static void HSVtoRGB(float& r, float& g, float& b, float h, float s, float v) {
 if (s == 0.0) {
   r = v;
   g = v;
   b = v;
 }
 int i = int(h * 6.0);
 float f = (h * 6.0) - i;
 float p = v * (1.0 - s);
 float q = v * (1.0 - s * f);
 float t = v * (1.0 - s * (1.0 - f));
 i = i % 6;
 if (i == 0) {
   r = v; g = t; b = p; return; // v, t, p  
 }
 if (i == 1) {
   r = q; g = v; b = p; return; // q, v, p
 }
 if (i == 2) {
   r = p; g = v; b = t; return; // p, v, t
 }
 if (i == 3) {
   r = p; g = q; b = v; return; // p, q, v
 }
 if (i == 4) {
   r = t; g = p; b = v; return; // t, p, v
 }
 if (i == 5) {
   r = v; g = p; b = q; return; // v, p, q
 }
}
void swirl(uint8_t x, uint8_t y, unsigned long step) {
 float fx = x - 31.5;
 float fy = y - 31.5;
 float dist = sqrt(fx * fx + fy * fy) * 0.5;
 float angle = (step * 0.1) + (dist * 1.5);
 float s = sin(angle);
 float c = cos(angle);
 float xs = x * c - y * s;
 float ys = x * s + y * c;
 float r = abs(xs + ys) * 12.0 - 20;
 float g = r + (s * 130);
 float b = r + (c * 130);
 setPixelp(x, y,
   r,
   g,
   b
 );
}
void setPixelU(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
 buffer[y][x][0] = r;
 buffer[y][x][1] = g;
 buffer[y][x][2] = b;
}
void gradient(uint8_t x, uint8_t  y, unsigned long  step) {
 uint8_t g = x * 32;
 uint8_t b = y * 32;
 uint8_t r = 255 - (x * 32);  
 setPixelU(x, y, r, g, b);
}
void rainbowSearch(uint8_t x, uint8_t y, unsigned long step) {
 float xs = sin((step) * 0.01) * 20.0;
 float ys = cos((step) * 0.01) * 20.0;
 float scale = ((sin(step / 60.0) + 1.0) * 0.2) + 0.2;
 float r = sin((x + xs) * scale) + cos((y + xs) * scale);
 float g = sin((x + xs) * scale) + cos((y + ys) * scale);
 float b = sin((x + ys) * scale) + cos((y + ys) * scale);
 setPixelp(x, y,
   r * 255,
   g * 255,
   b * 255
 );
}
void checker(uint8_t _x, uint8_t _y, unsigned long step) {
 //float x = _x - 8;
 //float y = _y - 8;
 float x = _x - 32;
 float y = _y - 32;
 float angle = step / 5.0;
 float s = sin(angle);
 float c = cos(angle);
 float xs = x * c - y * s;
 float ys = x * s + y * c;
 xs -= sin(step / 200.0) * 40.0;
 ys -= cos(step / 200.0) * 40.0;
 float scale = step % 20;
 scale /= 20.0;
 scale = (sin(step / 50.0) / 8.0) + 0.25;
 xs *= scale;
 ys *= scale;
 float xo = abs(xs) - int(abs(xs));
 float yo = abs(ys) - int(abs(ys));
 //  l = 0 if @ else 1 if xo > .1 and  else .5
 float l = int(floor(xs) + floor(ys)) % 2 ? 0 : (xo > 0.1 && yo > .1 ? 1 : 0.5);
 float r, g, b;
 HSVtoRGB(r, g, b, (step % 255) / 255.0, 10, 121);
 setPixelU(_x, _y,
   r * (l * 255),
   g * (l * 255),
   b * (l * 255)
 );
}
static void DrawOnCanvas2(Canvas *canvas) {
 /*
  * Let's create a simple animation. We use the canvas to draw
  * pixels. We wait between each step to have a slower animation.
  */
while (true)  {
 for (uint8_t x = 0; x < 32; x++) {
   for (uint8_t y = 0; y < 32; y++) {
     rainbowSearch(x, y, step);
   }
 }
 for (int x = 0; x < 32; x++) {
   for (int y = 0; y < 32; y++) {
     for (int c = 0; c < 3; c++) {  
			canvas->SetPixel(x, y, buffer[x][y][c], buffer[x][y][c], buffer[x][y][c]);  
     }
   }
 }
 step++;
}
}
int main(int argc, char *argv[]) {
 RGBMatrix::Options defaults;
 defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
 defaults.rows = 32;
 defaults.chain_length = 1;
 defaults.parallel = 1;
 defaults.show_refresh_rate = true;
 Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
 if (canvas == NULL)
   return 1;
 // It is always good to set up a signal handler to cleanly exit when we
 // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
 // for that.
 signal(SIGTERM, InterruptHandler);
 signal(SIGINT, InterruptHandler);
 DrawOnCanvas2(canvas);    // Using the canvas.
 // Animation finished. Shut down the RGB matrix.
 canvas->Clear();
 delete canvas;
 return 0;
} 
