#include "utils.h"


const std::vector<const TriggerPrimitive*>& getPrimitivesForView(char* view, const TriggerPrimitive* tps){
    std::vector<const TriggerPrimitive*> tps_for_view;
    for (int i = 0; i < tps.size(); i++) {
        if (tps[i].view == view) {
            tps_for_view.push_back(&tps[i]);
        }
    }
    return tps_for_view;
};
