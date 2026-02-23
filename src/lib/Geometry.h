#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include "TriggerPrimitive.hpp"

/**
 * @brief Calculate Y coordinate from U-plane wire geometry given Z position
 * 
 * This function computes the Y coordinate using U-plane wire positions.
 * U-plane wires have specific angular orientation relative to the detector axes.
 * 
 * @param tps Vector of trigger primitives from U plane
 * @param z The Z coordinate to evaluate at (in cm)
 * @param x_sign Sign of X coordinate (+1 or -1) to determine which side of APA
 * @return float The predicted Y coordinate in cm
 */
float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);

/**
 * @brief Calculate Y coordinate from V-plane wire geometry given Z position
 * 
 * This function computes the Y coordinate using V-plane wire positions.
 * V-plane wires have specific angular orientation relative to the detector axes.
 * 
 * @param tps Vector of trigger primitives from V plane
 * @param z The Z coordinate to evaluate at (in cm)
 * @param x_sign Sign of X coordinate (+1 or -1) to determine which side of APA
 * @return float The predicted Y coordinate in cm
 */
float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);

#endif // GEOMETRY_H
