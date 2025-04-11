/*
 * Copyright (C) 2025 Andreas Weber <andy.weber.aw@gmail.com>
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
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "PulseTrainAddon.h"

#define MAX_PULSETRAINS 8
#define PULSETRAIN_START  0x01
#define PULSETRAIN_RELEASE  0x02

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Code below only works on little endian MCUs
#endif

class PulseTrain
{
public:
  uint8_t pin;
  uint8_t polarity;

  uint8_t step;
  uint16_t cycles_left;

  uint32_t pulse_width;
  uint32_t pulse_period;

  uint32_t next;
  uint32_t last_poll;

  void release ()
  {
    pin = 255;
    polarity = 0;
    step = 0;
    cycles_left = 0;
    pulse_width = 0;
    pulse_period = 0;
    next = 0;
    last_poll = 0;
  }

  PulseTrain()
  {
    release ();
  }

  void poll()
  {
    static uint8_t wait_for_overflow = 0;

    // Attention: micros() overflow every 2^32/1e6/60s => 71min
    uint32_t now = micros();

    // simulate overflow every 1.05s for debugging purposes
    //now &= 0xFFFFF;

    if (wait_for_overflow && now < last_poll)
      {
        wait_for_overflow = 0;
        //digitalWrite (D5, !digitalRead(D5));
      }

    if (pin < 255 && !wait_for_overflow && cycles_left)
      {
        //char buf [100];
        //snprintf (buf, 100, "next = %li, now = %li, cycles_left = %i\n", next, now, cycles_left);
        //buf[99] = 0;
        //Serial.print (buf);
        if (now >= next)
          {
            //snprintf (buf, 100, "step = %i\n", step);
            //Serial.print (buf);
            switch (step)
              {
              case 0:
                digitalWrite (pin, polarity);
                next = now + pulse_width;
                step++;
                break;
              case 1:
                digitalWrite (pin, !polarity);
                next = now + pulse_period - pulse_width;
                step = 0;
                cycles_left--;
                break;
              }

            // simulate overflow every 1.05s for debugging purposes
            //next &= 0xFFFFF;
            if (now > next) // check for overflow of next
              {
                wait_for_overflow = 1;
                //digitalWrite (D6, !digitalRead(D6));
              }
          }
      }
    last_poll = now;
  }
};

static PulseTrain pulsetrains[MAX_PULSETRAINS];

static PulseTrain* getPulseTrain (uint8_t pin)
{
  uint8_t i;
  PulseTrain * unused = 0;
  for (i = 0; i < MAX_PULSETRAINS; i++)
    {
      if (pulsetrains[i].pin < 255)
        {
          if (pulsetrains[i].pin == pin)
            return &pulsetrains[i];
        }
      else if (!unused)
        {
          unused = &pulsetrains[i];
        }
    }
  return unused;
}

PulseTrainAddon::PulseTrainAddon(OctaveArduinoClass& a)
{
  libName = "PulseTrain/PulseTrain";
  a.registerLibrary(this);
}

void PulseTrainAddon::commandHandler(uint8_t cmdID, uint8_t* data, uint16_t datasz)
{
  switch (cmdID)
    {
      case PULSETRAIN_START:
      {
        PulseTrain *pt = getPulseTrain (data[0]);
        // id + X pins
        if(pt && datasz == 16)
          {
            pt->pin = data[0];

            // polarity == 0 => LOW pulse
            // polarity == 1 => HIGH pulse (default)
            pt->polarity = data[1];

            uint32_t initial_delay = *((uint32_t*)(data + 2)); //needed only once
            pt->pulse_width        = *((uint32_t*)(data + 6));
            pt->pulse_period       = *((uint32_t*)(data + 10));
            pt->cycles_left        = *((uint16_t*)(data + 14));

            pt->step = 0;

            pinMode (pt->pin, OUTPUT);

            // TODO: unsure, if I should preset the pin here
            //digitalWrite (pt->pin, !pt->polarity);
            pt->next = micros() + initial_delay;

            //char buf[100];
            //int len = 0;
            //snprintf (buf, 100, "pn = %i, polarity = %i, pulse_width = %li, pulse_period = %li, num_cycles = %i%n", pt->pin, pt->polarity, pt->pulse_width, pt->pulse_period, pt->cycles_left, &len);
            //buf[99] = 0;
            //sendResponseMsg (ARDUINO_DEBUG, (uint8_t*)buf, len);

            //sprintf (buf, "initial_delay = %li\n", initial_delay); Serial.print (buf);
            //sprintf (buf, "pulse_width = %li\n", pt->pulse_width); Serial.print (buf);
            //sprintf (buf, "pulse_period = %li\n", pt->pulse_period); Serial.print (buf);
            //sendResponseMsg (ARDUINO_DEBUG, (uint8_t *)buffer, len);

            sendResponseMsg(cmdID, data, 1);
          }
        else
          {
            // TODO: pt == 0 means "MAX_PULSETRAINS exceeded..."
            sendErrorMsg_P (ERRORMSG_INVALID_ARGS);
          }
        break;
      }
      case PULSETRAIN_RELEASE:
      {
        PulseTrain *pt = getPulseTrain (data[0]);
        if(pt && datasz == 1)
          {
            pt->release ();
            sendResponseMsg(cmdID, data, 1);
          }
        else
          {
            sendErrorMsg_P (ERRORMSG_INVALID_ARGS);
          }
        break;
      }
      default:
      {
        // notify of invalid cmd
        sendUnknownCmdIDMsg();
      }
    }
}

void PulseTrainAddon::loop()
{
  for (int i = 0; i < MAX_PULSETRAINS; i++)
    pulsetrains[i].poll();
}
