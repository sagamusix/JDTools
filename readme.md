# JDTools - Patch conversion utility for Roland JD-800 / JD-990

While it is easy to convert JD-800 patches to use with the JD-990 (the JD-990 itself can do it, and there are PC tools for it as well),
there don't appear to be any tools to go in the opposite direction. In the general case this makes sense, because the JD-990 has more features.

However, I purchased a sound bank for the JD-800 where the original SysEx dump for the JD-800 was lost and all I got was a SysEx dump for the JD-990, which my JD-800 obviously couldn't load.

Knowing that the patches in this dump must be fully compatible with the JD-800, I started studying the SysEx dump structures of the two synths and threw together some code in half a day to convert between the two.

The tool can load both SYX files (raw SysEx dumps) and MID / SMF files (Standard MIDI Files) and convert from JD-800 patch format to JD-990 and vice versa. The output is always a SYX file. It can also convert special setups but other multi mode settings cannot be converted (there isn't much to convert that the two synths have in common...).

There's no plans for supporting the JD-08 or the JD-800 VST plugin at this point in time, as their binary exchange format differs greatly from the SysEx dump formats, and the JD-08's MIDI implementation is really minimal - sending a whole bank of patches via MIDI appears to be impossible.

# Usage

## Conversion

Convert a JD-800 SysEx dump to JD-990 format - or vice versa! - by invoking `JDTools convert <input.syx> <output.syx>`. The input format (JD-800 or JD-990) is determined automatically from the SysEx contents.

## Merging

Merge any number of SysEx dumps containing temporary patches by invoking `JDTools merge <input1.syx> <input2.syx> <input3.syx> ... <output.syx>`. If an input file contains multiple dumps for the temporary patch area, they are all considered.

The following batch script can be used to pass a directory name instead of a list of individual files:

```
@echo off
REM Usage: merge.cmd Path\To\Directory output.syx 
setlocal enabledelayedexpansion enableextensions
set baseDir=%~1
set LIST=
for %%x in ("%baseDir%\*.syx") do set LIST=!LIST! "%%x"
set LIST=%LIST:~1%

JDTools merge %LIST% %2
```

## Listing

List all the contents of a SysEx dump by invoking `JDTools list <input.syx>`. This also lists objects that JDTools cannot convert (such as the JD-800 display area), but the actual contents are not shown for most of them.

# License

JDTools is provided under the BSD 3-clause license. See [license.md](license.md) for details.

# Building

This project is written in C++20. Currently only Visual Studio 2019 project is provided, but the code is platform-independent and should be trivial to convert to any other build system.

