/*  noise_1922 - Integer Noise
    Copyright (C) 2002-2005  Nick Lamb <njl+ladspa@filter.tlrmx.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

/* why is this necessary ? */
extern long int lrintf(float x);

struct io {
  LADSPA_Data *bits;
  LADSPA_Data *output;
};

static LADSPA_PortDescriptor port_descriptor[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
};

static LADSPA_PortRangeHint port_hints[] = {
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM , 1.0f , 24.0f },
  { 0 , 0 , 0 },
};

static const char * port_names[] = {
  "Bits",
  "Output",
};

static LADSPA_Handle instantiate(const LADSPA_Descriptor * Descriptor,
                                 unsigned long SampleRate);
static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                         LADSPA_Data * DataLocation);
static void cleanup(LADSPA_Handle Instance);
static void noise_int(LADSPA_Handle Instance, unsigned long SampleCount);

static LADSPA_Descriptor descriptors[] =
{
 {
  1922,
  "intNoise",
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "Integer Noise",
  "Nick Lamb <njl+ladspa@filter.tlrmx.org>",
  "GPL 2002-2005",
  sizeof(port_descriptor) / sizeof(LADSPA_PortDescriptor),
  port_descriptor,
  port_names,
  port_hints,
  NULL,
  instantiate,
  connect_port,
  NULL,
  noise_int,
  NULL,
  NULL,
  NULL,
  cleanup
 },
};

const LADSPA_Descriptor * ladspa_descriptor(unsigned long Index)
{
  if (Index < (sizeof(descriptors)/sizeof(LADSPA_Descriptor))) {
    return &descriptors[Index];
  } else {
    return NULL;
  }
}

static LADSPA_Handle instantiate(const LADSPA_Descriptor * Descriptor,
                                unsigned long SampleRate)
{
  return (LADSPA_Handle) calloc(sizeof(struct io), 1);
}

static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                 LADSPA_Data * DataLocation)
{
  struct io *data = (struct io *) Instance;
  if (Port == 0) {
    data->bits = DataLocation;
  } else if (Port == 1) {
    data->output = DataLocation;
  }
}

static void cleanup(LADSPA_Handle Instance)
{
  free(Instance);
}

static void noise_int(LADSPA_Handle Instance, unsigned long SampleCount)
{
  struct io *data = (struct io *) Instance;
  float bits = *(data->bits);

  if (bits < 1.0f) bits = 1.0f;
  if (bits > 24.0f) bits = 24.0f;

  float power = powf(2.0f, bits);
  float next = powf(2.0f, ceilf(bits));
  long precision = lrintf(power);

  unsigned long i;
  for (i = 0; i < SampleCount; ++i) {
    float r = random() % precision;

    float result = ceilf(r * (next / power));

    data->output[i] = (result / next) - 1.0f;
  }
}
