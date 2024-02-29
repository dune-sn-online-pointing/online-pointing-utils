#include <vector>
#include "position_calculator.h"
#include "cluster.h"

std::vector<int> calculate_position(std::vector<double> tp) { // only works for plane 2
    int z_apa_offset = int(tp[variables_to_index["channel"]]) / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
    int z_channel_offset = ((int(tp[variables_to_index["channel"]]) % 2560 - 1600) % 480) * wire_pitch_in_cm_collection;
    int z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    int y = 0;
    int x_signs = ((int(tp[variables_to_index["channel"]]) % 2560-2080<0) ? -1 : 1);
    int x = ((int(tp[variables_to_index["time_peak"]]) % EVENTS_OFFSET )* time_tick_in_cm + apa_width_in_cm/2) * x_signs;
    return {x, y, z};
}

std::vector<std::vector<int>> validate_position_calculation(std::vector<std::vector<double>> tps) {
    std::vector<std::vector<int>> positions;
    for (auto& tp : tps) {
        if (tp[0] == 2 and tp[variables_to_index["true_x"]] and tp[variables_to_index["true_y"]] and tp[variables_to_index["true_z"]]) {
            positions.push_back({tp[variables_to_index["true_x"]], tp[variables_to_index["true_y"]], tp[variables_to_index["true_z"]], calculate_position(tp)[0], calculate_position(tp)[1], calculate_position(tp)[2]});
        }
    }
    return positions;
}
