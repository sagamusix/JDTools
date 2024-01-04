// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <vector>

struct Patch800;
struct Patch990;
struct PatchVST;

void ConvertPatch800To990(const Patch800 &p800, Patch990 &p990);
void ConvertPatch990To800(const Patch990 &p990, Patch800 &p800);

void ConvertPatch800ToVST(const Patch800 &p800, PatchVST &pVST);
void ConvertPatchVSTTo800(const PatchVST &pVST, Patch800 &p800);

struct SpecialSetup800;
struct SpecialSetup990;

void ConvertSetup800To990(const SpecialSetup800 &s800, SpecialSetup990 &s990);
void ConvertSetup990To800(const SpecialSetup990 &s990, SpecialSetup800 &s800);
std::vector<PatchVST> ConvertSetup800ToVST(const SpecialSetup800 &s800);
