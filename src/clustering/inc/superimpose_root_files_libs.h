#ifndef SUPERIMPOSE_ROOT_FILES_LIBS_HD
#define SUPERIMPOSE_ROOT_FILES_LIBS_HD

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <climits>

// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"

#include "Cluster.h"

Cluster filter_clusters_within_radius(std::vector<Cluster>& clusters, float radius);




#endif