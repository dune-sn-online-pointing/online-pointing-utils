#ifndef PARAMETERS_TIMING_H
#define PARAMETERS_TIMING_H

// Timing-related constants
static const double time_tick_in_cm = 0.0805;
static double drift_speed = 1.61e-4; // cm/ns

static int conversion_tdc_to_tpc = 32;
static const float clock_tick = 16.; // ns
static const float TPC_sample_length = clock_tick * conversion_tdc_to_tpc; // 512 ns
// Time window to associate TPs to true particles (in TPC ticks converted to TDC ticks)
static int time_window = 0 * conversion_tdc_to_tpc;
static const int backtracker_error_margin = 0; // duplicate guard if used elsewhere

#endif // PARAMETERS_TIMING_H
