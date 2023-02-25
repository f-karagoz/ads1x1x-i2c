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

#include "mgos_ads1x1x_internal.h"

static char *mgos_ads1x1x_type_str(struct mgos_ads1x1x *dev) {
  if (!dev) {
    return "VOID";
  }
  switch (dev->type) {
  case ADC_NONE: return "NONE";

  case ADC_ADS1013: return "ADS1013";

  case ADC_ADS1014: return "ADS1014";

  case ADC_ADS1015: return "ADS1015";

  case ADC_ADS1113: return "ADS1113";

  case ADC_ADS1114: return "ADS1114";

  case ADC_ADS1115: return "ADS1115";

  case ADC_ADS1219: return "ADS1219";

  default: return "UNKNOWN";
  }
}

static bool mgos_ads1x1x_reset(struct mgos_ads1x1x *dev) {
  if (!dev) {
    return false;
  }

  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    return mgos_i2c_write_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 0x8583);

  case ADC_ADS1219:
    uint8_t command = MGOS_ADS1219_COM_RESET;
    return mgos_i2c_write(dev->i2c, dev->i2caddr, &command, 1, true);

  default: return false;
  }
}

struct mgos_ads1x1x *mgos_ads1x1x_create(struct mgos_i2c *i2c, uint8_t i2caddr, enum mgos_ads1x1x_type type) {
  struct mgos_ads1x1x *dev = NULL;

  if (!i2c || type == ADC_NONE || type > ADC_ADS1219) {
    return NULL;
  }

  dev = (struct mgos_ads1x1x *) calloc(1, sizeof(struct mgos_ads1x1x));
  if (!dev) {
    return NULL;
  }

  memset(dev, 0, sizeof(struct mgos_ads1x1x));
  dev->i2caddr = i2caddr;
  dev->i2c     = i2c;
  dev->type    = type;
  switch (type) {
  case ADC_ADS1015:
  case ADC_ADS1115:
  case ADC_ADS1219:
    dev->channels = 4;
    break;

  default:
    dev->channels = 1;
  }

  if (!mgos_ads1x1x_reset(dev)) {
    LOG(LL_INFO, ("Could not reset %s at I2C 0x%02x", mgos_ads1x1x_type_str(dev), dev->i2caddr));
    free(dev);
    return NULL;
  }
  LOG(LL_INFO, ("%s initialized at I2C 0x%02x", mgos_ads1x1x_type_str(dev), dev->i2caddr));
  
  if (dev->type == ADC_ADS1219) {
    if ( !mgos_gpio_set_mode(MGOS_ADS1219_DRDYN_PIN, MGOS_GPIO_MODE_INPUT) ) {  // Initialize DRDYN GPIO pin
      return NULL;
    }
  }

  return dev;
}

bool mgos_ads1x1x_destroy(struct mgos_ads1x1x **dev) {
  if (!*dev) {
    return false;
  }

  free(*dev);
  *dev = NULL;
  return true;
}

bool mgos_ads1x1x_set_fsr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_fsr fsr) {
  uint16_t val;

  if (!dev) {
    return false;
  }

  switch (fsr) {
  case MGOS_ADS1X1X_FSR_MAX:
  case MGOS_ADS1X1X_FSR_6144:
    val = 0;
    break;

  case MGOS_ADS1X1X_FSR_4096:
    val = 1;
    break;

  case MGOS_ADS1X1X_FSR_1024:
    val = 3;
    break;

  case MGOS_ADS1X1X_FSR_512:
    val = 4;
    break;

  case MGOS_ADS1X1X_FSR_MIN:
  case MGOS_ADS1X1X_FSR_256:
    val = 5;
    break;

  default:   // 2.048V
    val = 2;
  }

  // ADS1x13 only supports 2.048V, ADS1219 natively supports 2.048V or external reference
  if ((dev->type == ADC_ADS1013 || dev->type == ADC_ADS1113 || dev->type == ADC_ADS1219) && val != 2) {
    return false;
  }

  return mgos_i2c_setbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 9, 3, val);
}

