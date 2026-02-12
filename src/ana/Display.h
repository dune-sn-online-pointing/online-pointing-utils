#ifndef DISPLAY_H
#define DISPLAY_H

#include "global.h"

enum DrawMode {
  TRIANGLE,
  PENTAGON,
  RECTANGLE
};

// Pentagon calculation result
struct PentagonParams {
  double time_int_rise;
  double h_int_rise;
  double time_int_fall;
  double h_int_fall;
  double frac;
  bool valid;
};

/**
 * @brief Calculate intermediate height for pentagon shape given a fraction
 * @param time_start Start time of TP
 * @param time_peak Peak time of TP
 * @param time_end End time (time_start + samples_over_threshold)
 * @param adc_peak Peak ADC value
 * @param adc_integral Total ADC integral
 * @param frac Fraction for positioning intermediate vertices (0-1)
 * @param threshold_adc Threshold ADC for the plane (60 for X, 70 for U/V) - the plateau baseline
 * @return PentagonParams with calculated intermediate positions and heights
 */
PentagonParams calculatePentagonParams(
  double time_start, 
  double time_peak, 
  double time_end,
  double adc_peak, 
  double adc_integral, 
  double frac,
  double threshold_adc
);

/**
 * @brief Fill histogram with TP using triangle model
 * @param frame Histogram to fill
 * @param ch_contiguous Contiguous channel index
 * @param time_start Start time in ticks
 * @param samples_over_threshold Duration in ticks
 * @param samples_to_peak Samples to peak
 * @param adc_peak Peak ADC value
 * @param threshold_adc Threshold ADC value (default 60)
 */
void fillHistogramTriangle(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int samples_over_threshold,
  int samples_to_peak,
  int adc_peak,
  double threshold_adc = 60.0
);

/**
 * @brief Fill histogram with TP using pentagon model
 * @param frame Histogram to fill
 * @param ch_contiguous Contiguous channel index
 * @param time_start Start time in ticks
 * @param time_peak Peak time in ticks
 * @param samples_over_threshold Duration in ticks
 * @param adc_peak Peak ADC value
 * @param adc_integral Total ADC integral
 * @param threshold_adc Threshold ADC value (60 for X, 70 for U/V)
 */
void fillHistogramPentagon(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int time_peak,
  int samples_over_threshold,
  int adc_peak,
  double adc_integral,
  double threshold_adc
);

/**
 * @brief Fill histogram with TP using rectangle model (uniform intensity)
 * @param frame Histogram to fill
 * @param ch_contiguous Contiguous channel index
 * @param time_start Start time in ticks
 * @param samples_over_threshold Duration in ticks
 * @param adc_integral Total ADC integral
 */
void fillHistogramRectangle(
  TH2F* frame,
  int ch_contiguous,
  int time_start,
  int samples_over_threshold,
  double adc_integral
);

/**
 * @brief Draw current cluster/event in the viewer
 * @param canvas Canvas to draw on
 * @param items Vector of display items
 * @param idx Current index
 * @param mode Drawing mode (TRIANGLE or PENTAGON)
 */
void drawCluster(
  TCanvas* canvas,
  const std::vector<struct ViewerItem>& items,
  int idx,
  DrawMode mode
);

#endif // DISPLAY_H
