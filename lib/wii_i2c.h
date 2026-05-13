#ifndef WII_I2C_H_FILE
#define WII_I2C_H_FILE

#include <stdint.h>
#include "hardware/i2c.h"

#define WII_I2C_IDENT_NONE     0
#define WII_I2C_IDENT_NUNCHUK  0xa4200000
#define WII_I2C_IDENT_CLASSIC  0xa4200101

struct wii_i2c_nunchuk_state {
  // accelerometer
  int16_t acc_x;
  int16_t acc_y;
  int16_t acc_z;

  // analog stick:
  int8_t x;
  int8_t y;

  // buttons:
  uint8_t c;
  uint8_t z;
};

struct wii_i2c_classic_state {
  // analog sticks:
  int8_t lx;
  int8_t ly;
  int8_t rx;
  int8_t ry;

  // triggers (a_ is the analog part, d_ is the click bit):
  uint8_t a_lt;
  uint8_t a_rt;
  uint8_t d_lt;
  uint8_t d_rt;

  // d-pad:
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;

  // buttons:
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;

  // bumpers:
  uint8_t zr;
  uint8_t zl;

  // face buttons:
  uint8_t home;
  uint8_t plus;
  uint8_t minus;
};

int wii_i2c_init(i2c_inst_t *i2c, uint32_t sda_pin, uint32_t scl_pin);
int wii_i2c_read_ident(i2c_inst_t *i2c, uint32_t *ident);
int wii_i2c_request_state(i2c_inst_t *i2c);
int wii_i2c_read_state(i2c_inst_t *i2c, uint8_t *data);

void wii_i2c_decode_nunchuk(const uint8_t *data, struct wii_i2c_nunchuk_state *state);
void wii_i2c_decode_classic(const uint8_t *data, struct wii_i2c_classic_state *state);

#endif /* WII_I2C_H_FILE */
