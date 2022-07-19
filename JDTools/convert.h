// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

struct Patch800;
struct Patch990;

void ConvertPatch800To990(const Patch800 &p800, Patch990 &p990);
void ConvertPatch990To800(const Patch990 &p990, Patch800 &p800);

struct SpecialSetup800;
struct SpecialSetup990;

void ConvertSetup800To990(const SpecialSetup800 &s800, SpecialSetup990 &s990);
void ConvertSetup990To800(const SpecialSetup990 &s990, SpecialSetup800 &s800);
