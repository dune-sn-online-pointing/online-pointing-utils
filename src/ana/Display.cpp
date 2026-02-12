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
  double threshold_adc
) {
    PentagonParams result;
    result.valid = false;
    
    // CORRECT LOGIC:
    // 1. Subtract threshold*samples_over_threshold from adc_integral to get residual area
    // 2. Place vertices at midpoints: between (start, peak) and (peak, end)
    // 3. Find vertex heights that minimize difference with residual area
    
    double samples_over_threshold = time_end - time_start;
    double threshold_area = threshold_adc * samples_over_threshold;
    double residual_area = adc_integral - threshold_area;
    
    // If residual is negative or zero, degenerate to threshold
    if (residual_area <= 0) {
        result.time_int_rise = (time_start + time_peak) / 2.0;
        result.time_int_fall = (time_peak + time_end) / 2.0;
        result.h_int_rise = threshold_adc;
        result.h_int_fall = threshold_adc;
        result.frac = 0.5;
        result.valid = true;
        return result;
    }
    
    // Vertex time positions at midpoints
    double t1 = (time_start + time_peak) / 2.0;
    double t2 = (time_peak + time_end) / 2.0;
    
    // Pentagon vertices (above threshold baseline):
    // (time_start, 0), (t1, h1), (time_peak, adc_peak - threshold), (t2, h2), (time_end, 0)
    // where heights are relative to threshold
    
    double peak_height_above_threshold = adc_peak - threshold_adc;
    
    // Scan for best vertex heights
    double best_diff = std::numeric_limits<double>::max();
    double best_h1 = 0.0;
    double best_h2 = 0.0;
    
    int n_samples = 20; // number of candidate heights to try
    for (int i = 0; i <= n_samples; ++i) {
        double frac_h = double(i) / double(n_samples);
        double h1 = frac_h * peak_height_above_threshold;
        
        for (int j = 0; j <= n_samples; ++j) {
            double frac_h2 = double(j) / double(n_samples);
            double h2 = frac_h2 * peak_height_above_threshold;
            
            // Calculate pentagon area (above threshold baseline)
            // Pentagon: (time_start, 0), (t1, h1), (time_peak, peak_height), (t2, h2), (time_end, 0)
            std::vector<std::pair<double, double>> vertices = {
                {time_start, 0.0},
                {t1, h1},
                {time_peak, peak_height_above_threshold},
                {t2, h2},
                {time_end, 0.0}
            };
            
            double area = polygonArea(vertices);
            double diff = std::abs(area - residual_area);
            
            if (diff < best_diff) {
                best_diff = diff;
                best_h1 = h1;
                best_h2 = h2;
            }
        }
    }
    
    // Convert heights back to absolute values (add threshold)
    result.time_int_rise = t1;
    result.time_int_fall = t2;
    result.h_int_rise = threshold_adc + best_h1;
    result.h_int_fall = threshold_adc + best_h2;
    result.frac = 0.5; // midpoint
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
  double threshold_adc
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
    adc_peak, adc_integral, 0.5, threshold_adc
  );
  
  if (!params.valid) {
    // Fallback to triangle if pentagon calculation fails
    int samples_to_peak = time_peak - time_start;
    fillHistogramTriangle(frame, ch_contiguous, time_start, samples_over_threshold,
                         samples_to_peak, adc_peak, threshold_adc);
    return;
  }
  
  // Fill histogram with pentagon shape
  // Pentagon vertices: (time_start, threshold), (time_int_rise, h_int_rise), (time_peak, adc_peak),
  //                     (time_int_fall, h_int_fall), (time_end, threshold)
  // The pentagon starts and ends at threshold, NOT at 0
  
  int t_int_rise = static_cast<int>(std::round(params.time_int_rise));
  int t_int_fall = static_cast<int>(std::round(params.time_int_fall));
  
  // Loop through all ticks from time_start to time_end
  for (int t = time_start; t < time_end; ++t) {
    double intensity = threshold_adc;  // Start from threshold baseline
    
    if (t < t_int_rise) {
      // Segment 1: rising from threshold to h_int_rise
      double span = t_int_rise - time_start;
      if (span > 0) {
        double frac = double(t - time_start) / span;
        intensity = threshold_adc + frac * (params.h_int_rise - threshold_adc);
      } else {
        intensity = params.h_int_rise;
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
      // Segment 4: falling from h_int_fall to threshold
      double span = time_end - t_int_fall;
      if (span > 0) {
        double frac = double(t - t_int_fall) / span;
        intensity = params.h_int_fall - frac * (params.h_int_fall - threshold_adc);
      } else {
        intensity = threshold_adc;
      }
    }
    
    // Fill the bin
    int binx = frame->GetXaxis()->FindBin(ch_contiguous);
    int biny = frame->GetYaxis()->FindBin(t);
    double current = frame->GetBinContent(binx, biny);
    if (intensity > current) {
      frame->SetBinContent(binx, biny, intensity);
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

