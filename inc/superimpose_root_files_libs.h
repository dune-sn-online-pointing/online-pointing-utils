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

#include "group.h"

group filter_groups_within_radius(std::vector<group>& groups, float radius);

float distance(group group1, group group2);



#endif