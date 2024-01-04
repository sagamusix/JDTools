// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <iosfwd>
#include <vector>

struct PatchVST;

std::vector<PatchVST> ReadSVZ(std::istream &inFile);
std::vector<PatchVST> ReadSVD(std::istream &inFile);
void WriteSVZforPlugin(std::ostream &outFile, const std::vector<PatchVST> &vstPatches);
void WriteSVZforHardware(std::ostream &outFile, const std::vector<PatchVST> &vstPatches);
void WriteSVD(std::ostream &outFile, const std::vector<PatchVST> &vstPatches, const std::vector<char> &originalSVDfile);