bool mgos_ads1x1x_get_fsr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_fsr *fsr) {
  uint16_t val;

  if (!dev || !fsr) {
    return false;
  }

  if (dev->type == ADC_ADS1219)
  {
    *fsr = MGOS_ADS1X1X_FSR_2048;
    return true;
  }

  if (!mgos_i2c_getbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 9, 3, &val)) {
    return false;
  }
  switch (val) {
  case 0: *fsr = MGOS_ADS1X1X_FSR_6144; break;

  case 1: *fsr = MGOS_ADS1X1X_FSR_4096; break;

  case 2: *fsr = MGOS_ADS1X1X_FSR_2048; break;

  case 3: *fsr = MGOS_ADS1X1X_FSR_1024; break;

  case 4: *fsr = MGOS_ADS1X1X_FSR_512; break;

  default: *fsr = MGOS_ADS1X1X_FSR_256; break;
  }
  return true;
}

bool mgos_ads1x1x_set_dr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_dr dr) {
  uint16_t val;

  if (!dev) {
    return false;
  }
  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
    switch (dr) {
    case MGOS_ADS1X1X_SPS_MIN:
    case MGOS_ADS1X1X_SPS_8:
      val = 0;
      break;

    case MGOS_ADS1X1X_SPS_16:
      val = 1;
      break;

    case MGOS_ADS1X1X_SPS_32:
      val = 2;
      break;

    case MGOS_ADS1X1X_SPS_64:
      val = 3;
      break;

    case MGOS_ADS1X1X_SPS_DEFAULT:
    case MGOS_ADS1X1X_SPS_128:
      val = 4;
      break;

    case MGOS_ADS1X1X_SPS_250:
      val = 5;
      break;

    case MGOS_ADS1X1X_SPS_475:
      val = 6;
      break;

    case MGOS_ADS1X1X_SPS_MAX:
    case MGOS_ADS1X1X_SPS_860:
      val = 7;
      break;

    default: return false;
    }
    break;

  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    switch (dr) {
    case MGOS_ADS1X1X_SPS_MIN:
    case MGOS_ADS1X1X_SPS_128:
      val = 0;
      break;

    case MGOS_ADS1X1X_SPS_250:
      val = 1;
      break;

    case MGOS_ADS1X1X_SPS_490:
      val = 2;
      break;

    case MGOS_ADS1X1X_SPS_920:
      val = 3;
      break;

    case MGOS_ADS1X1X_SPS_DEFAULT:
    case MGOS_ADS1X1X_SPS_1600:
      val = 4;
      break;

    case MGOS_ADS1X1X_SPS_2400:
      val = 5;
      break;

    case MGOS_ADS1X1X_SPS_MAX:
    case MGOS_ADS1X1X_SPS_3300:
      val = 6;
      break;

    default: return false;
    }
    break;

    case ADC_ADS1219:
      switch (dr)
      {
      case MGOS_ADS1X1X_SPS_MIN:
      case MGOS_ADS1X1X_SPS_DEFAULT:
      case MGOS_ADS1X1X_SPS_20:
        val = 0;
        break;

      case MGOS_ADS1X1X_SPS_90:
        val = 1;
        break;
      
      case MGOS_ADS1X1X_SPS_330:
        val = 2;
        break;

      case MGOS_ADS1X1X_SPS_MAX:
      case MGOS_ADS1X1X_SPS_1000:
        val = 3;
        break;

      default: return false;
      }

  default: return false;
  }

  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    return mgos_i2c_setbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 5, 3, val);

  case ADC_ADS1219:
    uint8_t conf_reg_data = mgos_i2c_read_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF); // Read the register
  
    if (conf_reg_data == -1) {
      return false;
    }

    conf_reg_data &= ~( 0b11 << 2 ); // Clear the DR bit field (2-bits)
    conf_reg_data |= ( val << 2 );   // Set the new DR

    uint8_t reg_write_data[2] = { MGOS_ADS1219_COM_RR_CONF, conf_reg_data }; // Refer to datasheet
    return mgos_i2c_write(dev->i2c, dev->i2caddr, reg_write_data, 2, true);   // Write the register

  default: return false;
  }
}

