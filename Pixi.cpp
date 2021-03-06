/*
  Pixi.cpp - Library for Analog shield with Maxim PIXI A/D chip.
  Created by Wolfgang Friedrich, July 29. 2014.
  Will be released into the public domain.
*/

#include "Pixi.h"
#include <bcm2835.h>
#include <cstdio>

// Config SPI for communication witht the PIXI
Pixi::Pixi() {

  if (!bcm2835_init()) {
    printf("bcm2835_init failed. Are you running as root??\n");
  }

  if (!bcm2835_spi_begin()) {
    printf("bcm2835_spi_begin failed. Are you running as root??\n");
  }
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // The default
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);              // The default
  bcm2835_spi_setClockDivider(32);                         // The default
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                 // The default
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // the default
}

Pixi::~Pixi() {
  printf("Destructing Pixi.\n");
  bcm2835_spi_end();
  bcm2835_close();
}

/*
Read register and return value
if the debug parameter is set to TRUE, the register value is printed in format
SPI read adress: 0x0 : h 0x4 : l 0x24
SPI read result 0x424
*/
uint32_t Pixi::ReadRegister(char address, bool debug = false) {
  char request[3] = {char((address << 0x01) | PIXI_READ), 0x00, 0x00};
  char result[3];

  bcm2835_spi_transfernb(request, result, sizeof(request));

  uint32_t r = uint32_t(uint16_t(result[1] << 8) + uint16_t(result[2]));

  return (r);
}

/*
write value into register
Address needs to be shifted up 1 bit, LSB is read/write flag
hi data char is sent first, lo data char last.
*/
void Pixi::WriteRegister(char address, uint32_t value) {
  char writeBuffer[3] = {char((address << 0x01) | PIXI_WRITE), char(value >> 8),
                         char(value & 0xFF)};
  bcm2835_spi_transfern(writeBuffer, sizeof(writeBuffer));

  return; // (result);
}

/*
General Config for the PixiShield
Read ID register to make sure the shield is connected.
*/
uint32_t Pixi::config() {
  uint32_t result = 0;
  uint32_t info = 0;

  result = ReadRegister(PIXI_DEVICE_ID, true);

  if (result == 0x0424) {
    // enable default burst, thermal shutdown, leave conversion rate at 200k
    WriteRegister(PIXI_DEVICE_CTRL, !BRST | THSHDN); // ADCCONV = 00 default.
    // enable internal temp sensor
    // disable series resistor cancelation
    info = ReadRegister(PIXI_DEVICE_CTRL, false);
    WriteRegister(PIXI_DEVICE_CTRL, info | !RS_CANCEL);
    // keep TMPINTMONCFG at default 4 samples

    // Set int temp hi threshold
    WriteRegister(PIXI_TEMP_INT_HIGH_THRESHOLD,
                  0x0230); // 70 deg C in .125 steps
    // Keep int temp lo threshold at 0 deg C, negative values need function to
    // write a two's complement number. enable internal and both external temp
    // sensors
    info = ReadRegister(PIXI_DEVICE_CTRL, false);
    WriteRegister(PIXI_DEVICE_CTRL, info | TMPCTLINT | TMPCTLEXT1 | TMPCTLEXT2);

    uint32_t devCtl = ReadRegister(PIXI_DEVICE_CTRL, false);
    uint16_t adc_ctl = 0b11;  // ADCCTL  [0:1] ADC conversion mode selection.
    uint16_t dac_ctl = 0b00;  // DACCTL  [3:2] DAC mode selection.
    uint16_t adc_conv = 0b11; // ADCCONV [5:4] ADC conversion rate selection.
    uint16_t dac_ref = 0b1;   // DACREF  [6]   DAC voltage reference

    WriteRegister(PIXI_DEVICE_CTRL, devCtl | ADC_MODE_CONT | DACREF | !DACCTL);
  }

  return (result);
}

/*
Channel Config
Parameters that are not used for the selected channel are ignored.
*/

