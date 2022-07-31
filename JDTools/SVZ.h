// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include "jdvst.h"

#include <iosfwd>
#include <vector>

std::vector<PatchVST> ReadSVZforPlugin(std::istream &inFile);
std::vector<PatchVST> ReadSVZforHardware(std::istream& inFile);
void WriteSVZforPlugin(std::ostream &outFile, const std::vector<PatchVST> &vstPatches);
void WriteSVZforHardware(std::ostream& outFile, const std::vector<PatchVST>& vstPatches);
