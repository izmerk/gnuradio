/* -*- c++ -*- */
/*
 * Copyright 2009-2011 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_fll_band_edge_cc.h>
#include <gr_io_signature.h>
#include <gr_expj.h>
#include <gr_math.h>
#include <cstdio>

#define M_TWOPI (2*M_PI)

float sinc(float x)
{
  if(x == 0)
    return 1;
  else
    return sin(M_PI*x)/(M_PI*x);
}
  


gr_fll_band_edge_cc_sptr
gr_make_fll_band_edge_cc (float samps_per_sym, float rolloff,
			  int filter_size, float bandwidth)
{
  return gnuradio::get_initial_sptr(new gr_fll_band_edge_cc (samps_per_sym, rolloff,
							    filter_size, bandwidth));
}


static int ios[] = {sizeof(gr_complex), sizeof(float), sizeof(float), sizeof(float)};
static std::vector<int> iosig(ios, ios+sizeof(ios)/sizeof(int));
gr_fll_band_edge_cc::gr_fll_band_edge_cc (float samps_per_sym, float rolloff,
					  int filter_size, float bandwidth)
  : gr_sync_block ("fll_band_edge_cc",
		   gr_make_io_signature (1, 1, sizeof(gr_complex)),
		   gr_make_io_signaturev (1, 4, iosig)),
    d_updated (false)
{
  // Initialize samples per symbol
  if(samps_per_sym <= 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid number of sps. Must be > 0.");
  }
  d_sps = samps_per_sym;
  
  // Initialize rolloff factor
  if(rolloff < 0 || rolloff > 1.0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid rolloff factor. Must be in [0,1].");
  }
  d_rolloff = rolloff;
  
  // Initialize filter length
  if(filter_size <= 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid filter size. Must be > 0.");
  }
  d_filter_size = filter_size;
  
  // Initialize loop bandwidth
  if(bandwidth < 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid bandwidth. Must be >= 0.");
  }
  d_loop_bw = bandwidth;

  // base this on the number of samples per symbol
  d_max_freq =  M_TWOPI * (2.0/samps_per_sym);
  d_min_freq = -M_TWOPI * (2.0/samps_per_sym);

  // Set the damping factor for a critically damped system
  d_damping = sqrt(2.0)/2.0;

  // Set the bandwidth, which will then call update_gains()
  set_loop_bandwidth(bandwidth);

  // Build the band edge filters
  design_filter(d_sps, d_rolloff, d_filter_size);

  // Initialize loop values
  d_freq = 0;
  d_phase = 0;
}

gr_fll_band_edge_cc::~gr_fll_band_edge_cc ()
{
}

void
gr_fll_band_edge_cc::set_loop_bandwidth(float bw) 
{
  if(bw < 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid bandwidth. Must be >= 0.");
  }
  
  d_loop_bw = bw;
  update_gains();
}

void
gr_fll_band_edge_cc::set_damping_factor(float df) 
{
  if(df < 0 || df > 1.0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid damping factor. Must be in [0,1].");
  }
  
  d_damping = df;
  update_gains();
}


void
gr_fll_band_edge_cc::set_alpha(float alpha)
{
  if(alpha < 0 || alpha > 1.0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid alpha. Must be in [0,1].");
  }
  d_alpha = alpha;
}

void
gr_fll_band_edge_cc::set_beta(float beta)
{
  if(beta < 0 || beta > 1.0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid beta. Must be in [0,1].");
  }
  d_beta = beta;
}

void
gr_fll_band_edge_cc::set_samples_per_symbol(float sps)
{
  if(sps <= 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid number of sps. Must be > 0.");
  }
  d_sps = sps;
  design_filter(d_sps, d_rolloff, d_filter_size);
}

void
gr_fll_band_edge_cc::set_rolloff(float rolloff)
{
  if(rolloff < 0 || rolloff > 1.0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid rolloff factor. Must be in [0,1].");
  }
  d_rolloff = rolloff;
  design_filter(d_sps, d_rolloff, d_filter_size);
}

void
gr_fll_band_edge_cc::set_filter_size(int filter_size)
{
  if(filter_size <= 0) {
    throw std::out_of_range ("gr_fll_band_edge_cc: invalid filter size. Must be > 0.");
  }
  d_filter_size = filter_size;
  design_filter(d_sps, d_rolloff, d_filter_size);
}

float
gr_fll_band_edge_cc::get_loop_bandwidth()
{
  return d_loop_bw;
}

float
gr_fll_band_edge_cc::get_damping_factor()
{
  return d_damping;
}

float
gr_fll_band_edge_cc::get_alpha()
{
  return d_alpha;
}

float
gr_fll_band_edge_cc::get_beta()
{
  return d_beta;
}

float
gr_fll_band_edge_cc::get_samples_per_symbol()
{
  return d_sps;
}

float
gr_fll_band_edge_cc::get_rolloff()
{
  return d_rolloff;
}

int
gr_fll_band_edge_cc:: get_filter_size()
{
  return d_filter_size;
}

void
gr_fll_band_edge_cc::update_gains() 
{
  float denom = (1.0 + 2.0*d_damping*d_loop_bw + d_loop_bw*d_loop_bw);
  d_alpha = (4*d_damping*d_loop_bw) / denom;
  d_beta = (4*d_loop_bw*d_loop_bw) / denom;
}

void
gr_fll_band_edge_cc::design_filter(float samps_per_sym, float rolloff, int filter_size)
{
  int M = rint(filter_size / samps_per_sym);
  float power = 0;

  // Create the baseband filter by adding two sincs together
  std::vector<float> bb_taps;
  for(int i = 0; i < filter_size; i++) {
    float k = -M + i*2.0/samps_per_sym;
    float tap = sinc(rolloff*k - 0.5) + sinc(rolloff*k + 0.5);
    power += tap;
    
    bb_taps.push_back(tap);
  }

  d_taps_lower.resize(filter_size);
  d_taps_upper.resize(filter_size);

  // Create the band edge filters by spinning the baseband
  // filter up and down to the right places in frequency.
  // Also, normalize the power in the filters
  int N = (bb_taps.size() - 1.0)/2.0;
  for(int i = 0; i < filter_size; i++) {
    float tap = bb_taps[i] / power;
    
    float k = (-N + (int)i)/(2.0*samps_per_sym);
    
    gr_complex t1 = tap * gr_expj(-M_TWOPI*(1+rolloff)*k);
    gr_complex t2 = tap * gr_expj(M_TWOPI*(1+rolloff)*k);
    
    d_taps_lower[filter_size-i-1] = t1;
    d_taps_upper[filter_size-i-1] = t2;
  }
  
  d_updated = true;

  // Set the history to ensure enough input items for each filter
  set_history(filter_size+1);
}

void
gr_fll_band_edge_cc::print_taps()
{
  unsigned int i;

  printf("Upper Band-edge: [");
  for(i = 0; i < d_taps_upper.size(); i++) {
    printf(" %.4e + %.4ej,", d_taps_upper[i].real(), d_taps_upper[i].imag());
  }
  printf("]\n\n");

  printf("Lower Band-edge: [");
  for(i = 0; i < d_taps_lower.size(); i++) {
    printf(" %.4e + %.4ej,", d_taps_lower[i].real(), d_taps_lower[i].imag());
  }
  printf("]\n\n");
}

int
gr_fll_band_edge_cc::work (int noutput_items,
			   gr_vector_const_void_star &input_items,
			   gr_vector_void_star &output_items)
{
  const gr_complex *in  = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];

  float *frq = NULL;
  float *phs = NULL;
  float *err = NULL;
  if(output_items.size() == 4) {
    frq = (float *) output_items[1];
    phs = (float *) output_items[2];
    err = (float *) output_items[3];
  }

  if (d_updated) {
    d_updated = false;
    return 0;		     // history requirements may have changed.
  }

  int i;
  float error;
  gr_complex nco_out;
  gr_complex out_upper, out_lower;
  for(i = 0; i < noutput_items; i++) {
    nco_out = gr_expj(d_phase);
    out[i+d_filter_size-1] = in[i] * nco_out;

    // Perform the dot product of the output with the filters
    out_upper = 0;
    out_lower = 0;
    for(int k = 0; k < d_filter_size; k++) {
      out_upper += d_taps_upper[k] * out[i+k];
      out_lower += d_taps_lower[k] * out[i+k];
    }
    error = norm(out_lower) - norm(out_upper);

    d_freq = d_freq + d_beta * error;
    d_phase = d_phase + d_freq + d_alpha * error;

    if(d_phase > M_PI)
      d_phase -= M_TWOPI;
    else if(d_phase < -M_PI)
      d_phase += M_TWOPI;

    if (d_freq > d_max_freq)
      d_freq = d_max_freq;
    else if (d_freq < d_min_freq)
      d_freq = d_min_freq;

    if(output_items.size() == 4) {
      frq[i] = d_freq;
      phs[i] = d_phase;
      err[i] = error;
    }
  }

  return noutput_items;
}