uint32_t Pixi::configChannel(int channel, int channel_mode, uint32_t dac_dat,
                             uint32_t range, char adc_ctl) {
  uint32_t result = 0;
  uint32_t info = 0;

  if ((channel <= 19) && (channel_mode <= 12)) {

    if (channel_mode == CH_MODE_1 || channel_mode == CH_MODE_3 ||
        channel_mode == CH_MODE_4 || channel_mode == CH_MODE_5 ||
        channel_mode == CH_MODE_6 || channel_mode == CH_MODE_10) {
      // config DACREF (internal reference),DACCTL (sequential update)
      info = ReadRegister(PIXI_DEVICE_CTRL, true);
      WriteRegister(PIXI_DEVICE_CTRL, info | DACREF | !DACCTL);

      info = ReadRegister(PIXI_DEVICE_CTRL, true);
      // Enter DACDAT
      WriteRegister(PIXI_DAC_DATA + channel, dac_dat);
      // Mode1: config FUNCID, FUNCPRM (non-inverted default)
      if (channel_mode == CH_MODE_1) {
        WriteRegister(
            PIXI_PORT_CONFIG + channel,
            (((CH_MODE_1 << 12) & FUNCID) | ((range << 8) & FUNCPRM_RANGE)));
      };

      // Mode3: config GPO_DAT, leave channel at logic level 0
      if (channel_mode == CH_MODE_3) {
        if (channel <= 15) {
          WriteRegister(PIXI_GPO_DATA_0_15, 0x00);
        } else if (channel >= 16) {
          WriteRegister(PIXI_GPO_DATA_16_19, (0x00));
        };
      }
      // Mode3,4,5,6,10: config FUNCID, FUNCPRM (non-inverted default)
      if (channel_mode == CH_MODE_3 || channel_mode == CH_MODE_5 ||
          channel_mode == CH_MODE_6 || channel_mode == CH_MODE_10) {
        WriteRegister(
            PIXI_PORT_CONFIG + channel,
            (((channel_mode << 12) & FUNCID) | ((range << 8) & FUNCPRM_RANGE)));

      } else if (channel_mode == CH_MODE_4) {
        WriteRegister(
            PIXI_PORT_CONFIG + channel,
            (((channel_mode << 12) & FUNCID) | ((range << 8) & FUNCPRM_RANGE)
             // assoc port & FUNCPRM_ASSOCIATED_PORT
             ));
      }

      // Mode1: config GPIMD (leave at default INT never asserted
      if (channel_mode == CH_MODE_1) {
        //        WriteRegister ( PIXI_GPI_IRQ_MODE_0_7, 0 );
      }

    }

    else if (channel_mode == CH_MODE_7 || channel_mode == CH_MODE_8 ||
             channel_mode == CH_MODE_9) {

      // Mode9: config FUNCID, FUNCPRM
      if (channel_mode == CH_MODE_9) {
        WriteRegister(
            PIXI_PORT_CONFIG + channel,
            (((channel_mode << 12) & FUNCID) | ((range << 8) & FUNCPRM_RANGE)));
      }

      if (channel_mode == CH_MODE_7 || channel_mode == CH_MODE_8) {
        WriteRegister(PIXI_PORT_CONFIG + channel,
                      (((channel_mode << 12) & FUNCID) |
                       ((range << 8) & FUNCPRM_RANGE) |
                       ((1 << 5)) & FUNCPRM_NR_OF_SAMPLES));
      }

      // config ADCCTL
      info = ReadRegister(PIXI_DEVICE_CTRL, false);
      WriteRegister(PIXI_DEVICE_CTRL, info | (adc_ctl & ADCCTL));

    } else if (channel_mode == CH_MODE_2 || channel_mode == CH_MODE_11 ||
               channel_mode == CH_MODE_12) {

      WriteRegister(
          PIXI_PORT_CONFIG + channel,
          (((channel_mode << 12) & FUNCID) | ((range << 8) & FUNCPRM_RANGE)));
    };
  }
  return (result);
};

/*
void Pixi::configTempChannel()
{

}
*/

/*
void Pixi::configInterrupt()
{

}
*/

/*
Readout of raw register value for given temperature channel
*/
uint32_t Pixi::readRawTemperature(int temp_channel) {
  uint32_t result = 0;

  result = ReadRegister(PIXI_INT_TEMP_DATA + temp_channel,
                        false); // INT_TEMP_DATA is the lowest temp data adress,
                                // channel runs from 0 to 2.

  return (result);
}

/*
Readout of given temperature channel and conversion into degC float return value
*/
float Pixi::readTemperature(int temp_channel) {
  float result = 0;
  uint32_t rawresult = 0;
  bool sign = 0;

  rawresult = ReadRegister(PIXI_INT_TEMP_DATA + temp_channel,
                           false); // INT_TEMP_DATA is the lowest temp data
                                   // adress, channel runs from 0 to 2.

  sign = (rawresult & 0x0800) >> 11;

  if (sign == 1) {
    rawresult = ((rawresult & 0x07FF) xor 0x07FF) +
                1; // calc absolut value from 2's comnplement
  }

  result =
      0.125 * (rawresult & 0x0007); // pick only lowest 3 bit for value left of
                                    // decimal point One LSB is 0.125 deg C
  result = result + ((rawresult >> 3) & 0x01FF);

  if (sign == 1) {
    result = result * -1; // fix sign
  }

  return (result);
}

/*
output analog value when channel is configured in mode 5
*/
uint32_t Pixi::writeAnalog(int channel, uint32_t value) {
  uint32_t result = 0;
  uint32_t channel_func = 0;

  channel_func = ReadRegister(PIXI_PORT_CONFIG + channel, false);
  channel_func = (channel_func & FUNCID) >> 12;

  if (channel_func == 5) {
    WriteRegister(PIXI_DAC_DATA + channel, value);
    result = ReadRegister(PIXI_DAC_DATA + channel, false);
  }

  return (result);
};

uint32_t Pixi::readAnalog(int channel) {
  uint32_t result = 0;
  uint32_t channel_func = 0;
  channel_func = ReadRegister(PIXI_PORT_CONFIG + channel, false);
  channel_func = (channel_func & FUNCID) >> 12;

  if (channel_func == 7) {
    result = ReadRegister(PIXI_ADC_DATA + channel, false);
  }

  return (result);
}
uint32_t Pixi::setChannelType(int channel, uint32_t type) {
  uint32_t channel_func = 0;
  uint32_t orig = ReadRegister(PIXI_PORT_CONFIG + channel, false);
  WriteRegister(PIXI_PORT_CONFIG + channel, orig | (type << 12));

  return 0;
}

uint32_t Pixi::getChannelType(int channel) {
  uint32_t channel_func = 0;
  channel_func = ReadRegister(PIXI_PORT_CONFIG + channel, false);
  channel_func = (channel_func & FUNCID) >> 12;
  return channel_func;
}