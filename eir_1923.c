/*  eir_1923 - Experiments in Representation
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
  LADSPA_Data *output;
  LADSPA_Data *input;
  LADSPA_Data *mantissa;
  LADSPA_Data *exponent;
};

static LADSPA_PortDescriptor port_descriptor[] = {
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
};

static LADSPA_PortRangeHint port_hints[] = {
  { 0 , 0 , 0 },
  { 0 , 0 , 0 },
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MAXIMUM, 0.0f , 23.0f },
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MAXIMUM, 1.0f , 8.0f },
};

static const char * port_names[] = {
  "Output",
  "Intput",
  "Mantissa",
  "Exponent",
};

static LADSPA_Handle instantiate(const LADSPA_Descriptor * Descriptor,
                                 unsigned long SampleRate);
static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                         LADSPA_Data * DataLocation);
static void cleanup(LADSPA_Handle Instance);
static void eir(LADSPA_Handle Instance, unsigned long SampleCount);

static LADSPA_Descriptor descriptors[] =
{
 {
  1923,
  "eir",
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "Experiments in Representation",
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
  eir,
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
  } else if (Port == 1) {
    data->input = DataLocation;
  } else if (Port == 2) {
    data->mantissa = DataLocation;
  } else if (Port == 3) {
    data->exponent = DataLocation;
  }
}

static void cleanup(LADSPA_Handle Instance)
{
  free(Instance);
}

static void eir(LADSPA_Handle Instance, unsigned long SampleCount)
{
  struct io *data = (struct io *) Instance;
  int mbits = lrintf(*(data->mantissa));
  int ebits = lrintf(*(data->exponent));
  unsigned long i;

  if (mbits > 23) mbits = 23;
  if (mbits < 0) mbits = 0;
  if (ebits > 8) ebits = 8;
  if (ebits < 1) ebits = 1;

  for (i = 0; i < SampleCount; ++i) {
    union { float f; unsigned int u; } sample;
    unsigned int exponent, mantissa, sign;

    sample.f = data->input[i];

    /* take sample apart */
    sign = (sample.u & 0x80000000) >> 31;
    exponent = (sample.u & 0x7f800000) >> 23;
    mantissa = (sample.u & 0x007fffff);

    /* shorten mantissa */
    
    unsigned int power = 1 << (23 - mbits);
    unsigned int fraction = mantissa % power;
    unsigned int whole = mantissa / power;

    if (fraction * 2 > power) {
      mantissa = (whole + 1) * power;
    } else {
      mantissa = whole * power;
    }

    if (mantissa >= 0x800000) {
      mantissa = 0;
      exponent+= 1;
    }

    /* range limit exponent */

    if (exponent > 126) {
      exponent = 127; mantissa= 0;
    }

    if ((1 << ebits) <= (127 - exponent)) {
      exponent = 0; mantissa= 0;
    }

    /* put sample back together */
    sample.u = (sign << 31) + (exponent << 23) + mantissa;

    data->output[i] = sample.f;
  }
}
