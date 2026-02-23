#ifndef GLOBAL_H
#define GLOBAL_H

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

#include <nlohmann/json.hpp>
#include <regex>

#include "Utils.h"
#include "verbosity.h"
#include "root.h" //includes std libs

// Shared global includes and symbols used across the project

static int standard_backtracker_error_margin = 10; // TPC ticks

#endif
