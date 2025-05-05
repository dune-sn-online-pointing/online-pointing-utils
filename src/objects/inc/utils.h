#include <map>
#include <string>
#include <vector>

std::vector<std::string> plane_names = {"U", "V", "X"};

static const TPC_sampling_rate = 512.; // ns

// this is just a vector of pointers, to not create copies of the TriggerPrimitive objects
const std::vector<const TriggerPrimitive*>& getPrimitivesForView( char* view, const TriggerPrimitive* tps);