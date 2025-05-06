#include "utils.h"


std::vector<TriggerPrimitive*>& getPrimitivesForView(std::string* view, std::vector<TriggerPrimitive>& tps) {
    std::vector<TriggerPrimitive*> tps_for_view;

    for (auto & tp : tps) {
        if (tp.view == *view) {
            tps_for_view.push_back(&tp);
        }
    }
    return tps_for_view;
};
