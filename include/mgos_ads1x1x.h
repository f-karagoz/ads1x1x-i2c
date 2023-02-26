/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "mgos.h"
#include "mgos_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_ads1x1x;

enum mgos_ads1x1x_type {
  ADC_NONE = 0,
  ADC_ADS1013,
  ADC_ADS1014,
  ADC_ADS1015,
  ADC_ADS1113,
  ADC_ADS1114,
  ADC_ADS1115,
  ADC_ADS1219
};

enum mgos_ads1x1x_fsr {
  MGOS_ADS1X1X_FSR_MIN = 0, // 6.144V
  MGOS_ADS1X1X_FSR_6144,    // 6.144V
  MGOS_ADS1X1X_FSR_4096,    // 4.096V
  MGOS_ADS1X1X_FSR_2048,    // 2.048V
  MGOS_ADS1X1X_FSR_1024,    // 1.024V
  MGOS_ADS1X1X_FSR_512,     // 0.512V
  MGOS_ADS1X1X_FSR_256,     // 0.256V
  MGOS_ADS1X1X_FSR_DEFAULT, // 2.048V
  MGOS_ADS1X1X_FSR_MAX,     // 0.256V
};

enum mgos_ads1x1x_dr {
  MGOS_ADS1X1X_SPS_MIN = 0, // 8SPS for ADS111X, 20SPS for ADS1219, 128SPS for ADS101X
  MGOS_ADS1X1X_SPS_8,       // 8SPS, ADS111X only
  MGOS_ADS1X1X_SPS_16,      // 16SPS, ADS111X only
  MGOS_ADS1X1X_SPS_20,      // 20SPS, ADS1219 only
  MGOS_ADS1X1X_SPS_32,      // 32SPS, ADS111X only
  MGOS_ADS1X1X_SPS_64,      // 64SPS, ADS111X only
  MGOS_ADS1X1X_SPS_90,      // 90SPS, ADS1219 only
  MGOS_ADS1X1X_SPS_128,     // 128SPS, both ADS111X and ADS101X
  MGOS_ADS1X1X_SPS_250,     // 250SPS, both ADS111X and ADS101X
  MGOS_ADS1X1X_SPS_330,     // 330SPS, ADS1219 only
  MGOS_ADS1X1X_SPS_475,     // 475SPS, ADS111X only
  MGOS_ADS1X1X_SPS_490,     // 490SPS, ADS101X only
  MGOS_ADS1X1X_SPS_860,     // 860SPS, ADS111X only
  MGOS_ADS1X1X_SPS_920,     // 920SPS, ADS101X only
  MGOS_ADS1X1X_SPS_1000,    // 1000SPS, ADS1219 only
  MGOS_ADS1X1X_SPS_1600,    // 1600SPS, ADS101X only
  MGOS_ADS1X1X_SPS_2400,    // 2400SPS, ADS101X only
  MGOS_ADS1X1X_SPS_3300,    // 3300SPS, ADS101X only
  MGOS_ADS1X1X_SPS_DEFAULT, // 20SPS for ADS1219, 128SPS for ADS111X, 1600SPS for ADS101X
  MGOS_ADS1X1X_SPS_MAX,     // 860SPS for ADS111X, 1000SPS for ADS1219, 3300SPS for ADS101X
};

enum mgos_ads1x1x_gain {
  MGOS_ADS1X1X_GAIN_1,      // 1GAIN, ADS1219 only
  MGOS_ADS1X1X_GAIN_4       // 4GAIN, ADS1219 only
};

enum mgos_ads1x1x_cmode {
  MGOS_ADS1X1X_CMODE_SNGL,  // Single conversion mode, ADS1219 only
  MGOS_ADS1X1X_CMODE_CONT     // Continuos conversion mode, ADS1219 only
};

/*
 * Initialize a ADS1X1X on the I2C bus `i2c` at address specified in `i2caddr`
 * parameter (default ADS1X1X is on address 0x48). The device will be polled for
 * validity, upon success a new `struct mgos_ads1x1x` is allocated and
 * returned. If the device could not be found, NULL is returned.
 */
struct mgos_ads1x1x *mgos_ads1x1x_create(struct mgos_i2c *i2c, uint8_t i2caddr, enum mgos_ads1x1x_type type);

/*
 * Destroy the data structure associated with a ADS1X1X device. The reference
 * to the pointer of the `struct mgos_ads1x1x` has to be provided, and upon
 * successful destruction, its associated memory will be freed and the pointer
 * set to NULL and true will be returned.
 */
bool mgos_ads1x1x_destroy(struct mgos_ads1x1x **dev);

/* Get or Set the Full Scale Range (FSR). All chips in the ADS1X1X family support
 * the same settings. By default, 2.048V is used.
 *
 * Note: ADS1x13 does not support this, and always has an FSR of 2.048V.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_set_fsr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_fsr fsr);
bool mgos_ads1x1x_get_fsr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_fsr *fsr);

/* Get or Set the Data Rate (in Samples per Second). If the supplied `dr`
 * argument cannot be set on the chip, false is returned. Otherwise, the
 * supplied `dr` is set. By default, ADS101X sets 1600SPS, ADS111X sets 128SPS.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_set_dr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_dr dr);
bool mgos_ads1x1x_get_dr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_dr *dr);

/* Get or Set the Gain. Only the ADS1219 suppports this configuration.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_set_gain(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_gain gain);
bool mgos_ads1x1x_get_gain(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_gain *gain);

/* Get or Set the conversion mode.
 * Currently only implemented for ADS1219.
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_set_cmode(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_cmode cmode);
bool mgos_ads1x1x_get_cmode(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_cmode *cmode);

/* Get or Set the channels to/in use.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_set_channel(struct mgos_ads1x1x *dev,  uint8_t chanP, uint8_t chanN);
bool mgos_ads1x1x_get_channel(struct mgos_ads1x1x *dev,  uint8_t *chanP, uint8_t *chanN);

/* Checks if the data is ready to be read. Only the ADS1219 suppports this configuration.
 *
 * Returns true for data ready, false for data not ready.
 */
bool mgos_ads1x1x_is_data_ready(struct mgos_ads1x1x *dev);

/* Read a channel from the ADC and return the read value in `result`. If the
 * channel was invalid, or an error occurred, false is returned and the result
 * cannot be relied upon.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_read(struct mgos_ads1x1x *dev, uint8_t chan, int32_t *result);

/* Read a 2-channel differential from the ADC and return the read value in `result`.
 * If the channel pair invalid, or an error occurred, false is returned and the
 * result cannot be relied upon. Upon success, true is returned.
 *
 * Note: This is only available on ADS1X15 chips.
 *       Valid chanP/chanN pairs are : 0/1, 0/3, 1/3, 2/3.
 *
 * Returns true on success, false otherwise.
 */
bool mgos_ads1x1x_read_diff(struct mgos_ads1x1x *dev, uint8_t chanP, uint8_t chanN, int32_t *result);

#ifdef __cplusplus
}
#endif
