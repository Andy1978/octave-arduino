/*
 * Octave arduino spi interface
 * Copyright (C) 2018 John Donoghue <john.donoghue@ieee.org>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "settings.h"
#include "OctaveSPILibrary.h"

#define ARDUINO_CONFIGSPI   1
#define ARDUINO_READ_WRITE_SPI 2
#define ARDUINO_READ_WRITE_REP_SPI 3

#ifdef USE_SPI
#include <SPI.h>

class SPIDevice
{
  #define USED 1
  #define ENABLED 2

public:
  uint8_t flags;
  uint8_t cspin;
  uint16_t cs_setup;    // min delay in µs between CS and first edge of SCLK
  SPISettings settings; // speedMaximum, dataOrder, dataMode

  SPIDevice();
  uint8_t init(uint8_t id, uint8_t spi_mode, uint8_t spi_bitorder, uint32_t spi_bitrate, uint16_t spi_cs_setup);
  uint8_t free();
  void set_cs(uint8_t state);
};

SPIDevice::SPIDevice ()
{
  flags = 0;
}

uint8_t
SPIDevice::init (uint8_t id, uint8_t spi_mode, uint8_t spi_bitorder, uint32_t spi_bitrate, uint16_t spi_cs_setup)
{
  flags = USED|ENABLED;
  cspin = id;
  cs_setup = spi_cs_setup;
  settings = SPISettings (spi_bitrate, spi_bitorder? MSBFIRST: LSBFIRST, spi_mode);
  return 0;
}

uint8_t
SPIDevice::free ()
{
  flags = 0;
  return 0;
}

void SPIDevice::set_cs(uint8_t state)
{
  digitalWrite (cspin, state);
}

#define MAX_SPI_DEVICES 5
static SPIDevice spidevs[MAX_SPI_DEVICES];

SPIDevice *
getSPI (uint8_t id)
{
  uint8_t i;
  SPIDevice * unused = 0;

  for (i=0; i<MAX_SPI_DEVICES; i++)
    {
      if (spidevs[i].flags & USED)
        {
          if (spidevs[i].cspin == id)
            return &spidevs[i];
        }
      else if (!unused)
        {
          unused = &spidevs[i];
        }
    }
  return unused;
}

#endif

OctaveSPILibrary::OctaveSPILibrary (OctaveArduinoClass &oc)
{

  libName = "SPI";

  oc.registerLibrary (this);
}

void
OctaveSPILibrary::commandHandler (uint8_t cmdID, uint8_t* data, uint16_t datasz)
{
  switch (cmdID)
    {
#ifdef USE_SPI
      case ARDUINO_CONFIGSPI:
        {
          if (datasz == 10)
            {
              //               data
              // spi id        0 (cs pin)
              // enable        1
              // mode          2
              // bitorder      3
              // bitrate       4..7
              // cs_setup_time 8..9

              SPIDevice *dev = getSPI (data[0]);

              if(dev == 0)
                {
                  sendErrorMsg_P (ERRORMSG_INVALID_DEVICE);
                  return;
                }

              if (data[1] == 1)
                {
                  uint32_t bitrate = ((uint32_t)data[4]<<24) | ((uint32_t)data[5]<<16) | ((uint32_t)data[6]<<8) | data[7];
                  uint16_t cs_setup_time = ((uint16_t)data[8]<<8) | data[9];
                  dev->init(data[0], data[2], data[3], bitrate, cs_setup_time);

                  // TODO: first call only ?
                  SPI.begin ();

                  dev->set_cs(HIGH);
                }
              else
                {
                  // TODO: last call only
                  // SPI.end ();

                  dev->free();
                }
              sendResponseMsg (cmdID,data, 2);
            }
          else if(datasz == 1)
            {
              SPIDevice * dev = getSPI (data[0]);
              if(dev == 0 || (dev->flags&USED)==0)
                {
                  sendErrorMsg_P (ERRORMSG_INVALID_DEVICE);
                  return;
                }

              // TODO: last call only
              // SPI.end ();

              dev->free();
            }
          else
            {
              sendInvalidNumArgsMsg ();
            }
          break;
        }
      case ARDUINO_READ_WRITE_SPI:
      case ARDUINO_READ_WRITE_REP_SPI:

        if (datasz >= 4)
          {
            SPIDevice * dev = getSPI (data[0]);
            if(dev == 0 || (dev->flags&USED)==0)
              {
                sendErrorMsg_P (ERRORMSG_INVALID_DEVICE);
                return;
              }

            uint8_t delay_us = data[1];
            uint8_t first_block_len = data[2];
            uint16_t data_len = datasz - 3;

            if (first_block_len > data_len)
              first_block_len = 0;

            // start of dataIn
            uint8_t *p = &data[3];

            if (cmdID == ARDUINO_READ_WRITE_REP_SPI && datasz >= 6)
              {
                uint16_t n_end_repeat = (((uint16_t)data[3])<<8) | ((uint16_t)data[4]);
                p += 2;
                data_len -= 2;
                // repeat the last byte n_end_repeat times
                for (uint16_t k = 0; k < n_end_repeat; ++k)
                  *(p + data_len + k) = *(p + data_len - 1);
                data_len += n_end_repeat;
            }

            // begin transaction
            SPI.beginTransaction (dev->settings);

            dev->set_cs(LOW);
            delayMicroseconds (dev->cs_setup);

            if (first_block_len && delay_us > 0)
            {
              // transfer the first block
              SPI.transfer (p, first_block_len);
              delayMicroseconds (delay_us);
            }

            // transfer the rest
            SPI.transfer (p+first_block_len, data_len - first_block_len);

            dev->set_cs(HIGH);
            SPI.endTransaction ();

            sendResponseMsg (cmdID, p, data_len);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;
#endif

      default:
        sendUnknownCmdIDMsg ();
        break;
    }
}