bool mgos_ads1x1x_get_dr(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_dr *dr) {
  uint16_t val;

  if (!dev || !dr) {
    return false;
  }

  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    if (!mgos_i2c_getbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 5, 3, &val)) {
      return false;
    }
    break;

  case ADC_ADS1219:
    if (!mgos_i2c_getbits_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF, 2, 2, (uint8_t*)&val)) {
      return false;
    }
    break;

  default: return false;
  }

  
  switch (dev->type) {
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    switch (val) {
    case 0: *dr = MGOS_ADS1X1X_SPS_128; break;

    case 1: *dr = MGOS_ADS1X1X_SPS_250; break;

    case 2: *dr = MGOS_ADS1X1X_SPS_490; break;

    case 3: *dr = MGOS_ADS1X1X_SPS_920; break;

    case 4: *dr = MGOS_ADS1X1X_SPS_1600; break;

    case 5: *dr = MGOS_ADS1X1X_SPS_2400; break;

    default: *dr = MGOS_ADS1X1X_SPS_3300; break;
    }
    break;

  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
    switch (val) {
    case 0: *dr = MGOS_ADS1X1X_SPS_8; break;

    case 1: *dr = MGOS_ADS1X1X_SPS_16; break;

    case 2: *dr = MGOS_ADS1X1X_SPS_32; break;

    case 3: *dr = MGOS_ADS1X1X_SPS_64; break;

    case 4: *dr = MGOS_ADS1X1X_SPS_128; break;

    case 5: *dr = MGOS_ADS1X1X_SPS_250; break;

    case 6: *dr = MGOS_ADS1X1X_SPS_475; break;

    default: *dr = MGOS_ADS1X1X_SPS_860; break;
    }
    break;

    case ADC_ADS1219:
      switch (val) {
        case 0: *dr = MGOS_ADS1X1X_SPS_20; break;
        
        case 1: *dr = MGOS_ADS1X1X_SPS_90; break;
        
        case 2: *dr = MGOS_ADS1X1X_SPS_330; break;
        
        default: *dr = MGOS_ADS1X1X_SPS_1000; break;
      }
      break;

  default: return false;
  }
  return true;
}

bool mgos_ads1x1x_set_gain(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_gain gain)
{
  uint8_t val;

  if (!dev) {
    return false;
  }

  switch ( gain ) {
    case MGOS_ADS1X1X_GAIN_1:
    val = 0;
    break;
    
    case MGOS_ADS1X1X_GAIN_4:
    val = 1;
    break;

    default: return false;
  }

  switch (dev->type) {
  case ADC_ADS1219:
    uint8_t conf_reg_data = mgos_i2c_read_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF); // Read the register

    if (conf_reg_data == -1) {
      return false;
    }
    conf_reg_data &= ~( 1 << 4 );     // Clear the Gain bit
    conf_reg_data |= ( val << 4 );    // Set the new Gain

    uint8_t reg_write_data[2] = { MGOS_ADS1219_COM_RR_CONF, conf_reg_data }; // Refer to datasheet
    return mgos_i2c_write(dev->i2c, dev->i2caddr, reg_write_data, 2, true);   // Write the register

  default: return false;
  }
}

bool mgos_ads1x1x_get_gain(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_gain *gain)
{
  uint8_t val;

  if (!dev || !gain) {
    return false;
  }

  switch (dev->type) {
  case ADC_ADS1219:
    if (!mgos_i2c_getbits_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF, 4, 1, &val)) {
      return false;
    }
    break;

  default: return false;
  }

  switch (dev->type) {
    case ADC_ADS1219:
      switch (val) {
        case 0: *gain = MGOS_ADS1X1X_GAIN_1; break;
        
        default: *gain = MGOS_ADS1X1X_GAIN_4; break;
      }
      break;

  default: return false;
  }
  return true;
}

bool mgos_ads1x1x_set_cmode(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_cmode cmode)
{
  uint8_t val;

  if (!dev) {
    return false;
  }

  switch ( cmode ) {
    case MGOS_ADS1X1X_CMODE_SNGL:
    val = 0;
    break;
    
    case MGOS_ADS1X1X_CMODE_CONT:
    val = 1;
    break;

    default: return false;
  }

  switch (dev->type) {
  case ADC_ADS1219:
    uint8_t conf_reg_data = mgos_i2c_read_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF); // Read the register

    if (conf_reg_data == -1) {
      return false;
    }

    conf_reg_data &= ~( 1 << 1 );     // Clear the CM bit
    conf_reg_data |= ( val << 1 );    // Set the new CM

    uint8_t reg_write_data[2] = { MGOS_ADS1219_COM_RR_CONF, conf_reg_data }; // Refer to datasheet
    return mgos_i2c_write(dev->i2c, dev->i2caddr, reg_write_data, 2, true);   // Write the register

  default: return false;
  }
}

