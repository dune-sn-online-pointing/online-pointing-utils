#ifndef WRITE_TPS_SIMPLE_H
#define WRITE_TPS_SIMPLE_H

#include <string>
#include <vector>
#include "../translate/TriggerPrimitiveSimple.hpp"
#include "../translate/TrueParticleSimple.h"
#include "../translate/NeutrinoSimple.h"


void write_tps_simple(
	const std::string& out_filename,
	const std::vector<std::vector<TriggerPrimitiveSimple>>& tps_by_event,
	const std::vector<std::vector<TrueParticleSimple>>& true_particles_by_event,
	const std::vector<std::vector<NeutrinoSimple>>& neutrinos_by_event
);

#endif // WRITE_TPS_SIMPLE_H