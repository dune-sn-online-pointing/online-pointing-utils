#include <vector>
#include "../inc/position_calculator.h" 

std::vector<int> calculate_position(std::vector<int> tp) { // only works for plane 2
    int z_apa_offset = tp[3] / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
    int z_channel_offset = ((tp[3] % 2560 - 1600) % 480) * wire_pitch_in_cm_collection;
    int z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    int y = 0;
    int x_signs = ((tp[3] % 2560-2080)<0) ? -1 : 1;
    int x = ((tp[2] - 4792* tp[7] )* time_tick_in_cm + apa_width_in_cm/2) * x_signs;
    return {x, y, z};
}