bool mgos_ads1x1x_get_cmode(struct mgos_ads1x1x *dev, enum mgos_ads1x1x_cmode *cmode)
{
  uint8_t val;

  if (!dev || !gain) {
    return false;
  }

  switch (dev->type) {
  case ADC_ADS1219:
    if (!mgos_i2c_getbits_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF, 1, 1, &val)) {
      return false;
    }
    break;

  default: return false;
  }

  switch (dev->type) {
    case ADC_ADS1219:
      switch (val) {
        case 0: *cmode = MGOS_ADS1X1X_CMODE_SNGL; break;
        
        default: *cmode = MGOS_ADS1X1X_CMODE_CONT; break;
      }
      break;

  default: return false;
  }
  return true;
}

bool mgos_ads1x1x_set_channel(struct mgos_ads1x1x *dev,  uint8_t chanP, uint8_t chanN)
{
  uint16_t conf_val;

  if (!dev) {
    return false;
  }
  if (chanN == 0xff) {
    // Single ended read
    if (chanP >= dev->channels) {
      return false;
    }
    if (dev->type == ADC_ADS1219) {
      switch (chanP) {
      case 0: mux = 3; break;

      case 1: mux = 4; break;

      case 2: mux = 5; break;

      default: mux = 6; break;
      }
    } else {
      switch (chanP) {
      case 0: mux = 4; break;

      case 1: mux = 5; break;

      case 2: mux = 6; break;

      default: mux = 7; break;
      }
    }
  } else {
    if (dev->type != ADC_ADS1015 && dev->type != ADC_ADS1115 && dev->type != ADC_ADS1219) {
      if (chanP != 0 || chanN != 1) {
        return false;
      }
      mux = 0;
    } else if (dev->type == ADC_ADS1219) {
      // ADS1219: Differential read, only 4 allowed combinations
      if (chanP == 0 && chanN == 1) {
        mux = 0;
      } else if (chanP == 2 && chanN == 3) {
        mux = 1;
      } else if (chanP == 1 && chanN == 2) {
        mux = 2;
      } else {
        return false;
      }
    } else {
      // ADS1X15: Differential read, only 4 allowed combinations
      if (chanP == 0 && chanN == 1) {
        mux = 0;
      } else if (chanP == 0 && chanN == 3) {
        mux = 1;
      } else if (chanP == 1 && chanN == 3) {
        mux = 2;
      } else if (chanP == 2 && chanN == 3) {
        mux = 3;
      } else {
        return false;
      }
    }
  }

  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    conf_val = mgos_i2c_read_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF);
    break;

  case ADC_ADS1219:
    conf_val = mgos_i2c_read_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF);
    break;

  default: return false;
  }

  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    // Clear and then set MUX bits
    conf_val |= (7 << 12);                                                                    // TODO check need to set the bits (original code)
    conf_val &= ~(7 << 12);
    conf_val |= (mux << 12);

    conf_val |= (1 << 15); // OS - start conversion
    conf_val |= (1 << 8);  // MODE - singleshot

    return (mgos_i2c_write_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, conf_val));
    break;

  case ADC_ADS1219:
    // Clear and then set MUX bits
    conf_val |= (7 << 5);                                                                    // TODO check need to set the bits
    conf_val &= ~(7 << 5);
    conf_val |= (mux << 5);

    conf_val &= ~(1 << 1);  // MODE - singleshot, clear bit

    return (mgos_i2c_write_reg_b(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF, conf_val));       // TODO warning: uint8_t <-- conf_val [uint16_t]
    break;

  default: return false;
  }
}

