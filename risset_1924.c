/*  risset_1924 - Continuous Risset Scales
    Copyright (C) 2002-2005  Nick Lamb <njl+ladspa@filter.tlrmx.org>

    Thanks to Steve Harris for speed-ups and other tweaks

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

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

/* size of sinus wavetable */
#define TBL_SIZE 2048

struct io {
  float rate;
  LADSPA_Data *output;
  LADSPA_Data *master;
  LADSPA_Data *base;
  LADSPA_Data *speed;
  float phi;
  float increment;
  float gain;
  float freq;
  float *tbl;
};

static LADSPA_PortDescriptor port_descriptor[] = {
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
};

static LADSPA_PortRangeHint port_hints[] = {
  { 0 , 0 , 0 },
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0, -90.0f , 6.0f },
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_SAMPLE_RATE, 0.002f , 0.05f },
  { LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH, -6.0f , 12.0f },
};

static const char * port_names[] = {
  "Output",
  "Gain (dB)",
  "Core Frequency (Hz)",
  "Speed",
};

static LADSPA_Handle instantiate(const LADSPA_Descriptor * Descriptor,
                                 unsigned long SampleRate);
static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                         LADSPA_Data * DataLocation);
static void cleanup(LADSPA_Handle Instance);
static void activate(LADSPA_Handle Instance);
static void risset(LADSPA_Handle Instance, unsigned long SampleCount);

static void f_sin(const float * const tbl, const float phase, float *out);

static LADSPA_Descriptor descriptors[] =
{
 {
  1924,
  "rissetScales",
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "Continuous Risset Scales",
  "Nick Lamb <njl+ladspa@filter.tlrmx.org>",
  "GPL 2002-2005",
  sizeof(port_descriptor) / sizeof(LADSPA_PortDescriptor),
  port_descriptor,
  port_names,
  port_hints,
  NULL,
  instantiate,
  connect_port,
  activate,
  risset,
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
  struct io* data = calloc(sizeof(struct io), 1);
  float *tbl = malloc(sizeof(float) * TBL_SIZE + 1);
  int i;

  data->rate = (float) SampleRate;
  data->tbl = tbl;
  for (i=0; i<TBL_SIZE + 1; i++) {
    tbl[i] = sin((double)i / (double)TBL_SIZE * M_PI * 2.0);
  }

  return (LADSPA_Handle) data;
}

static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                 LADSPA_Data * DataLocation)
{
  struct io *data = (struct io *) Instance;
  if (Port == 0) {
    data->output = DataLocation;
  } else if (Port == 1) {
    data->master = DataLocation;
  } else if (Port == 2) {
    data->base = DataLocation;
  } else if (Port == 3) {
    data->speed = DataLocation;
  }
}

static void cleanup(LADSPA_Handle Instance)
{
  free(Instance);
}

static void activate(LADSPA_Handle Instance)
{
  struct io *data = (struct io *) Instance;
  data->phi = 0.0f;
  data->increment = 0.0f;
  data->gain = 0.0f;
}

static const float sqrt_1_2pi = 0.39894228040143267793;
/* 1.0 / ln(2) */
static const float ln2r = 1.442695041f;
/* 1.0 / 7.0 */
static const float f1_7 = 0.142857142f;

typedef union {
  float f;
  int32_t i;
} ls_pcast32;

static inline float f_pow2(float x)
{
  ls_pcast32 *px, tx, lx;
  float dx;

  px = (ls_pcast32 *)&x; // store address of float as long pointer
  tx.f = (x-0.5f) + (3<<22); // temporary value for truncation
  lx.i = tx.i - 0x4b400000; // integer power of 2
  dx = x - (float)lx.i; // float remainder of power of 2

  x = 1.0f + dx * (0.6960656421638072f + // cubic apporoximation of 2^x
             dx * (0.224494337302845f +  // for x in the range [0, 1]
             dx * (0.07944023841053369f)));
  (*px).i += (lx.i << 23); // add integer power of 2 to exponent

  return (*px).f;
}

static inline float normal(float x)
{
  return sqrt_1_2pi * f_pow2(-ln2r * x * x);
}

static void risset(LADSPA_Handle Instance, unsigned long SampleCount)
{
  struct io *data = (struct io *) Instance;
  unsigned long i;
  const float master = DB_CO(*(data->master)) * f1_7;
  const float base = *(data->base) * 0.125 / (data->rate);
  const float speed = *(data->speed) / (data->rate * 60.0);
  const float d_gain = ( master - data->gain) / SampleCount;
  const float d_freq = ( base - data->freq) / SampleCount;
  float phi = data->phi;
  float increment = data->increment;
  float gain = data->gain;
  float freq = data->freq;

  if (increment == 0.0f) increment = freq;

  for (i = 0; i < SampleCount; ++i) {
    const float iob = increment / freq;
    float vals[7];

    f_sin(data->tbl, phi, vals);
    double sample = vals[0] * normal( -4.5f + iob)
                  + vals[1] * normal( -3.5f + iob)
                  + vals[2] * normal( -2.5f + iob)
                  + vals[3] * normal( -1.5f + iob)
                  + vals[4] * normal( -0.5f + iob)
                  + vals[5] * normal(  0.5f + iob)
                  + vals[6] * normal(  1.5f + iob);

    phi += increment;
    increment += (freq * speed);
    if (increment < freq) {
      increment += freq;
      phi = 2.0 * phi;
    } else if (increment >= 2.0f * freq) {
      increment -= freq;
      phi = 0.5 * phi;
    }
    if (phi > 1.0f) {
      phi -= 1.0f;
    }

    data->output[i] = sample * gain;
    gain += d_gain;
    freq += d_freq;
  }

  /* keep state */
  data->phi = phi;
  data->increment = increment;
  data->gain = master; /* exactly */
  data->freq = base; /* exactly */
}

static void f_sin(const float * const tbl, const float phase, float *out)
{
  /* calculate phase as a 20.12 fixedpoint number */
  unsigned int iph = lrintf(phase * 4096.0 * TBL_SIZE);
  unsigned int i;

  for (i=0; i<7; i++) {
    unsigned int idx = (iph >> 12) & (TBL_SIZE - 1);
    float frac = (iph && 0xfff) * 0.0002459419f;

    /* calculate the linear interpolation of the table */
    out[i] = tbl[idx] + frac * (tbl[idx+1] - tbl[idx]);
    /* double the phase for the next octave */
    iph += iph;
  }
}
