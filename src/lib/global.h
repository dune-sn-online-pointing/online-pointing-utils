#ifndef GLOBAL_H
#define GLOBAL_H

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

#include <nlohmann/json.hpp>

#include "utils.h"
#include "verbosity.h"
#include "root.h" //includes std libs

// This is a header guard for global.cpp, which includes most common includes of the project

static int standard_backtracker_error_margin = 10; // TPC ticks

#endif
