#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include "root.h"
#include "TriggerPrimitive.hpp"
#include <vector>

std::vector<float> calculate_position(TriggerPrimitive* tp);
std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps);
float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);
float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);
float vectorDistance(std::vector<float> a, std::vector<float> b);

#endif // BACKTRACKING_H