bool mgos_ads1x1x_get_channel(struct mgos_ads1x1x *dev,  uint8_t *chanP, uint8_t *chanN)
{
  uint16_t mux;

  if (!dev || !chanP || !chanN) {
    return false;
  }

  // Get MUX data from register
  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    if (!mgos_i2c_getbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONF, 12, 3, &mux)) {
      return false;
    }
    break;

  case ADC_ADS1219:
    if (!mgos_i2c_getbits_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RR_CONF, 5, 3, (uint8_t*)&mux)) {
      return false;
    }
    break;

  default: return false;
  }

  // Cross check the MUX data within ADC devices to return channels
  switch (dev->type) {
  case ADC_ADS1113:
  case ADC_ADS1114:
  case ADC_ADS1115:
  case ADC_ADS1013:
  case ADC_ADS1014:
  case ADC_ADS1015:
    switch (mux) {
    case 0:
      *chanP = 0;
      *chanN = 1;
      return true;
    case 1:
      *chanP = 0;
      *chanN = 3;
      return true;
    case 2:
      *chanP = 1;
      *chanN = 3;
      return true;
    case 3:
      *chanP = 2;
      *chanN = 3;
      return true;
    case 4:
      *chanP = 0;
      *chanN = 0xff;
      return true;
    case 5:
      *chanP = 1;
      *chanN = 0xff;
      return true;
    case 6:
      *chanP = 2;
      *chanN = 0xff;
      return true;
    case 7:
      *chanP = 3;
      *chanN = 0xff;
      return true;
    default: return false;
    }
  case ADC_ADS1219:
    switch (mux) {
    case 0:
      *chanP = 0;
      *chanN = 1;
      return true;
    case 1:
      *chanP = 2;
      *chanN = 3;
      return true;
    case 2:
      *chanP = 1;
      *chanN = 2;
      return true;
    case 3:
      *chanP = 0;
      *chanN = 0xff;
      return true;
    case 4:
      *chanP = 1;
      *chanN = 0xff;
      return true;
    case 5:
      *chanP = 2;
      *chanN = 0xff;
      return true;
    case 6:
      *chanP = 3;
      *chanN = 0xff;
      return true;
    case 7:
      *chanP = 0xff;
      *chanN = 0xff;
      return true;
    default: return false;
    }
  default: return false;
  }
}

bool mgos_ads1x1x_is_data_ready(struct mgos_ads1x1x *dev)
{
  return (!mgos_gpio_read(MGOS_ADS1219_DRDYN_PIN));
}

bool mgos_ads1x1x_read(struct mgos_ads1x1x *dev, uint8_t chan, int16_t *result) {
  return mgos_ads1x1x_read_diff(dev, chan, 0xff, result);
}

// if chanN is 0xff, perform a single ended read (with chanP against GND)
bool mgos_ads1x1x_read_diff(struct mgos_ads1x1x *dev, uint8_t chanP, uint8_t chanN, int32_t *result) {
  int16_t  result_val;
  uint8_t  result_val_arr[3];
  uint16_t mux;

  if (!mgos_ads1x1x_set_channel(dev, chanP, chanN)) { // Set the MUX
    return false;
  }

  if (dev->type == ADC_ADS1219) {
    mgos_i2c_write(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_START_SYNCH, 1, 1);   // Start conversion
  }

  // ADS1219 signals when the conversion is done, ADS101X has 1ms conversion delay, ADS111X has 8ms.
  if (dev->type == ADC_ADS1219) {
    while ( !mgos_ads1x1x_is_data_ready(dev) )                                            // TODO look up for better way as this might fail
      ;
  } else if (dev->type >= ADC_ADS1113) {
    mgos_usleep(8000);
  } else {
    mgos_usleep(1000);
  }

  if (dev->type == ADC_ADS1219) {
    mgos_i2c_read_reg_n(dev->i2c, dev->i2caddr, MGOS_ADS1219_COM_RDATA, 3, result_val_arr);
  } else {
    result_val = mgos_i2c_read_reg_w(dev->i2c, dev->i2caddr, MGOS_ADS1X1X_REG_POINTER_CONV);
  }

  // ADS101X has 12 bit conversion
  if (dev->type < ADC_ADS1113) {
    result_val /= 16;
  }

  if (dev->type == ADC_ADS1219) {
    *result = (result_val_arr[0] << 16 | result_val_arr[1] << 8 | result_val_arr[2]);
  } else {
    *result = result_val;
  }
  
  return true;
}

// Mongoose OS library initialization
bool mgos_ads1x1x_i2c_init(void) {
  return true;
}

// Application TODO
// Start continuous conversion and read periodically with a timer callback
// Curently continuous coversion function did not implemented
// To read from different analog channel or even differential read we need to
// switch between the channels with a mux. This is done in the mgos_ads1x1x_read_diff()
