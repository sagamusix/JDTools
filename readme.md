# JDTools - Patch conversion utility for Roland JD-800 / JD-990

While it is easy to convert JD-800 patches to use with the JD-990 (the JD-990 itself can do it, and there are PC tools for it as well),
there don't appear to be any tools to go in the opposite direction. In the general case this makes sense, because the JD-990 has more features.

However, I purchased a sound bank for the JD-800 where the original SysEx dump for the JD-800 was lost and all I got was a SysEx dump for the JD-990, which my JD-800 obviously couldn't load.

Knowing that the patches in this dump must be fully compatible with the JD-800, I started studying the SysEx dump structures of the two synths and threw together some code in half a day to convert between the two.

The tool can load both SYX files (raw SysEx dumps) and MID / SMF files (Standard MIDI Files) and convert from JD-800 patch format to JD-990 and vice versa. It can also convert special setups but other multi mode settings cannot be converted (there isn't much to convert that the two synths have in common...).

After the basic functionality was done, I wondered what would be required to support the JD-800 VST plugin or the JD-08 as well - turns out, quite a lot! Nevertheless, I managed to add support for them as well, so you can convert original JD-800 patches to the plugin's format (as long as they don't use ROM card waveforms), and also convert plugin banks to use with the original JD-800 (as long as they don't use extended features such as unison or tempo-synced LFOs).
As the plugin appears to be based on Roland's ZenCore engine, don't expect conversions that sound 100% identical. Some parameters seem to have much lower internal precision than on a real JD-800. On the upside, the converted files also work with the Zenology plugin. But as the conversion process is quite complex, it's always possible there is a bug, so please report those if you find any.

Figuring out the plugin and SVZ hardware patch format was a lot of work, so if you find this tool useful, please consider [donating](https://paypal.me/JohannesSchultz) a few bucks.

# Usage

## Conversion

It is possible to convert between practically all format combinations:

- JD-800 SysEx dump (SYX, MID) to JD-990 SysEx dump (SYX)
- JD-800 SysEx dump (SYX, MID) to JD-800 VST / Zenology patch bank (BIN)
- JD-800 SysEx dump (SYX, MID) to JD-08 / ZC1 patch bank (SVZ)
- JD-990 SysEx dump (SYX, MID) to JD-800 SysEx dump (SYX)
- JD-990 SysEx dump (SYX, MID) to JD-800 VST / Zenology patch bank (BIN)
- JD-990 SysEx dump (SYX, MID) to JD-08 / ZC1 patch bank (SVZ)
- JD-800 VST or Zenology patch bank (BIN) to JD-800 SysEx dump (SYX)
- JD-800 VST or Zenology patch bank (BIN) to JD-08 / ZC1 patch bank (SVZ)
- JD-08 / ZC1 patch bank to JD-800 SysEx dump (SYX)
- JD-08 / ZC1 patch bank to JD-800 VST or Zenology patch bank (BIN)

The input format of the conversion is determined automatically, but due to the different output options, the desired target format has to be specified explicitely.

By invoking `JDTools convert syx <input> <output>`, the input file is converted to a SysEx dump (so if the source is a JD-800 SysEx dump, the output file is a JD-990 SysEx dump, in all other cases the output is a JD-800 SysEx dump).

By invoking `JDTools convert bin <input> <output>`, the input file is converted to the JD-800 VST patch bank format (BIN).

By invoking `JDTools convert svz <input> <output>`, the input file is converted to the JD-08 / ZC1 hardware patch bank format (SVZ).

To convert e.g. a JD-800 VST patch bank to a JD-990 SysEx dump, an intermediate conversion to a JD-800 SysEx dump is required.

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

# History

## v.08 (2022-07-31)

- Convert to / from JD-800 VST and Zenology .bin format
- Convert to / from JD-08 / ZC1 hardware .svz format
- JD-990 to JD-800 conversion: Aftertouch bend depth is now initialized to +/-0 instead of -36 for special setups if no key uses aftertouch bend

## v0.7 (2022-07-25)

Various small improvements:

- JD-990 to JD-800 conversion: Aftertouch bend depth is now initialized to +/-0 instead of -36 if no tone uses aftertouch bend
- Temporary patches and setups are now also converted and listed
- On Windows 10 Version 1903 and newer, file paths outside of the current locale are supported


## v0.6 (2022-07-22)

- Adds support for reading Standard MIDI Files (MID / SMF).

## v0.5 (2022-07-21)

- Initial release with conversion, listing and merging functionality.

# License

JDTools is provided under the BSD 3-clause license. JDTools makes use of miniz, which is released under the MIT license. See [license.md](license.md) for details.

# Building

This project is written in C++20. Currently only Visual Studio 2019 project is provided, but the code is platform-independent and should be trivial to convert to any other build system.

