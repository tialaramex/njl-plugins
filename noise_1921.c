/*  noise_1921 - IEEE Single Precision Noise
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

struct io {
  LADSPA_Data *output;
};

static LADSPA_PortDescriptor port_descriptor[] = {
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
};

static LADSPA_PortRangeHint port_hints[] = {
  { 0 , 0 , 0 },
};

static const char * port_names[] = {
  "Output",
};

static LADSPA_Handle instantiate(const LADSPA_Descriptor * Descriptor,
                                 unsigned long SampleRate);
static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                         LADSPA_Data * DataLocation);
static void cleanup(LADSPA_Handle Instance);
static void noise_float(LADSPA_Handle Instance, unsigned long SampleCount);

static LADSPA_Descriptor descriptors[] =
{
 {
  1921,
  "floatNoise",
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "IEEE Single Precision Noise",
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
  noise_float,
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
    data->output = DataLocation;
  }
}

static void cleanup(LADSPA_Handle Instance)
{
  free(Instance);
}

static void noise_float(LADSPA_Handle Instance, unsigned long SampleCount)
{
  struct io *data = (struct io *) Instance;
  unsigned long i;

  for (i = 0; i < SampleCount; ++i) {
    union { float f; unsigned int u; } sample;
    unsigned int exponent, mantissa, sign;

    sign = random() % 2;
    exponent = random() % 127;
    mantissa = random() % 0x800000;

    if (exponent > 0) {
      sample.u = (sign << 31) + (exponent << 23) + mantissa;
    } else {
      sample.u = (sign << 31);
    }

    data->output[i] = sample.f;
  }
}
