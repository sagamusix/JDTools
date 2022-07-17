// JDTools - Patch conversion tools for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

struct Patch800;
struct Patch990;

void ConvertPatch800To990(const Patch800 &p800, Patch990 &p990);
void ConvertPatch990To800(const Patch990 &p990, Patch800 &p800);
