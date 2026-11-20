//////////////////////////////////////////////////////////
// Implementation for the JobConfig class.
// Declarations live in JobConfig.h.
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 21:52:25 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 01:09:38 UTC (override/-f flag)
#include "JobConfig.h"
#include "Global.h"          // LogInfo / LogThrowIf, and the global verboseMode/debugMode
#include "CmdLineParser.h"

#include <fstream>

void JobConfig::readArgs(int argc, char** argv) {
    CmdLineParser clp;
    // Tolerate extra/unrecognised args (e.g. a positional input file passed
    // alongside -j) instead of throwing.
    CmdLineParser::setIsFascist(false);
    clp.getDescription() << "> read_trigger_trees configuration" << std::endl;
    clp.addOption("json", {"-j", "--json"}, "JSON configuration file");
    clp.addTriggerOption("verboseMode", {"-v", "--verbose"}, "Run in verbose mode");
    clp.addTriggerOption("debugMode",   {"-d", "--debug"},   "Run in debug mode (implies verbose)");
    clp.addTriggerOption("override",    {"-f", "--override"}, "Overwrite existing output files");
    clp.parseCmdLine(argc, argv);

    jsonFile = clp.getOptionVal<std::string>("json", std::string(""));
    verbose  = clp.isOptionTriggered("verboseMode");
    debug    = clp.isOptionTriggered("debugMode");
    override = clp.isOptionTriggered("override");
    if (debug) verbose = true;

    // Propagate to the global flags the rest of the code reads.
    verboseMode = verbose;
    debugMode   = debug;

    if (verbose) LogInfo << "JobConfig: json=" << (jsonFile.empty() ? "(none)" : jsonFile)
                         << ", verbose=" << verbose << ", debug=" << debug << std::endl;
}

void JobConfig::readJson(const std::string& path) {
    std::ifstream in(path);
    LogThrowIf(!in.good(), "JobConfig: cannot open JSON file: " << path);
    in >> j_;
    if (verbose) LogInfo << "JobConfig: loaded configuration from " << path << std::endl;
}
