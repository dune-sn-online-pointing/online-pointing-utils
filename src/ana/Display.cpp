#include "Display.h"
#include <cmath>
#include <limits>

// Polygon area using Shoelace formula
double polygonArea(const std::vector<std::pair<double, double>>& vertices) {
    double sum1 = 0.0, sum2 = 0.0;
    int n = vertices.size();
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        sum1 += vertices[i].first * vertices[j].second;
        sum2 += vertices[j].first * vertices[i].second;
    }
    return 0.5 * std::abs(sum1 - sum2);
}

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
    result.valid = false;
    
    // Target area is the adc_integral + offset (threshold compensation)
    double target_area = adc_integral + offset;
    double threshold = offset / (time_end - time_start); // approximate threshold
    
    // Scan for best pentagon vertices that match the target area
    // Try different intermediate points (t1 between start-peak, t2 between peak-end)
    double best_diff = std::numeric_limits<double>::max();
    int best_t1 = -1;
    int best_t2 = -1;
    double best_y1 = 0.0;
    double best_y2 = 0.0;
    
    // Sample intermediate points
    int n_samples = 10; // number of candidate points to try
    for (int i = 1; i < n_samples; ++i) {
        double frac_left = double(i) / double(n_samples);
        int t1 = static_cast<int>(time_start + frac_left * (time_peak - time_start));
        if (t1 <= time_start || t1 >= time_peak) continue;
        
        for (int j = 1; j < n_samples; ++j) {
            double frac_right = double(j) / double(n_samples);
            int t2 = static_cast<int>(time_peak + frac_right * (time_end - time_peak));
            if (t2 <= time_peak || t2 >= time_end) continue;
            
            // Calculate intermediate heights for this configuration
            // Use linear interpolation as a starting point
            double y1 = threshold + frac_left * (adc_peak - threshold);
            double y2 = threshold + (1.0 - frac_right) * (adc_peak - threshold);
            
            // Ensure y1 and y2 are between threshold and peak
            if (y1 < threshold || y1 > adc_peak) continue;
            if (y2 < threshold || y2 > adc_peak) continue;
            
            // Calculate pentagon area with these vertices
            // Pentagon: (time_start, threshold), (t1, y1), (time_peak, adc_peak), (t2, y2), (time_end, threshold)
            std::vector<std::pair<double, double>> vertices = {
                {time_start, threshold},
                {t1, y1},
                {time_peak, adc_peak},
                {t2, y2},
                {time_end, threshold}
            };
            
            double area = polygonArea(vertices);
            double diff = std::abs(area - target_area);
            
            if (diff < best_diff) {
                best_diff = diff;
                best_t1 = t1;
                best_t2 = t2;
                best_y1 = y1;
                best_y2 = y2;
            }
        }
    }
    
    // If we found a reasonable solution, use it
    if (best_t1 > 0 && best_t2 > 0) {
        result.time_int_rise = best_t1;
        result.time_int_fall = best_t2;
        result.h_int_rise = best_y1;
        result.h_int_fall = best_y2;
        result.frac = frac; // keep original frac for compatibility
        result.valid = true;
        return result;
    }
    
    // Fallback to old parametric approach if scan failed
    result.frac = frac;
  
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
  double time_over_threshold = time_end - time_start;
  // target_area already defined above, reuse it
  
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
  
  // CRITICAL: time_end must be time_start + samples_over_threshold
  // This ensures we fill all bins from time_start to time_start+SoT
  int time_end = time_start + samples_over_threshold;
  
  // Clamp peak_time to be within [time_start, time_end]
  if (time_peak < time_start) time_peak = time_start;
  if (time_peak > time_end) time_peak = time_end;
  
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
  
  // EXTENDED RANGE: Pentagon base can extend 1-2 pixels beyond time_start and time_end
  // to better match the actual waveform shape
  int extended_start = time_start - 1;
  int extended_end = time_end + 1;
  
  // CRITICAL: Loop must include ALL ticks from extended_start to extended_end (inclusive start, exclusive end)
  for (int t = extended_start; t < extended_end; ++t) {
    double intensity = 0.0;
    
    if (t < time_start) {
      // Extended base before time_start - extrapolate from first segment
      double span = t_int_rise - time_start;
      if (span > 0) {
        double frac = double(t - time_start) / span;
        intensity = frac * params.h_int_rise;
        // Only fill if intensity is positive and above threshold
        if (intensity < threshold_adc * 0.5) intensity = 0.0;
      }
    }
    else if (t < t_int_rise) {
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
    else if (t < time_end) {
      // Segment 4: falling from h_int_fall to 0 (passes through threshold at time_end)
      double span = time_end - t_int_fall;
      if (span > 0) {
        double frac = double(t - t_int_fall) / span;
        intensity = params.h_int_fall - frac * params.h_int_fall;
      }
    }
    else {
      // Extended base after time_end - extrapolate from last segment
      double span = time_end - t_int_fall;
      if (span > 0) {
        double frac = double(t - t_int_fall) / span;
        intensity = params.h_int_fall - frac * params.h_int_fall;
        // Only fill if intensity is positive and above threshold
        if (intensity < threshold_adc * 0.5) intensity = 0.0;
      }
    }
    
    // Only fill bins with positive intensity
    if (intensity > 0) {
      int binx = frame->GetXaxis()->FindBin(ch_contiguous);
      int biny = frame->GetYaxis()->FindBin(t);
      double current = frame->GetBinContent(binx, biny);
      if (intensity > current) {
        frame->SetBinContent(binx, biny, intensity);
      }
    }
  }
}

void fillHistogramRectangle(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int samples_over_threshold,
  double adc_integral
) {
  if (!frame) return;
  
  // Rectangle mode: uniform intensity across all samples
  // intensity = total_integral / number_of_samples
  double uniform_intensity = (samples_over_threshold > 0) 
    ? adc_integral / double(samples_over_threshold) 
    : 0.0;
  
  int time_end = time_start + samples_over_threshold;
  
  // Fill all pixels from time_start to time_end with uniform intensity
  for (int t = time_start; t < time_end; ++t) {
    int binx = frame->GetXaxis()->FindBin(ch_contiguous);
    int biny = frame->GetYaxis()->FindBin(t);
    double current = frame->GetBinContent(binx, biny);
    if (uniform_intensity > current) {
      frame->SetBinContent(binx, biny, uniform_intensity);
    }
  }
}

