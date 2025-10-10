#include "Display.h"
#include "ParametersManager.h"
#include <TH2F.h>
#include <TCanvas.h>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Display {

PentagonParams calculatePentagonParams(
  double time_start, 
  double time_peak, 
  double time_end,
  double adc_peak, 
  double adc_integral, 
  double frac,
  double offset
) {
  PentagonParams result;
  result.frac = frac;
  result.valid = false;
  
  // Bounds checking for frac
  if (frac < 0.1) {
    result.h_int_rise = 0;
    result.h_int_fall = 0;
    result.frac = 0.1;
    result.valid = true;
    return result;
  }
  if (frac > 0.9) {
    result.h_int_rise = adc_peak;
    result.h_int_fall = adc_peak;
    result.frac = 0.9;
    result.valid = true;
    return result;
  }
  
  // Calculate time positions
  double ah = time_peak - time_start;  // rise time
  double bh = time_end - time_peak;     // fall time
  double ch = adc_peak;
  
  double ad = ah * frac;
  double dh = ah - ad;
  double eb = bh * frac;
  double he = bh - eb;
  
  result.time_int_rise = time_start + ad;
  result.time_int_fall = time_peak + eb;
  
  // Calculate intermediate height from area constraint
  // Pentagon with vertices at baseline (0):
  // Vertices: (time_start, 0), (t_int_rise, h), (time_peak, peak), (t_int_fall, h), (time_end, 0)
  // t_int_rise = time_start + frac*dh,  t_int_fall = time_peak + frac*he
  // Total Area = 0.5*h*(time_end - time_start) + 0.5*peak*(t_int_fall - t_int_rise)
  // where t_int_fall - t_int_rise = (time_peak + frac*he) - (time_start + frac*dh)
  //                                = dh + frac*he - frac*dh = dh*(1-frac) + frac*he
  // Solving for h:
  // target_area = 0.5*h*time_over_threshold + 0.5*peak*(dh*(1-frac) + frac*he)
  // h = (2*target_area - peak*(dh*(1-frac) + frac*he)) / time_over_threshold
  
  double time_over_threshold = time_end - time_start;
  double target_area = adc_integral + offset;
  
  double peak_width = dh * (1.0 - frac) + frac * he;
  double intermediate_height = (2 * target_area - ch * peak_width) / time_over_threshold;
  
  // Validate intermediate height
  if (intermediate_height < 0) {
    // Adjust frac upward
    double new_frac = 1.0 - 0.5 * (1.0 - frac);
    return calculatePentagonParams(time_start, time_peak, time_end, adc_peak, 
                                   adc_integral, new_frac, offset);
  }
  else if (intermediate_height > ch) {
    // Adjust frac downward
    double new_frac = 0.5 * frac;
    return calculatePentagonParams(time_start, time_peak, time_end, adc_peak, 
                                   adc_integral, new_frac, offset);
  }
  
  // Valid result
  result.h_int_rise = intermediate_height;
  result.h_int_fall = intermediate_height;
  result.valid = true;
  
  return result;
}

void fillHistogramTriangle(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int samples_over_threshold,
  int samples_to_peak,
  int adc_peak,
  double threshold_adc
) {
  if (!frame) return;
  
  int time_end = time_start + std::max(1, samples_over_threshold);
  int peak_time = time_start + samples_to_peak;
  
  for (int t = time_start; t < time_end; ++t) {
    double intensity = adc_peak;
    
    // Rising edge
    if (t <= peak_time) {
      if (peak_time != time_start) {
        double frac = double(t - time_start) / double(peak_time - time_start);
        intensity = threshold_adc + frac * (adc_peak - threshold_adc);
      }
    }
    // Falling edge
    else {
      int fall_span = (time_end - 1) - peak_time;
      if (fall_span > 0) {
        double frac = double(t - peak_time) / double(fall_span);
        intensity = adc_peak - frac * (adc_peak - threshold_adc);
      }
    }
    
    int binx = frame->GetXaxis()->FindBin(ch_contiguous);
    int biny = frame->GetYaxis()->FindBin(t);
    double current = frame->GetBinContent(binx, biny);
    if (intensity > current) {
      frame->SetBinContent(binx, biny, intensity);
    }
  }
}

void fillHistogramPentagon(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int time_peak,
  int samples_over_threshold,
  int adc_peak,
  double adc_integral,
  double threshold_adc,
  double offset
) {
  if (!frame) return;
  
  int time_end = time_start + std::max(1, samples_over_threshold);
  
  // Calculate pentagon parameters
  PentagonParams params = calculatePentagonParams(
    time_start, time_peak, time_end,
    adc_peak, adc_integral, 0.5, offset
  );
  
  if (!params.valid) {
    // Fallback to triangle if pentagon calculation fails
    int samples_to_peak = time_peak - time_start;
    fillHistogramTriangle(frame, ch_contiguous, time_start, samples_over_threshold,
                         samples_to_peak, adc_peak, threshold_adc);
    return;
  }
  
  // Fill histogram with pentagon shape
  // Pentagon vertices: (time_start, 0), (time_int_rise, h_int_rise), (time_peak, adc_peak),
  //                     (time_int_fall, h_int_fall), (time_end, 0)
  // The sides must pass through (time_start, threshold_adc) and (time_end, threshold_adc)
  
  int t_int_rise = static_cast<int>(std::round(params.time_int_rise));
  int t_int_fall = static_cast<int>(std::round(params.time_int_fall));
  
  for (int t = time_start; t < time_end; ++t) {
    double intensity = 0.0;
    
    if (t < t_int_rise) {
      // Segment 1: rising from 0 to h_int_rise (passes through threshold at time_start)
      double span = t_int_rise - time_start;
      if (span > 0) {
        double frac = double(t - time_start) / span;
        intensity = frac * params.h_int_rise;
      }
    }
    else if (t < time_peak) {
      // Segment 2: rising from h_int_rise to adc_peak
      double span = time_peak - t_int_rise;
      if (span > 0) {
        double frac = double(t - t_int_rise) / span;
        intensity = params.h_int_rise + frac * (adc_peak - params.h_int_rise);
      } else {
        intensity = adc_peak;
      }
    }
    else if (t == time_peak) {
      // Peak
      intensity = adc_peak;
    }
    else if (t <= t_int_fall) {
      // Segment 3: falling from adc_peak to h_int_fall
      double span = t_int_fall - time_peak;
      if (span > 0) {
        double frac = double(t - time_peak) / span;
        intensity = adc_peak - frac * (adc_peak - params.h_int_fall);
      } else {
        intensity = params.h_int_fall;
      }
    }
    else {
      // Segment 4: falling from h_int_fall to 0 (passes through threshold at time_end)
      double span = time_end - t_int_fall;
      if (span > 0) {
        double frac = double(t - t_int_fall) / span;
        intensity = params.h_int_fall - frac * params.h_int_fall;
      }
    }
    
    int binx = frame->GetXaxis()->FindBin(ch_contiguous);
    int biny = frame->GetYaxis()->FindBin(t);
    double current = frame->GetBinContent(binx, biny);
    if (intensity > current) {
      frame->SetBinContent(binx, biny, intensity);
    }
  }
}

} // namespace Display
