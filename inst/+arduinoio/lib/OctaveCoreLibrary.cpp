/*
 * Octave arduino core interface
 * Copyright (C) 2018-2019 John Donoghue <john.donoghue@ieee.org>
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
#include "OctaveCoreLibrary.h"

#if defined(ARDUINO_ARCH_ESP32) && ! defined(ARDUINO_NANO_ESP32)
  // contains analogWrite
  #include <ESP32Servo.h>
#endif

#define ARDUINO_RESET       0
#define ARDUINO_INIT        1
#define ARDUINO_CONFIGPIN   2
#define ARDUINO_DIGITAL     3
#define ARDUINO_ANALOG      4
#define ARDUINO_PWM         5
#define ARDUINO_PLAYTONE    6
#define ARDUINO_DAC         7

#define ARDUINO_GETLIB      8

#define ARDUINO_VERSION     20
#define ARDUINO_UPTIME      21

// endian flag
#define ARDUINO_LITTLEENDIAN 0x00
#define ARDUINO_BIGENDIAN    0x80
#define ARDUINO_ENDIAN ARDUINO_LITTLEENDIAN

// TODO: how know what board we are ???
//compiler provides something like:
// -DF_CPU=16000000L -DARDUINO=10805 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR
//  freq              ide ver         board             arch
#if defined(ARDUINO_AVR_UNO)
  #define BOARD_ID  0
#elif defined(ARDUINO_AVR_MEGA2560)
  #define BOARD_ID  1
#elif defined(ARDUINO_AVR_NANO)
  #define BOARD_ID  2
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
  #define BOARD_ID  5
#elif defined(ARDUINO_AVR_NANO_EVERY)
  #define BOARD_ID  6
#elif defined(ARDUINO_AVR_LILYPAD)
  #define BOARD_ID  10
#elif defined(ARDUINO_AVR_PRO)
  #define BOARD_ID  20
#elif defined(ARDUINO_AVR_PROMICRO)
  #define BOARD_ID  21
#elif defined(ARDUINO_AVR_LEONARDO)
  #define BOARD_ID  22
#elif defined(ARDUINO_AVR_MICRO)
  #define BOARD_ID  23
#elif defined(ARDUINO_SAMD_ZERO)
  // sparkfun samed21 dev/mini
  #if USB_VID == 0x1B4F && USB_PID == 0x8D21
    #define BOARD_ID 41
  #else
    // Arduino Zero
    #define BOARD_ID 40
  #endif
#elif defined(ARDUINO_SAM_DUE)
  #define BOARD_ID 45
  #define NUM_TOTAL_PINS PINS_COUNT
#elif defined(ARDUINO_SAMD_MKRZERO)
  #define BOARD_ID 50
  #define NUM_TOTAL_PINS 33
#elif defined(ARDUINO_SAMD_MKR1000)
  #define BOARD_ID 51
  #define NUM_TOTAL_PINS 30
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
  #define BOARD_ID 60
  #define NUM_TOTAL_PINS 34
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
  #define BOARD_ID 100
  #ifndef ARDUINO_ARCH_MBED
    #error "Expected mbed architechture"
  #endif
#elif defined(ARDUINO_NANO_RP2040_CONNECT)
  #define BOARD_ID 101
  #define BOARD_VOLTAGE 33
  #ifndef ARDUINO_ARCH_MBED
    #error "Expected mbed architechture"
  #endif
#elif defined(ARDUINO_NANO_ESP32)
  #define BOARD_ID 102
  #define BOARD_VOLTAGE 33
#elif defined(ARDUINO_ESP32_DEV)
  #define BOARD_ID 111
  #define BOARD_VOLTAGE 33
#elif defined(ARDUINO_UNOR4_WIFI)
  #define BOARD_ID  112
#elif defined(ARDUINO_UNOR4_MINIMA)
  #define BOARD_ID  113
#else
  #error "Unknown board type"
#endif

// board voltage = actualV*10
#ifndef BOARD_VOLTAGE
  #if defined(ARDUINO_ARCH_SAMD) || defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_SAM) || defined (ARDUINO_ARCH_RP2040)
    #define BOARD_VOLTAGE 33
  #elif defined(ARDUINO_AVR_PRO) || defined(ARDUINO_AVR_PROMICRO)
   #if F_CPU == 8000000L
    #define BOARD_VOLTAGE 33
   #else
    #define BOARD_VOLTAGE 50
   #endif
  #else
    #define BOARD_VOLTAGE 50
  #endif
#endif

static const int8_t map_config_mode[] PROGMEM =
{
  INPUT, // unset
  INPUT, // analoginput
  INPUT, // dig in
  OUTPUT, // dig out
  INPUT_PULLUP, // pullup
  INPUT_PULLUP, //-1, // i2c TODO: ??
  OUTPUT, // pwm
  OUTPUT, // servo
  OUTPUT, // spi TODO ?
  INPUT,  // interrupt
  -1,     // dac
  -1,     // reserved
};

int
get_mode(uint8_t m)
{
  if (m < sizeof (map_config_mode))
    {
      return pgm_read_byte_near (map_config_mode + m);
    }
  return INPUT;
}

#define pinToAnalog(a) (a < A0 ? 0 : a-A0)

#ifndef NUM_TOTAL_PINS
  #define NUM_TOTAL_PINS NUM_DIGITAL_PINS
#endif

//#ifdef UNO_WIFI_REV2_328MODE
//  #error ("Uno wifi firmware must be compiled without a 328 emultaion enabled")
//#endif

static uint8_t pinconfig[NUM_TOTAL_PINS];

#if defined (ARDUINO_ARCH_AVR) || defined (ARDUINO_ARCH_MEGAAVR)
#include <avr/wdt.h>
void
reset ()
{
  wdt_enable (WDTO_1S);
  while(1) {}
}
#elif defined (ARDUINO_ARCH_SAMD) || defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_SAM) || defined (ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RENESAS)
void
reset ()
{
  // processor software reset
  NVIC_SystemReset ();
}
#elif defined (ARDUINO_ARCH_ESP32)
void
reset ()
{
  ESP.restart ();
}
#else
  #error("Unimplemented architecture for reset")
#endif

OctaveCoreLibrary::OctaveCoreLibrary (OctaveArduinoClass &oc)
  : occlass (oc)
{

  libName = "Core";

  oc.registerLibrary (this);

  // set pins as not set
  for(unsigned int i = 0; i<NUM_TOTAL_PINS; i++)
    {
      pinconfig[i] = 0xff;
    }
}
void
OctaveCoreLibrary::commandHandler (uint8_t cmdID, uint8_t* data, uint16_t datasz)
{
  int val;

  switch (cmdID)
    {
      case ARDUINO_RESET:
        sendResponseMsg (cmdID, 0, 0);
        reset ();
        break;

      case ARDUINO_INIT:
//      ** NOTE ** DO NOT use debugPrint in the init routine

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
        data[0] =  SIGNATURE_0;
        data[1] =  SIGNATURE_1;
        data[2] =  SIGNATURE_2;
#elif defined (ARDUINO_ARCH_SAMD) || defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_SAM)
        // allow the config file to supply the mcu
        data[0] =  0;
        data[1] =  0;
        data[2] =  0;
#else
        data[0] =  0;
        data[1] =  0;
        data[2] =  0;
#endif
        data[3] = BOARD_ID;
        data[4] = BOARD_VOLTAGE;
        data[5] = occlass.getLibCount ();
	// flags
	// if anything is big endian, 0x80
	data[6] = ARDUINO_ENDIAN | 0;

        sendResponseMsg (cmdID, data, 7);
        break;
      case ARDUINO_GETLIB:
        {
          if(datasz != 1)
            {
              sendInvalidNumArgsMsg ();
            }
          else
            {
              // lib idx
              // populate data[1]... with lib name
              datasz = 0;
              const char * name = occlass.getLibName (data[0]);
              while (name[datasz] != '\0' && datasz < 254)
                {
                  data[datasz+1] = name[datasz];
                  datasz++;
                }
              sendResponseMsg (cmdID, data, datasz+1);
            }
          break;
        }
      case ARDUINO_CONFIGPIN:
        if (datasz == 1 && data[0] < NUM_TOTAL_PINS)
          {
            data[1] = pinconfig[data[0]]; // TODO: get mode somehow ????
            sendResponseMsg (cmdID, data, 2);
          }
        else if (datasz == 2 && data[1] >= sizeof(map_config_mode))
          {
            sendErrorMsg_P (ERRORMSG_INVALID_MODE);
          }
        else if (datasz >= 1 && data[0] >= NUM_TOTAL_PINS)
          {
            sendErrorMsg_P (ERRORMSG_INVALID_PIN);
          }
        else if (datasz == 2 && data[0] < NUM_TOTAL_PINS && data[1] < sizeof(map_config_mode))
          {
            int mode = get_mode (data[1]);
            pinconfig[data[0]] = data[1];

            if (data[1] == 10) // dac
              analogWriteResolution (8); // ist eigentlich default, trotzdem schreiben

            if (mode != -1)
              pinMode (data[0], mode);

            sendResponseMsg (cmdID, data, 0);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;

      case ARDUINO_DIGITAL:

        if (datasz == 1)
          {
            val = digitalRead (data[0]);
            if (val == HIGH)
              data[1] = 1;
            else
              data[1] = 0;

            sendResponseMsg (cmdID, data, 2);
          }
        else if (datasz == 2)
          {
            digitalWrite (data[0], data[1] ? HIGH : LOW);
            sendResponseMsg (cmdID, data, 0);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
       break;

      case ARDUINO_ANALOG:

        if (datasz == 1)
          {
            val = analogRead (pinToAnalog(data[0]));
            data[1] = (val>>8)&0xff;
            data[2] = (val)&0xff;
            sendResponseMsg (cmdID, data, 3);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;

      case ARDUINO_PWM:

        if (datasz == 2)
          {
            analogWrite (data[0], data[1]);
            sendResponseMsg (cmdID, data, 0);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;
      case ARDUINO_PLAYTONE:
        if (datasz == 5)
          {
#if defined(ARDUINO_SAM_DUE)
            sendErrorMsg_P (ERRORMSG_UNIMPLEMENTED);
#else
            // 0 = pin
            // 1 = freqh
            // 2 = freql (hz)
            // 3 = durh
            // 4 = durl (10ths of second)
            unsigned long duration = (((unsigned long)(data[3]))<<8 | data[4]) * 100;
            unsigned int freq = (((unsigned int)(data[1]))<<8 | data[2]);

            if (freq == 0)
              noTone (data[0]);
            else
              tone (data[0], freq, duration);

            sendResponseMsg (cmdID, data, 0);
#endif
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;

      case ARDUINO_DAC:

        if (datasz == 2)
          {
            analogWrite (data[0], data[1]);
            sendResponseMsg (cmdID, data, 0);
          }
        else
          {
            sendInvalidNumArgsMsg ();
          }
        break;

      case ARDUINO_VERSION:
        {
          data[0] = VERSION_MAJOR;
          data[1] = VERSION_MINOR;
          data[2] = VERSION_PATCH;
          sendResponseMsg (cmdID, data, 3);
        }
        break;
      case ARDUINO_UPTIME:
        {
          unsigned long t = millis ();
          data[0] = (t>>24)&0xff;
          data[1] = (t>>16)&0xff;
          data[2] = (t>>8)&0xff;
          data[3] = (t>>0)&0xff;
          sendResponseMsg (cmdID, data, 4);
        }
        break;
      default:
        sendUnknownCmdIDMsg ();
        break;
    }
}
