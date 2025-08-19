#ifndef PARAMETERS_GEOMETRY_H
#define PARAMETERS_GEOMETRY_H

#include <cmath>

// Geometry-related constants
static const int EVENTS_OFFSET = 0; // TODO: verify usage; kept for compatibility
static const double apa_lenght_in_cm = 230;
static const double wire_pitch_in_cm_collection = 0.479;
static const double wire_pitch_in_cm_induction_diagonal = 0.4669;
static const double apa_angle = (90 - 35.7);
static const double wire_pitch_in_cm_induction = wire_pitch_in_cm_induction_diagonal / sin(apa_angle * M_PI / 180);
static const double offset_between_apa_in_cm = 2.4;
static const double apa_height_in_cm = 598.4;
static const double apa_width_in_cm = 4.7;
static const double apa_angular_coeff = tan(apa_angle * M_PI / 180);

#endif // PARAMETERS_GEOMETRY_H
